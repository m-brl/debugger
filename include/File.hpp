#pragma once

#include <elf.h>
#include <exception>
#include <filesystem>
#include <libdwarf.h>
#include <vector>
#include <sys/stat.h>

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
    public:
        class FileNotFound : public std::exception {
        private:
            std::string _message;

        public:
            FileNotFound(std::string message) : _message(std::move(message)) {
            }

            [[nodiscard]] const char *what() const noexcept override {
                return this->_message.c_str();
            }
        };

    private:
        std::filesystem::path _path;
        int _fd;
        struct stat _fileStat;
        Elf_Ehdr _ehdr;

        std::vector<Section> _sections;
        std::vector<Symbol> _symbols;

        Tree<Dwarf_Die> _debugTree;

        void _parseSymbols();
        void _parseSections();
        void _parseDebug();
        void _parseFile();
        void _openFile();

    public:
        explicit File(std::filesystem::path path);
        ~File() = default;

        Section getSectionByName(std::string name);
        Symbol getSymbolByOffset(Elf64_Off offset);
    };
} // namespace ELF
