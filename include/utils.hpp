#pragma once

#include <string>
#include <vector>
#include <functional>
#include <span>
#include <cstdint>

#define SWAP_ENDIAN_16(x) __builtin_bswap16(x)
#define SWAP_ENDIAN_32(x) __builtin_bswap32(x)
#define SWAP_ENDIAN_64(x) __builtin_bswap64(x)

using Address = std::uintptr_t;

std::vector<std::string> split(const std::string& str, char delimiter);

template<typename T, typename F>
std::vector<std::string> text(const std::vector<T>& data, F toString) {
    std::vector<std::string> result;
    result.reserve(data.size());
    for (auto& item : data) {
        result.push_back(toString(item));
    }
    return result;
}

template<typename T, typename F>
std::vector<std::string> text(const std::span<T>& data, F toString) {
    std::vector<std::string> result;
    result.reserve(data.size());
    for (auto& item : data) {
        result.push_back(toString(item));
    }
    return result;
}
