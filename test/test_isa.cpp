#include <iostream>
#include <cassert>
#include "hardware/isa.hpp"

int main() {
    // 1. Create a dummy instruction
    Instruction orig;
    orig.opcode    = OP_MAT_LOAD;
    orig.sram_bank = 1;
    orig.dram_addr = 0x1234;
    orig.sram_addr = 0x5A;

    // 2. Encode to 32-bit integer
    uint32_t packed = orig.encode();
    std::cout << "Packed 32-bit Hex: 0x" << std::hex << packed << std::endl;

    // 3. Decode back to struct
    Instruction unpacked = Instruction::decode(packed);

    // 4. Assert every field matches
    assert(unpacked.opcode == orig.opcode);
    assert(unpacked.sram_bank == orig.sram_bank);
    assert(unpacked.dram_addr == orig.dram_addr);
    assert(unpacked.sram_addr == orig.sram_addr);

    std::cout << "SUCCESS: All ISA bits encoded and decoded perfectly!" << std::endl;
    return 0;
}