#include "ExecutableFile.hpp"
#include "constant.h"
#include "utils/Tree.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ranges>
#include <fstream>
#include <iostream>
#include <dwarf.h>
#include <dwarf.h>
#include <libdwarf.h>

ExecutableFile::ExecutableFile(std::filesystem::path path) : _path(path), _fd(-1), _fileStat(), _elfFile(std::make_shared<ELF::ElfFile>(path)), _dwarfFile(std::make_shared<dwarf::DwarfFile>(path)) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("File not found");
    }

}

ExecutableFile::~ExecutableFile() {
}
