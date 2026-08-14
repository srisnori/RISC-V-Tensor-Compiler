#ifndef SYSTOLIC_ARRAY_HPP
#define SYSTOLIC_ARRAY_HPP

#include <cstdint>
#include <array>

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

        void load_weights(const std::array<std::array<int32_t, 4>, 4>& weights) {
            for (int r = 0; r < 4; ++r) {
                for (int c = 0; c < 4; ++c) {
                    grid[r][c].weight = weights[r][c];
                }
            }
        }

        void tick(const std::array<int32_t, 4>& inputs_left) {
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
};

#endif