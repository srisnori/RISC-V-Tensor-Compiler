#pragma once
#include <cstdint>

enum class AwarenessSignal : uint8_t {
    NONE                     = 0x00,
    COMPUTE_STARVED_WEIGHTS  = 0x01,
    COMPUTE_STARVED_INPUTS   = 0x02,
    COMPUTE_TILE_COMPLETE    = 0x03,
    SRAM_BUFFER_SLOT_FREED   = 0x10,
    SRAM_BANK_CONFLICT_WARN  = 0x11,
    DMA_TRANSFER_COMPLETE    = 0x12,
    DRAM_TRANSFER_DELAYED    = 0x20
};

struct alignas(8) EventToken {
    AwarenessSignal signal;
    uint8_t         unit_id;
    uint16_t        context_data;
    uint32_t        timestamp_cycle;
};