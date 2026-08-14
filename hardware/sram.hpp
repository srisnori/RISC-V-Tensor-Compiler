#ifndef SRAM_HPP
#define SRAM_HPP
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <algorithm>

class ScratchpadSRAM {
private:
    static constexpr size_t BANK_SIZE_BYTES = 32768;
    std::vector<uint8_t> bank_A;
    std::vector<uint8_t> bank_B;

public: 
    ScratchpadSRAM() : bank_A(BANK_SIZE_BYTES, 0), bank_B(BANK_SIZE_BYTES, 0) {}

    void reset() {
        std::fill(bank_A.begin(), bank_A.end(), 0);
        std::fill(bank_B.begin(), bank_B.end(), 0);
    }

    uint8_t* get_ptr(uint8_t bank, uint16_t offset) {
        if (offset >= BANK_SIZE_BYTES) {
            throw std::out_of_range("SRAM offset exceeds 32KB bank limit");
        }
        return (bank == 0) ? &bank_A[offset] : &bank_B[offset];
    }
};

#endif