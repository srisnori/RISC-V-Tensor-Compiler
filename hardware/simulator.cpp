#include <iostream>
#include <vector>
#include <iomanip>
#include <cstdint>
#include <fstream>
#include <stdexcept>

#include "hardware/isa.hpp"
#include "hardware/sram.hpp"
#include "hardware/systolic_array.hpp"
#include "runtime/dram.hpp"
#include "runtime/dma_engine.hpp"
#include "runtime/aware.hpp"

class ConcurrentAcceleratorSimulator {
public:
    HostDRAM dram;
    ScratchpadSRAM sram;
    DMAEngine dma;
    SystolicArray4x4 systolic_array;
    uint64_t clock_cycles = 0;

    std::vector<EventToken> event_stream;

    // Active compute state
    bool compute_busy = false;
    int compute_cycles_left = 0;
    int current_compute_bank = 0;
    int32_t* current_act_tile = nullptr;

    void log_event(const EventToken& token) {
        if (token.signal != AwarenessSignal::NONE) {
            event_stream.push_back(token);
        }
    }

    void run_program_concurrent(const std::vector<uint32_t>& program) {
        size_t pc = 0;

        // Run until all instructions are dispatched and all units are idle
        while (pc < program.size() || dma.is_busy() || compute_busy) {
            clock_cycles++;

            // 1. Hazard-Aware Instruction Dispatch
            if (pc < program.size()) {
                Instruction inst = Instruction::decode(program[pc]);

                // LOAD: Only dispatch if DMA is free AND compute isn't using this bank
                if (inst.opcode == OP_MAT_LOAD && !dma.is_busy()) {
                    bool bank_is_locked = compute_busy && (current_compute_bank == inst.sram_bank);
                    if (!bank_is_locked) {
                        EventToken tok = dma.start_dram_to_sram(
                            dram, inst.dram_addr, sram, inst.sram_bank, inst.sram_addr, 64, clock_cycles
                        );
                        log_event(tok);
                        pc++;
                    }
                } 
                // STORE: Only dispatch if DMA is free AND compute isn't using this bank
                else if (inst.opcode == OP_MAT_STORE && !dma.is_busy()) {
                    bool bank_is_locked = compute_busy && (current_compute_bank == inst.sram_bank);
                    if (!bank_is_locked) {
                        EventToken tok = dma.start_sram_to_dram(
                            sram, inst.sram_bank, inst.sram_addr, dram, inst.dram_addr, 64, clock_cycles
                        );
                        log_event(tok);
                        pc++;
                    }
                } 
                // COMPUTE: Only dispatch if compute is idle AND DMA isn't writing to this bank
                else if (inst.opcode == OP_MAT_MUL && !compute_busy) {
                    bool dma_is_writing_this_bank = dma.is_busy() && (dma.active_sram_bank == inst.sram_bank);
                    if (!dma_is_writing_this_bank) {
                        sram.lock_bank_for_compute(inst.sram_bank);
                        current_compute_bank = inst.sram_bank;
                        current_act_tile = reinterpret_cast<int32_t*>(
                            sram.get_ptr(inst.sram_bank, inst.sram_addr, false, nullptr, clock_cycles)
                        );
                        compute_busy = true;
                        compute_cycles_left = 8; // 4 feed cycles + 4 drain cycles
                        pc++;
                    }
                } 
                // SYNC Barrier: wait for everything to finish
                else if (inst.opcode == OP_SYNC) {
                    if (!dma.is_busy() && !compute_busy) {
                        pc++;
                    }
                }
            }

            // 2. Step DMA unit
            if (dma.is_busy()) {
                EventToken dma_tok = dma.tick(clock_cycles);
                log_event(dma_tok);
            }

            // 3. Step Systolic Array concurrently in the SAME cycle
            if (compute_busy) {
                if (compute_cycles_left > 4) {
                    int col = 8 - compute_cycles_left;
                    std::array<int32_t, 4> input_col = {
                        current_act_tile[0 * 4 + col],
                        current_act_tile[1 * 4 + col],
                        current_act_tile[2 * 4 + col],
                        current_act_tile[3 * 4 + col]
                    };
                    EventToken tok = systolic_array.tick(input_col, true, clock_cycles);
                    log_event(tok);
                } else {
                    EventToken tok = systolic_array.tick(std::array<int32_t, 4>{0, 0, 0, 0}, true, clock_cycles);
                    log_event(tok);
                }

                compute_cycles_left--;
                if (compute_cycles_left == 0) {
                    compute_busy = false;
                    EventToken free_tok = sram.release_bank_after_compute(current_compute_bank, clock_cycles);
                    log_event(free_tok);
                }
            }
        }
    }

