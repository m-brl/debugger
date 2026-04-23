#pragma once

#include <elf.h>

namespace ELF {
    union Header {
        char *buffer;
        Elf64_Ehdr *elf64_ehdr;
    };
}
