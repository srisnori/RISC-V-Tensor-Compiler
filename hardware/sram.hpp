#ifndef SRAM_HPP
#define SRAM_HPP

#include <cstdint>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include "runtime/aware.hpp"

class ScratchpadSRAM {
private:
    static constexpr size_t BANK_SIZE_BYTES = 32768; // 32KB
    std::vector<uint8_t> bank_A;
    std::vector<uint8_t> bank_B;

    // State tracking for double-buffering awareness
    uint8_t compute_owner_bank = 0; // 0 for bank_A, 1 for bank_B
    bool bank_A_locked = false;      // True if actively in use by compute
    bool bank_B_locked = false;

public: 
    ScratchpadSRAM() : bank_A(BANK_SIZE_BYTES, 0), bank_B(BANK_SIZE_BYTES, 0) {}

    void reset() {
        std::fill(bank_A.begin(), bank_A.end(), 0);
        std::fill(bank_B.begin(), bank_B.end(), 0);
        bank_A_locked = false;
        bank_B_locked = false;
        compute_owner_bank = 0;
    }

    // Access pointer with conflict detection
    uint8_t* get_ptr(uint8_t bank, uint16_t offset, bool is_dma_access = false, EventToken* out_token = nullptr, uint32_t current_cycle = 0) {
        if (offset >= BANK_SIZE_BYTES) {
            throw std::out_of_range("SRAM offset exceeds 32KB bank limit");
        }

        // Awareness Check: Did DMA try to write to the bank the systolic array is currently using?
        if (is_dma_access && ((bank == 0 && bank_A_locked) || (bank == 1 && bank_B_locked))) {
            if (out_token) {
                *out_token = EventToken{
                    .signal = AwarenessSignal::SRAM_BANK_CONFLICT_WARN,
                    .unit_id = bank,
                    .context_data = offset,
                    .timestamp_cycle = current_cycle
                };
            }
        }

        return (bank == 0) ? &bank_A[offset] : &bank_B[offset];
    }

    // Called when Systolic Array starts computing on a bank
    void lock_bank_for_compute(uint8_t bank) {
        if (bank == 0) bank_A_locked = true;
        else bank_B_locked = true;
        compute_owner_bank = bank;
    }

    // Called when Systolic Array completes its 4x4 tile pass
    EventToken release_bank_after_compute(uint8_t bank, uint32_t current_cycle = 0) {
        if (bank == 0) bank_A_locked = false;
        else bank_B_locked = false;

        // Emit semantic signal: This bank is now free for DMA to stream the next tile
        return EventToken{
            .signal = AwarenessSignal::SRAM_BUFFER_SLOT_FREED,
            .unit_id = bank,
            .context_data = 0,
            .timestamp_cycle = current_cycle
        };
    }

    uint8_t get_compute_bank() const { return compute_owner_bank; }
    uint8_t get_idle_dma_bank() const { return (compute_owner_bank == 0) ? 1 : 0; }
};

#endif