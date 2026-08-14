#include <iostream>
#include <cassert>
#include <cstdint>
#include <vector>
#include "runtime/dram.hpp"

int main() {
    HostDRAM dram;

    // 1. Write an array of 4 integers into DRAM at offset 0x1000
    int32_t original_data[4] = {10, 20, 30, 40};
    dram.write_bytes(0x1000, reinterpret_cast<const uint8_t*>(original_data), sizeof(original_data));

    // 2. Read back into a new buffer
    int32_t read_back[4] = {0};
    dram.read_bytes(reinterpret_cast<uint8_t*>(read_back), 0x1000, sizeof(read_back));

    // 3. Verify exact byte matching
    for (int i = 0; i < 4; ++i) {
        assert(read_back[i] == original_data[i]);
    }

    std::cout << "SUCCESS: 16MB Host DRAM Memory Allocator Verified!" << std::endl;
    return 0;
}