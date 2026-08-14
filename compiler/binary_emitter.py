import struct
from typing import List
from compiler.isa import (Instruction, OP_MAT_LOAD, OP_MAT_STORE, OP_MAT_MUL, OP_SYNC)
from compiler.tensor_partitioner import TileTask

class BinaryEmitter:
    def __init__(self, tasks: List[TileTask]):
        self.tasks = tasks
        self.instructions: List[Instruction] = []

    def emit_instructions(self) -> List[Instruction]:
        self.instructions.clear()

        for task in self.tasks:
            # 1. Load Matrix A Activation Tile from DRAM into SRAM
            self.instructions.append(
                Instruction(
                    opcode=OP_MAT_LOAD,
                    sram_bank=task.sram_bank,
                    dram_addr=task.dram_addr_a & 0xFFFF,
                    sram_addr=0x00 # Placed at base offset of bank
                )
            )

            # 2. Synchronize before compute
            self.instructions.append(
                Instruction(opcode=OP_SYNC, sram_bank=0, dram_addr=0, sram_addr=0)
            )

            # 3. Stream activations through Systolic Array
            self.instructions.append(
                Instruction(
                    opcode=OP_MAT_MUL,
                    sram_bank=task.sram_bank,
                    dram_addr=0x0000,
                    sram_addr=0x00
                )
            )

            # 4. Store Accumulator Results from SRAM back to DRAM C
            self.instructions.append(
                Instruction(
                    opcode=OP_MAT_STORE,
                    sram_bank=task.sram_bank,
                    dram_addr=task.dram_addr_c & 0xFFFF,
                    sram_addr=0x00
                )
            )

            # 5. Pipeline Flush
            self.instructions.append(
                Instruction(opcode=OP_SYNC, sram_bank=0, dram_addr=0, sram_addr=0)
            )

        return self.instructions

    def save_binary(self, output_path: str):
        if not self.instructions:
            self.emit_instructions()

        # Pack 32-bit unsigned integers in little-endian format ('<I')
        with open(output_path, "wb") as f:
            for inst in self.instructions:
                packed = inst.encode()
                f.write(struct.pack("<I", packed))

    def save_assembly(self, output_path: str):
        if not self.instructions:
            self.emit_instructions()

        with open(output_path, "w") as f:
            for inst in self.instructions:
                f.write(f"0x{inst.encode():08X} : {inst}\n")