    void print_awareness_summary() const {
        std::cout << "\n================ HARDWARE-AWARENESS CONCURRENT LOG ================\n";
        std::cout << std::left << std::setw(12) << "Cycle" 
                  << std::setw(28) << "Event Signal" 
                  << std::setw(10) << "Unit ID" 
                  << "Context Data\n";
        std::cout << "-------------------------------------------------------------------\n";

        for (const auto& ev : event_stream) {
            std::string sig_name;
            switch (ev.signal) {
                case AwarenessSignal::COMPUTE_STARVED_WEIGHTS: sig_name = "COMPUTE_STARVED_WEIGHTS"; break;
                case AwarenessSignal::COMPUTE_STARVED_INPUTS:  sig_name = "COMPUTE_STARVED_INPUTS";  break;
                case AwarenessSignal::COMPUTE_TILE_COMPLETE:   sig_name = "COMPUTE_TILE_COMPLETE";   break;
                case AwarenessSignal::SRAM_BUFFER_SLOT_FREED:  sig_name = "SRAM_BUFFER_SLOT_FREED";  break;
                case AwarenessSignal::SRAM_BANK_CONFLICT_WARN: sig_name = "SRAM_BANK_CONFLICT_WARN"; break;
                case AwarenessSignal::DMA_TRANSFER_COMPLETE:   sig_name = "DMA_TRANSFER_COMPLETE";   break;
                case AwarenessSignal::DRAM_TRANSFER_DELAYED:   sig_name = "DRAM_TRANSFER_DELAYED";   break;
                default: sig_name = "UNKNOWN"; break;
            }

            std::cout << std::left << std::setw(12) << ev.timestamp_cycle 
                      << std::setw(28) << sig_name 
                      << std::setw(10) << static_cast<int>(ev.unit_id) 
                      << "0x" << std::hex << ev.context_data << std::dec << "\n";
        }
        std::cout << "===================================================================\n";
    }
};

std::vector<uint32_t> load_binary_program(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open binary file: " + path);
    }
    std::vector<uint32_t> program;
    uint32_t inst;
    while (file.read(reinterpret_cast<char*>(&inst), sizeof(uint32_t))) {
        program.push_back(inst);
    }
    return program;
}

int main(int argc, char** argv) {
    std::cout << "RISC-V HARDWARE ACCELERATOR (HAZARD-AWARE CONCURRENT EXECUTION)" << std::endl;
    std::string bin_path = (argc > 1) ? argv[1] : "test/test_program.bin";
    ConcurrentAcceleratorSimulator sim;

    std::array<std::array<int32_t, 4>, 4> weights = {{
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    }};
    sim.systolic_array.load_weights(weights);

    try {
        std::vector<uint32_t> program = load_binary_program(bin_path);
        std::cout << "Loaded " << program.size() << " instructions from: " << bin_path << std::endl;

        sim.run_program_concurrent(program);

        std::cout << "\nHAZARD-AWARE CONCURRENT EXECUTION COMPLETE!" << std::endl;
        std::cout << "TOTAL HARDWARE CLOCK CYCLES: " << sim.clock_cycles << std::endl;

        sim.print_awareness_summary();

    } catch (const std::exception& e) {
        std::cerr << "RUNTIME ERROR: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}