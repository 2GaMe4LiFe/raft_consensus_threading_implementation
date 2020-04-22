#pragma once

#include <iostream>

namespace utils {
    std::uint64_t constexpr mix(char m, std::uint64_t s) {
        return ((s<<7) + ~(s>>3)) + ~m;
    }

    uint64_t constexpr hash(const char* m) {
        return (*m) ? mix(*m,hash(m+1)) : 0;
    }
}