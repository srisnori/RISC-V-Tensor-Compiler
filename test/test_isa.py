# test/test_isa.py
from compiler.isa import Instruction, OP_MAT_LOAD

def test_isa_encoding():
    # Same values we tested in C++: OP_MAT_LOAD, bank=1, dram=0x1234, sram=0x5A
    inst = Instruction(OP_MAT_LOAD, 1, 0x1234, 0x5A)
    packed = inst.encode()
    print(f"Python Packed Hex: {hex(packed)}")
    
    # Assert exact match with C++ test result (0x312345A)
    assert packed == 0x312345A, f"Expected 0x312345A, got {hex(packed)}"

    # Test decode
    unpacked = Instruction.decode(packed)
    assert unpacked.opcode == OP_MAT_LOAD
    assert unpacked.sram_bank == 1
    assert unpacked.dram_addr == 0x1234
    assert unpacked.sram_addr == 0x5A
    print("SUCCESS: Python ISA Encoder & Decoder verified against C++!")

if __name__ == "__main__":
    test_isa_encoding()