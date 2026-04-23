#pragma once

#include "SourceFile.hpp"
#include "utils/Tree.hpp"

#include "dwarf/Dwarf.hpp"
#include "dwarf/DwarfFile.hpp"

#include "elf/ElfFile.hpp"

#include <iostream>
#include <format>
#include <exception>
#include <filesystem>
#include <memory>
#include <libdwarf.h>
#include <sys/stat.h>
#include <vector>
#include <ranges>

class ExecutableFile {
    private:
        std::filesystem::path _path;
        int _fd;
        struct stat _fileStat;

        std::shared_ptr<ELF::ElfFile> _elfFile;
        std::shared_ptr<dwarf::DwarfFile> _dwarfFile;

    public:
        explicit ExecutableFile(std::filesystem::path path);
        ~ExecutableFile();

        std::filesystem::path getPath() const { return _path; }
        std::shared_ptr<ELF::ElfFile> getElfFile() const { return _elfFile; }
        std::shared_ptr<dwarf::DwarfFile> getDwarfFile() const { return _dwarfFile; }
};
