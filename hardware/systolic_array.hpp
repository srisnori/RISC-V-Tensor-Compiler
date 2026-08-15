#ifndef SYSTOLIC_ARRAY_HPP
#define SYSTOLIC_ARRAY_HPP

#include <cstdint>
#include <array>
#include "../runtime/aware.hpp"

struct PE {
    int32_t weight = 0;
    int32_t accumulator = 0;
    int32_t activation_out = 0;

    void tick(int32_t activation_in) {
        accumulator += activation_in * weight;
        activation_out = activation_in;
    }

    void reset() {
        weight = 0;
        accumulator = 0;
        activation_out = 0;
    }
};

class SystolicArray4x4 {
public: 
    std::array<std::array<PE, 4>, 4> grid;
    bool weights_loaded = false;
    uint32_t active_tile_cycles = 0;
    uint16_t current_tile_id = 0;

    void load_weights(const std::array<std::array<int32_t, 4>, 4>& weights, uint16_t tile_id = 0) {
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                grid[r][c].weight = weights[r][c];
            }
        }
        weights_loaded = true;
        current_tile_id = tile_id;
        active_tile_cycles = 0;
    }

    // Context-Aware Tick: Computes AND returns semantic event tokens
    EventToken tick(const std::array<int32_t, 4>& inputs_left, bool inputs_valid = true, uint32_t current_cycle = 0) {
        // Starvation Awareness: Check if weights are missing
        if (!weights_loaded) {
            return EventToken{
                .signal = AwarenessSignal::COMPUTE_STARVED_WEIGHTS,
                .unit_id = 0,
                .context_data = current_tile_id,
                .timestamp_cycle = current_cycle
            };
        }

        // Starvation Awareness: Check if inputs/activations are missing
        if (!inputs_valid) {
            return EventToken{
                .signal = AwarenessSignal::COMPUTE_STARVED_INPUTS,
                .unit_id = 0,
                .context_data = current_tile_id,
                .timestamp_cycle = current_cycle
            };
        }

        // Compute Step (Systolic Propagation)
        std::array<std::array<int32_t, 4>, 4> next_acts;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                next_acts[r][c] = (c == 0) ? inputs_left[r] : grid[r][c - 1].activation_out;
            }
        }

        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                grid[r][c].tick(next_acts[r][c]);
            }
        }

        active_tile_cycles++;

        // Completion Awareness: For a 4x4 array with 4 inputs, 
        // full wavefront computation drains in 7-8 cycles
        if (active_tile_cycles == 7) {
            active_tile_cycles = 0; // RESET for the next tile
            return EventToken{
                .signal = AwarenessSignal::COMPUTE_TILE_COMPLETE,
                .unit_id = 0,
                .context_data = current_tile_id,
                .timestamp_cycle = current_cycle
            };
        }

        return EventToken{.signal = AwarenessSignal::NONE, .unit_id = 0, .context_data = 0, .timestamp_cycle = current_cycle};
    }

    std::array<std::array<int32_t, 4>, 4> get_results() const {
        std::array<std::array<int32_t, 4>, 4> res;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                res[r][c] = grid[r][c].accumulator;
            }
        }
        return res;
    }

    void reset_accumulators() {
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                grid[r][c].accumulator = 0;
            }
        }
        weights_loaded = false;
        active_tile_cycles = 0;
    }
};

#endif