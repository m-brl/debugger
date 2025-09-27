#pragma once

#include <elf.h>

union Elf_Ehdr {
  char *buffer;
  Elf32_Ehdr *elf32_ehdr;
  Elf64_Ehdr *elf64_ehdr;
};

#define SWAP_ENDIAN_16(x)   __builtin_bswap16(x)
#define SWAP_ENDIAN_32(x)   __builtin_bswap32(x)
#define SWAP_ENDIAN_64(x)   __builtin_bswap64(x)
