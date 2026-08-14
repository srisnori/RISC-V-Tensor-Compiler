#ifndef ISA_HPP
#define ISA_HPP
#include <cstdint>

enum opcode : uint8_t {
    OP_MAT_LOAD = 0x01,
    OP_MAT_STORE = 0x02,
    OP_MAT_MUL = 0x03,
    OP_SYNC = 0x04,
};

struct Instruction {
    uint8_t opcode;
    uint8_t sram_bank;
    uint16_t dram_addr;
    uint8_t sram_addr;

    uint32_t encode() const {
        return (static_cast<uint32_t>(opcode) << 25) |
                (static_cast<uint32_t>(sram_bank & 0x01) << 24) |
                (static_cast<uint32_t>(dram_addr) << 8) |
                (static_cast<uint32_t>(sram_addr));
    }

    static Instruction decode(uint32_t raw_inst) {
        Instruction inst;
        inst.opcode = (raw_inst >> 25) & 0x7F;
        inst.sram_bank = (raw_inst >> 24) & 0x01;
        inst.dram_addr = (raw_inst >> 8) & 0xFFFF;
        inst.sram_addr = raw_inst & 0xFF;
        return inst;
    }
};

#endif