#include <iostream>
#include <cassert>
#include <array>
#include "hardware/systolic_array.hpp"

int main() {
    SystolicArray4x4 array;

    // Load identity-like weight matrix
    std::array<std::array<int32_t, 4>, 4> weights = {{
        {1, 0, 0, 0},
        {0, 2, 0, 0},
        {0, 0, 3, 0},
        {0, 0, 0, 4}
    }};
    array.load_weights(weights);

    // Stream single vector [5, 5, 5, 5] across 4 clock cycles
    std::array<int32_t, 4> input = {5, 5, 5, 5};
    for (int cycle = 0; cycle < 4; ++cycle) {
        array.tick(input);
    }

    auto results = array.get_results();

    // Verify PE[0][0] accumulator = 5 * 1 * 4 cycles = 20
    assert(results[0][0] == 20);
    // Verify PE[1][1] accumulator (delayed by 1 cycle, so 3 ticks executed) = 5 * 2 * 3 = 30
    assert(results[1][1] == 30);

    std::cout << "SUCCESS: 4x4 Systolic Array Grid Verified!" << std::endl;
    return 0;
}