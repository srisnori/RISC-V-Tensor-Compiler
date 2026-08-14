#include <iostream>
#include <cassert>
#include <cstdint>
#include "runtime/dram.hpp"
#include "hardware/sram.hpp"
#include "runtime/dma_engine.hpp"

int main() {
    HostDRAM dram;
    ScratchpadSRAM sram;
    DMAEngine dma;

    // 1. Write a 4x4 matrix (16 integers = 64 bytes) into DRAM at 0x2000
    int32_t sample_matrix[16];
    for (int i = 0; i < 16; ++i) {
        sample_matrix[i] = (i + 1) * 10;
    }
    dram.write_bytes(0x2000, reinterpret_cast<const uint8_t*>(sample_matrix), sizeof(sample_matrix));

    // 2. Trigger DMA transfer: DRAM (0x2000) -> SRAM Bank 0 (Offset 0)
    // Latency = 10 setup + (64 bytes / 8 bytes_per_cycle) = 18 cycles
    dma.start_dram_to_sram(dram, 0x2000, sram, 0, 0, sizeof(sample_matrix));

    assert(dma.is_busy() == true);
    assert(dma.cycles_remaining == 18);

    // 3. Step clock 18 cycles
    for (int cycle = 0; cycle < 18; ++cycle) {
        dma.tick();
    }

    // 4. Verify transfer is done and data inside SRAM matches DRAM
    assert(dma.is_busy() == false);

    int32_t* sram_data = reinterpret_cast<int32_t*>(sram.get_ptr(0, 0));
    for (int i = 0; i < 16; ++i) {
        assert(sram_data[i] == (i + 1) * 10);
    }

    std::cout << "SUCCESS: DMA Engine Asynchronous Transfer & Latency Verified!" << std::endl;
    return 0;
}