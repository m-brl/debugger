#pragma once

#include "Dwarf.hpp"
#include "SourceFile.hpp"
#include "utils/Tree.hpp"

#include <iostream>
#include <format>
#include <elf.h>
#include <exception>
#include <filesystem>
#include <memory>
#include <libdwarf.h>
#include <sys/stat.h>
#include <vector>
#include <ranges>

namespace ELF {
    union Elf_Ehdr {
        char *buffer;
        Elf64_Ehdr *elf64_ehdr;
    };

    struct ProgramHeader {
        Elf64_Off headerOff;
        Elf64_Phdr *header;
        Elf64_Off contentOff;
        char *content;
    };

    struct SectionHeader {
        Elf64_Off headerOff;
        Elf64_Shdr *header;
        Elf64_Off contentOff;
        char *content;
    };

    struct Symbol {
        Elf64_Off offset;
        Elf64_Sym *sym;
        SectionHeader section;
        Elf64_Addr relocatedAddress;
    };

    class ExecutableFile {
    private:
        std::filesystem::path _path;
        int _fd;
        struct stat _fileStat;
        Elf_Ehdr _ehdr;

        std::vector<ProgramHeader> _programs;
        std::vector<SectionHeader> _sections;
        std::vector<Symbol> _symbols;

        Dwarf_Debug _dw_dbg;
        Tree<std::shared_ptr<dwarf::Die>> _debugTree;
        std::vector<std::shared_ptr<dwarf::Line>> _debugLines;
        std::vector<std::shared_ptr<dwarf::Cie>> _debugCies;
        Dwarf_Cie *_debugCiesRaw;
        std::vector<std::shared_ptr<dwarf::Fde>> _debugFdes;
        Dwarf_Fde *_debugFdesRaw;
        std::vector<std::shared_ptr<dwarf::Cie>> _debugHeCies;
        Dwarf_Cie *_debugHeCiesRaw;
        std::vector<std::shared_ptr<dwarf::Fde>> _debugHeFdes;
        Dwarf_Fde *_debugHeFdesRaw;

        std::map<std::string, std::shared_ptr<SourceFile>> _sourceFiles;

        // Dwarf parsing
        void _readLines(Dwarf_Die cu_die);
        void _readCiesAndFdes();
        void _readCU();
        void _parseDebug();

        // ELF parsing
        void _parseSymbols();
        void _parsePrograms();
        void _parseSections();
        void _parseFile();
        void _openFile();

    public:
        explicit ExecutableFile(std::filesystem::path path);
        ~ExecutableFile();

        std::filesystem::path getPath() const { return _path; }

        // ELF
        std::vector<Symbol> getSymbols() const { return _symbols; }

        auto getProgramByType(Elf64_Word type) {
            auto data = this->_programs | std::views::filter([type](const ProgramHeader& program) {
                    return program.header->p_type == type;
                    });
            if (data.empty()) {
                throw std::runtime_error("Program not found");
            }
            return data;
        }

        SectionHeader getSectionByName(std::string name);
        SectionHeader getSectionByType(Elf64_Word type);
        Symbol getSymbolByOffset(Elf64_Off offset);
        std::string getSymbolName(Symbol symbol);
        Symbol getSymbolByName(std::string name);

        // Dwarf
        auto getDebugCies() const { return _debugCies; }
        auto getDebugFdes() const { return _debugFdes; }
        auto getDebugHeCies() const { return _debugHeCies; }
        auto getDebugHeFdes() const { return _debugHeFdes; }

        Dwarf_Addr getSymbolAddress(std::string name);
        Dwarf_Debug getDwarfDebug() const { return _dw_dbg; }
        std::shared_ptr<dwarf::Fde> getFdeAtPc(long rip);
        std::shared_ptr<dwarf::Die> getDieAtPc(long rip);


        LineLocation getLineLocation(Dwarf_Addr address);
        std::vector<std::shared_ptr<dwarf::Line>> getDebugLines() const;
    };
} // namespace ELF
