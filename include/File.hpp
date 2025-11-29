#pragma once

#include "Die.hpp"

#include <elf.h>
#include <exception>
#include <filesystem>
#include <libdwarf.h>
#include <sys/stat.h>
#include <vector>

template <typename T>
struct Tree {
    T data;
    std::vector<Tree<T>> children;
};

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
    };

    class File {
    private:
        std::filesystem::path _path;
        int _fd;
        struct stat _fileStat;
        Elf_Ehdr _ehdr;

        std::vector<Section> _sections;
        std::vector<Symbol> _symbols;

        std::vector<Tree<Die>> _debugTree;

        // Dwarf parsing
        void _readCU(Dwarf_Debug* dw_dbg);
        void _parseDebug();

        // Dwarf utils
        void _displayDieTree(Tree<Die> node, int depth);

        // ELF parsing
        void _parseSymbols();
        void _parseSections();
        void _parseFile();
        void _openFile();

    public:
        explicit File(std::filesystem::path path);
        ~File() = default;

        Section getSectionByName(std::string name);
        Symbol getSymbolByOffset(Elf64_Off offset);

        void displayDieTree();
    };
} // namespace ELF
