# test/test_partitioner.py
from compiler.tensor_partitioner import TensorPartitioner

def test_partitioner():
    # Partition an [8x8] * [8x8] matrix multiply
    # Dimensions are 8x8x8 -> (8/4) * (8/4) * (8/4) = 2 * 2 * 2 = 8 total 4x4 tile tasks
    partitioner = TensorPartitioner(M=8, N=8, K=8, 
                                    base_dram_a=0x1000, 
                                    base_dram_b=0x2000, 
                                    base_dram_c=0x3000)
    tasks = partitioner.partition()

    print(f"Total Tile Tasks Generated: {len(tasks)}")
    assert len(tasks) == 8, f"Expected 8 tile tasks, got {len(tasks)}"

    # Check first task properties
    first = tasks[0]
    assert first.tile_m == 0 and first.tile_n == 0 and first.tile_k == 0
    assert first.dram_addr_a == 0x1000
    assert first.dram_addr_b == 0x2000
    assert first.sram_bank == 0

    # Check bank toggling (ping-pong)
    assert tasks[1].sram_bank == 1
    assert tasks[2].sram_bank == 0

    print("SUCCESS: Tensor Partitioner (M, N, K) Tiling Verified!")

if __name__ == "__main__":
    test_partitioner()