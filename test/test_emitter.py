# test/test_emitter.py
import os
import struct
from compiler.tensor_partitioner import TensorPartitioner
from compiler.binary_emitter import BinaryEmitter
from compiler.isa import Instruction

def test_emitter():
    # 1. Partition [8x8] * [8x8] -> 8 Tile Tasks
    partitioner = TensorPartitioner(M=8, N=8, K=8)
    tasks = partitioner.partition()

    # 2. Emit instructions
    emitter = BinaryEmitter(tasks)
    instructions = emitter.emit_instructions()

    # 8 tasks * 5 instructions per task = 40 instructions
    print(f"Total Instructions Emitted: {len(instructions)}")
    assert len(instructions) == 40, f"Expected 40 instructions, got {len(instructions)}"

    # 3. Save binary and assembly files
    bin_path = "test/test_program.bin"
    asm_path = "test/test_program.asm"
    emitter.save_binary(bin_path)
    emitter.save_assembly(asm_path)

    # 4. Verify binary file size: 40 instructions * 4 bytes = 160 bytes
    file_size = os.path.getsize(bin_path)
    print(f"Binary Artifact Size: {file_size} bytes")
    assert file_size == 160, f"Expected 160 bytes, got {file_size}"

    # 5. Read back binary and verify first instruction decodes back properly
    with open(bin_path, "rb") as f:
        first_word = struct.unpack("<I", f.read(4))[0]
    decoded = Instruction.decode(first_word)
    assert decoded.opcode == instructions[0].opcode
    assert decoded.dram_addr == instructions[0].dram_addr

    print("SUCCESS: Binary Emitter and Machine Code Payload Verified!")

if __name__ == "__main__":
    test_emitter()