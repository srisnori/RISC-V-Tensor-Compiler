OP_MAT_LOAD  = 0x01
OP_MAT_STORE = 0x02
OP_MAT_MUL   = 0x03
OP_SYNC      = 0x04

class Instruction:
    def __init__(self, opcode: int, sram_bank: int, dram_addr: int, sram_addr: int):
        self.opcode = opcode
        self.sram_bank = sram_bank
        self.dram_addr = dram_addr
        self.sram_addr = sram_addr
    
    def encode(self) -> int:
        return (((self.opcode & 0x7F) << 25) | ((self.sram_bank & 0x01) << 24) |
            ((self.dram_addr & 0xFFFF) << 8) | (self.sram_addr & 0xFF))

    @classmethod
    def decode(cls, raw_inst: int) -> 'Instruction':
        opcode = (raw_inst >> 25) & 0x7F
        sram_bank = (raw_inst >> 24) & 0x01
        dram_addr = (raw_inst >> 8) & 0xFFFF
        sram_addr = raw_inst & 0xFF
        return cls(opcode, sram_bank, dram_addr, sram_addr)
    
    def __repr__(self):
        op_names = {OP_MAT_LOAD: "MAT_LOAD", OP_MAT_STORE: "MAT_STORE",
            OP_MAT_MUL: "MAT_MUL", OP_SYNC: "SYNC"}
        return (f"Instruction({op_names.get(self.opcode, 'UNKNOWN')}, "
                f"bank={self.sram_bank}, dram=0x{self.dram_addr:04X}, sram=0x{self.sram_addr:02X})")