#include <iostream>
#include <cassert>
#include <cstdint>
#include "hardware/sram.hpp"

int main() {
    ScratchpadSRAM sram;

    uint8_t* ptr_a = sram.get_ptr(0, 100);
    ptr_a[0] = 42;
    ptr_a[1] = 99;

    uint8_t* check_a = sram.get_ptr(0, 100);
    assert(check_a[0] == 42);
    assert(check_a[1] == 99);

    uint8_t* check_b = sram.get_ptr(1, 100);
    assert(check_b[0] == 0);

    sram.reset();
    assert(sram.get_ptr(0, 100)[0] == 0);

    std::cout << "SUCCESS: SRAM Dual-Bank Memory Verified!" << std::endl;
    return 0;
}