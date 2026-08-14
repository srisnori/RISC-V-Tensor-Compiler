#include <iostream>
#include <vector>
#include <iomanip>
#include <cstdint>

#include "hardware/isa.hpp"
#include "hardware/sram.hpp"
#include "hardware/systolic_array.hpp"

class AcceleratorSimulator {
public:
    ScratchpadSRAM sram;
    SystolicArray4x4 systolic_array;
    uint64_t clock_cycles = 0;

    void execute_instruction(uint32_t raw_inst) {
        Instruction inst = Instruction::decode(raw_inst);
        clock_cycles++;

        switch(inst.opcode) {
            case OP_MAT_LOAD: {
                std::cout << "[Cycle " << clock_cycles << "] EXEC: MAT_LOAD -> SRAM Bank " 
                    << static_cast<int>(inst.sram_bank) 
                    << " (Offset " << static_cast<int>(inst.sram_addr) << ") from DRAM 0x" 
                    << std::hex << inst.dram_addr << std::dec << std::endl; 
                
                    clock_cycles += 10;
                    break;
            }

            case OP_MAT_STORE: {
                std::cout << "[Cycle " << clock_cycles << "] EXEC: MAT_STORE -> SRAM Bank " 
                          << static_cast<int>(inst.sram_bank) 
                          << " to DRAM 0x" << std::hex << inst.dram_addr << std::dec << std::endl;
                
                clock_cycles += 10;
                break;
            }

            case OP_MAT_MUL: {
                std::cout << "[Cycle " << clock_cycles << "] EXEC: MAT_MUL -> Streaming Systolic Array" << std::endl;
                std::array<int32_t, 4> sample_inputs = {1, 2, 3, 4};
                for (int tick = 0; tick < 8; ++tick) {
                    systolic_array.tick(sample_inputs);
                    clock_cycles++;
                }
                break;
            }

            case OP_SYNC: {
                std::cout << "[Cycle " << clock_cycles << "] EXEC: SYNC -> Execution Pipeline Flushed" << std::endl;
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

int main () {
    std::cout << "RISC-V HARDWARE SIMULATOR RUNNER" << std::endl;
    AcceleratorSimulator sim;
    std::array<std::array<int32_t, 4>, 4> sample_weights = {{
        {1, 2, 0, 1},
        {0, 1, 3, 0},
        {2, 0, 1, 1},
        {1, 1, 0, 2}
    }};
    sim.systolic_array.load_weights(sample_weights);

    Instruction i1{OP_MAT_LOAD, 0, 0x1000, 0x00}; 
    Instruction i2{OP_MAT_MUL, 0, 0x0000, 0x00}; 
    Instruction i3{OP_SYNC, 0, 0x0000, 0x00};

    std::vector<uint32_t> program = {i1.encode(), i2.encode(), i3.encode()};
    for (uint32_t raw_inst : program) {
        sim.execute_instruction(raw_inst);
    }

    std::cout << "TOTAL CLOCK CYCLES: " << sim.clock_cycles << std::endl;
    return 0;
}