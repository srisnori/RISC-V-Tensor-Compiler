#ifndef SYSTOLIC_ARRAY_HPP
#define SYSTOLIC_ARRAY_HPP

#include <cstdint>
#include <array>

struct PE {
    int32_t weight = 0;
    int32_t accumulator = 0;
    int32_t activation_out = 0;

    void tick(int32_t activation_in) {
        accumulator += act_in * weight;
        act_out = act_in;
    }

    void reset() {
        weight = 0;
        accumulator = 0;
        activation_out = 0;
    }
};


class SystolicArray4x4 {
    public: 
}

#endif