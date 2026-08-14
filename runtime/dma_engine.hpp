#ifndef DMA_ENGINE_HPP
#define DMA_ENGINE_HPP

#include <cstdint>
#include <cstring>
#include "runtime/dram.hpp"
#include "hardware/sram.hpp"

class DMAEngine {
public:
    static constexpr uint64_t SETUP_LATENCY = 10;   
    static constexpr uint64_t BYTES_PER_CYCLE = 8;  
    bool busy = false;
    uint64_t cycles_remaining = 0;

    void start_dram_to_sram(HostDRAM& dram, uint32_t dram_addr, ScratchpadSRAM& sram, uint8_t sram_bank, 
                            uint16_t sram_addr, size_t num_bytes) {
        uint8_t* src = dram.get_ptr(dram_addr);
        uint8_t* dst = sram.get_ptr(sram_bank, sram_addr);
        std::memcpy(dst, src, num_bytes);

        busy = true;
        cycles_remaining = SETUP_LATENCY + (num_bytes / BYTES_PER_CYCLE);
    }

    void start_sram_to_dram(ScratchpadSRAM& sram, uint8_t sram_bank, uint16_t sram_addr, 
                            HostDRAM& dram, uint32_t dram_addr, 
                            size_t num_bytes) {
        uint8_t* src = sram.get_ptr(sram_bank, sram_addr);
        uint8_t* dst = dram.get_ptr(dram_addr);
        std::memcpy(dst, src, num_bytes);

        busy = true;
        cycles_remaining = SETUP_LATENCY + (num_bytes / BYTES_PER_CYCLE);
    }

    void tick() {
        if (busy) {
            if (cycles_remaining > 0) {
                cycles_remaining--;
            }
            if (cycles_remaining == 0) {
                busy = false;
            }
        }
    }

    bool is_busy() const {
        return busy;
    }
};

#endif