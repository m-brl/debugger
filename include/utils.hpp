#pragma once

#include <string>
#include <vector>
#include <functional>

std::vector<std::string> split(const std::string& str, char delimiter);

#define SWAP_ENDIAN_16(x) __builtin_bswap16(x)
#define SWAP_ENDIAN_32(x) __builtin_bswap32(x)
#define SWAP_ENDIAN_64(x) __builtin_bswap64(x)

template<typename T, typename F>
std::vector<std::string> text(const std::vector<T>& data, F toString) {
    std::vector<std::string> result;
    result.reserve(data.size());
    for (auto& item : data) {
        result.push_back(toString(item));
    }
    return result;
}
