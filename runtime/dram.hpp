#ifndef DRAM_HPP
#define DRAM_HPP

#include <cstdint>
#include <vector>
#include <stdexcept>
#include <cstring> 

class HostDRAM {
private:
    static constexpr size_t DRAM_SIZE_BYTES = 16 * 1024 * 1024;
    std::vector<uint8_t> memory_space;

public: 
    HostDRAM() : memory_space(DRAM_SIZE_BYTES, 0) {}
    uint8_t* get_ptr(uint32_t dram_addr) {
        if (dram_addr >= DRAM_SIZE_BYTES) {
            throw std::out_of_range("HostDRAM access out of bounds!");
        }
        return &memory_space[dram_addr]; 
    }

    void write_bytes(uint32_t dram_addr, const uint8_t* src, size_t num_bytes) {
        if (dram_addr + num_bytes > DRAM_SIZE_BYTES) {
            throw std::out_of_range("DRAM write overflows memory size!");
        }
        std::memcpy(&memory_space[dram_addr], src, num_bytes);
    }

    void read_bytes(uint8_t* dest, uint32_t dram_addr, size_t num_bytes) {
        if (dram_addr + num_bytes > DRAM_SIZE_BYTES) {
            throw std::out_of_range("DRAM read overflows memory size!");
        }
        std::memcpy(dest, &memory_space[dram_addr], num_bytes);
    }

    void reset() {
        std::fill(memory_space.begin(), memory_space.end(), 0);
    }
};

#endif