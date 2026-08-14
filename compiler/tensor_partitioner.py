# compiler/tensor_partitioner.py
from dataclasses import dataclass
from typing import List

@dataclass
class TileTask:
    tile_m: int        # Row index in output matrix C (step of 4)
    tile_n: int        # Col index in output matrix C (step of 4)
    tile_k: int        # Reduction index across K (step of 4)
    dram_addr_a: int   # Byte address for Matrix A tile in DRAM
    dram_addr_b: int   # Byte address for Matrix B tile in DRAM
    dram_addr_c: int   # Byte address for Matrix C tile in DRAM
    sram_bank: int     # 0 (Bank A) or 1 (Bank B) for double buffering

class TensorPartitioner:
    TILE_SIZE = 4            # 4x4 Systolic Array native tile
    BYTES_PER_ELEM = 4       # int32 / float32 = 4 bytes
    TILE_BYTES = 4 * 4 * 4   # 64 bytes per 4x4 tile

    def __init__(self, M: int, N: int, K: int, base_dram_a: int = 0x1000, 
                 base_dram_b: int = 0x2000, base_dram_c: int = 0x3000):
        assert M % self.TILE_SIZE == 0, f"M ({M}) must be a multiple of {self.TILE_SIZE}"
        assert N % self.TILE_SIZE == 0, f"N ({N}) must be a multiple of {self.TILE_SIZE}"
        assert K % self.TILE_SIZE == 0, f"K ({K}) must be a multiple of {self.TILE_SIZE}"

        self.M = M
        self.N = N
        self.K = K
        self.base_dram_a = base_dram_a
        self.base_dram_b = base_dram_b
        self.base_dram_c = base_dram_c

    def _calc_dram_offset_a(self, m: int, k: int) -> int:
        # Row-major offset for Matrix A [M x K]: (m * K + k) * 4 bytes
        return self.base_dram_a + ((m * self.K + k) * self.BYTES_PER_ELEM)

    def _calc_dram_offset_b(self, k: int, n: int) -> int:
        # Row-major offset for Matrix B [K x N]: (k * N + n) * 4 bytes
        return self.base_dram_b + ((k * self.N + n) * self.BYTES_PER_ELEM)

    def _calc_dram_offset_c(self, m: int, n: int) -> int:
        # Row-major offset for Matrix C [M x N]: (m * N + n) * 4 bytes
        return self.base_dram_c + ((m * self.N + n) * self.BYTES_PER_ELEM)

    def partition(self) -> List[TileTask]:
        tasks: List[TileTask] = []
        current_bank = 0

        # Generate 3D Loop-Nest Plan: M -> N -> K
        for m in range(0, self.M, self.TILE_SIZE):
            for n in range(0, self.N, self.TILE_SIZE):
                for k in range(0, self.K, self.TILE_SIZE):
                    task = TileTask(
                        tile_m=m,
                        tile_n=n,
                        tile_k=k,
                        dram_addr_a=self._calc_dram_offset_a(m, k),
                        dram_addr_b=self._calc_dram_offset_b(k, n),
                        dram_addr_c=self._calc_dram_offset_c(m, n),
                        sram_bank=current_bank
                    )
                    tasks.append(task)
                    # Ping-pong bank select (0 -> 1 -> 0 -> 1)
                    current_bank = 1 - current_bank
        return tasks