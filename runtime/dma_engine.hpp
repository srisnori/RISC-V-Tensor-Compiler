#ifndef DMA_ENGINE_HPP
#define DMA_ENGINE_HPP

#include <cstdint>
#include <cstring>
#include "runtime/dram.hpp"
#include "hardware/sram.hpp"
#include "runtime/aware.hpp"

class DMAEngine {
public:
    static constexpr uint64_t SETUP_LATENCY = 10;   
    static constexpr uint64_t BYTES_PER_CYCLE = 8;  
    
    bool busy = false;
    uint64_t cycles_remaining = 0;
    uint8_t  active_sram_bank = 0;
    bool     is_to_sram = false;

    // Start DRAM -> SRAM transfer with awareness validation
    EventToken start_dram_to_sram(HostDRAM& dram, uint32_t dram_addr, 
                                 ScratchpadSRAM& sram, uint8_t sram_bank, 
                                 uint16_t sram_addr, size_t num_bytes, 
                                 uint32_t current_cycle = 0) {
        EventToken conflict_token{.signal = AwarenessSignal::NONE};

        // Pass is_dma_access = true to check if bank is locked by systolic compute
        uint8_t* dst = sram.get_ptr(sram_bank, sram_addr, true, &conflict_token, current_cycle);
        
        if (conflict_token.signal == AwarenessSignal::SRAM_BANK_CONFLICT_WARN) {
            // Signal to the runtime that the DMA was forced to touch a locked bank
            return conflict_token;
        }

        uint8_t* src = dram.get_ptr(dram_addr);
        std::memcpy(dst, src, num_bytes);

        busy = true;
        active_sram_bank = sram_bank;
        is_to_sram = true;
        cycles_remaining = SETUP_LATENCY + (num_bytes / BYTES_PER_CYCLE);

        return EventToken{.signal = AwarenessSignal::NONE, .timestamp_cycle = current_cycle};
    }

    EventToken start_sram_to_dram(ScratchpadSRAM& sram, uint8_t sram_bank, uint16_t sram_addr, 
                                 HostDRAM& dram, uint32_t dram_addr, 
                                 size_t num_bytes, uint32_t current_cycle = 0) {
        EventToken conflict_token{.signal = AwarenessSignal::NONE};
        uint8_t* src = sram.get_ptr(sram_bank, sram_addr, true, &conflict_token, current_cycle);
        
        if (conflict_token.signal == AwarenessSignal::SRAM_BANK_CONFLICT_WARN) {
            return conflict_token;
        }

        uint8_t* dst = dram.get_ptr(dram_addr);
        std::memcpy(dst, src, num_bytes);

        busy = true;
        active_sram_bank = sram_bank;
        is_to_sram = false;
        cycles_remaining = SETUP_LATENCY + (num_bytes / BYTES_PER_CYCLE);

        return EventToken{.signal = AwarenessSignal::NONE, .timestamp_cycle = current_cycle};
    }

    // Context-Aware Tick: Emits an event token when transfer finishes
    EventToken tick(uint32_t current_cycle = 0) {
        if (busy) {
            if (cycles_remaining > 0) {
                cycles_remaining--;
            }
            if (cycles_remaining == 0) {
                busy = false;
                // Emit transfer complete token
                return EventToken{
                    .signal = AwarenessSignal::DMA_TRANSFER_COMPLETE,
                    .unit_id = active_sram_bank,
                    .context_data = static_cast<uint16_t>(is_to_sram ? 1 : 0),
                    .timestamp_cycle = current_cycle
                };
            }
        }
        return EventToken{.signal = AwarenessSignal::NONE, .timestamp_cycle = current_cycle};
    }

    bool is_busy() const {
        return busy;
    }
};

#endif