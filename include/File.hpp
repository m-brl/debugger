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

namespace ELF {
    union Elf_Ehdr {
        char *buffer;
        Elf64_Ehdr *elf64_ehdr;
    };

    struct Section {
        Elf64_Off headerOff;
        Elf64_Shdr *header;
        Elf64_Off contentOff;
        char *content;
    };

    struct Symbol {
        Elf64_Off offset;
        Elf64_Sym *sym;
        Section section;
        Elf64_Addr relocatedAddress;
    };

    class ExecutableFile {
    private:
        std::filesystem::path _path;
        int _fd;
        struct stat _fileStat;
        Elf_Ehdr _ehdr;

        std::vector<Section> _sections;
        std::vector<Symbol> _symbols;

        Dwarf_Debug _dw_dbg;
        Tree<std::shared_ptr<dwarf::Die>> _debugTree;
        std::vector<std::shared_ptr<dwarf::Line>> _debugLines;
        std::vector<dwarf::Cie> _debugCies;
        Dwarf_Cie *_debugCiesRaw;
        std::vector<dwarf::Fde> _debugFdes;
        Dwarf_Fde *_debugFdesRaw;
        std::vector<dwarf::Cie> _debugHeCies;
        Dwarf_Cie *_debugHeCiesRaw;
        std::vector<dwarf::Fde> _debugHeFdes;
        Dwarf_Fde *_debugHeFdesRaw;

        std::map<std::string, std::shared_ptr<SourceFile>> _sourceFiles;

        // Dwarf parsing
        void _readLines(Dwarf_Die cu_die);
        void _readCiesAndFdes();
        void _readCU();
        void _parseDebug();

        // ELF parsing
        void _parseSymbols();
        void _parseSections();
        void _parseFile();
        void _openFile();

    public:
        explicit ExecutableFile(std::filesystem::path path);
        ~ExecutableFile();

        std::filesystem::path getPath() const { return _path; }

        // ELF
        std::vector<Symbol> getSymbols() const {
            std::cerr << "Getting symbols, count: " << _symbols.size() << std::endl;
            return _symbols; }
        Section getSectionByName(std::string name);
        Section getSectionByType(Elf64_Word type);
        Symbol getSymbolByOffset(Elf64_Off offset);
        std::string getSymbolName(Symbol symbol);
        Symbol getSymbolByName(std::string name);

        // Dwarf
        Dwarf_Addr getSymbolAddress(std::string name);
        Dwarf_Debug getDwarfDebug() const { return _dw_dbg; }
        dwarf::Fde getFdeAtPc(long rip);

        LineLocation getLineLocation(Dwarf_Addr address);
        std::vector<std::shared_ptr<dwarf::Line>> getDebugLines() const;
    };
} // namespace ELF
