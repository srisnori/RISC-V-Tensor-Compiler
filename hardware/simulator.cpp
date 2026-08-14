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

class AcceleratorSimulator {
public:
    HostDRAM dram;
    ScratchpadSRAM sram;
    DMAEngine dma;
    SystolicArray4x4 systolic_array;
    uint64_t clock_cycles = 0;

    void execute_instruction(uint32_t raw_inst) {
        Instruction inst = Instruction::decode(raw_inst);
        clock_cycles++; // 1 cycle fetch & decode

        switch (inst.opcode) {
            case OP_MAT_LOAD: {
                dma.start_dram_to_sram(dram, inst.dram_addr, sram, inst.sram_bank, inst.sram_addr, 64);
                while (dma.is_busy()) {
                    dma.tick();
                    clock_cycles++;
                }
                break;
            }

            case OP_MAT_STORE: {
                dma.start_sram_to_dram(sram, inst.sram_bank, inst.sram_addr, dram, inst.dram_addr, 64);
                while (dma.is_busy()) {
                    dma.tick();
                    clock_cycles++;
                }
                break;
            }

            case OP_MAT_MUL: {
                int32_t* act_tile = reinterpret_cast<int32_t*>(sram.get_ptr(inst.sram_bank, inst.sram_addr));

                for (int col = 0; col < 4; ++col) {
                    std::array<int32_t, 4> input_col = {
                        act_tile[0 * 4 + col],
                        act_tile[1 * 4 + col],
                        act_tile[2 * 4 + col],
                        act_tile[3 * 4 + col]
                    };
                    systolic_array.tick(input_col);
                    clock_cycles++;
                }
                for (int drain = 0; drain < 4; ++drain) {
                    systolic_array.tick({0, 0, 0, 0});
                    clock_cycles++;
                }
                break;
            }

            case OP_SYNC: {
                clock_cycles += 1;
                break;
            }

            default: {
                std::cerr << "ERROR: Unrecognized Opcode: 0x" 
                          << std::hex << static_cast<int>(inst.opcode) << std::dec << std::endl;
                break;
            }
        }
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
    std::cout << "RISC-V HARDWARE ACCELERATOR RUNNER" << std::endl;
    std::string bin_path = (argc > 1) ? argv[1] : "test/test_program.bin";
    AcceleratorSimulator sim;

    // Load static weights into systolic array (4x4 identity matrix for testing)
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

        for (uint32_t raw_inst : program) {
            sim.execute_instruction(raw_inst);
        }

        std::cout << "EXECUTION COMPLETE!" << std::endl;
        std::cout << "TOTAL HARDWARE CLOCK CYCLES: " << sim.clock_cycles << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "RUNTIME ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}