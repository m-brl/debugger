#pragma once

#include <string>
#include <elf.h>
#include <variant>

#include "utils.hpp"

namespace ELF {
  struct Section {
    std::variant<Elf64_Off, Elf32_Off> headerOff;
    std::variant<Elf64_Shdr*, Elf32_Shdr*> header;
    std::variant<Elf64_Off, Elf32_Off> contentOff;
    char *content;
  };
}
