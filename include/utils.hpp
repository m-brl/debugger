#pragma once

#include <string>
#include <vector>

std::vector<std::string> split(const std::string& str, char delimiter);

#define SWAP_ENDIAN_16(x)   __builtin_bswap16(x)
#define SWAP_ENDIAN_32(x)   __builtin_bswap32(x)
#define SWAP_ENDIAN_64(x)   __builtin_bswap64(x)
