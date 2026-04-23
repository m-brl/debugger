#include "elf/ElfFile.hpp"

#include <system_error>
#include <fstream>

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

namespace ELF {
    void ElfFile::_parseSymbols() {
        std::optional<Section> symtab = ELF::getSectionByName(_header, ".symtab");
        if (!symtab.has_value()) return;

        Elf64_Shdr *symtabHeader = symtab->getHeader();
        _symbols.reserve(symtabHeader->sh_size / sizeof(Elf64_Sym));
        for (Elf64_Off offset = 0; offset < symtabHeader->sh_size; offset += sizeof(Elf64_Sym)) {
            Elf64_Sym *sym = reinterpret_cast<Elf64_Sym *>(_header.buffer + symtabHeader->sh_offset + offset);
            _symbols.emplace_back(_header, sym);
        }
    }

    void ElfFile::_parseDynSymbols() {
        std::optional<Section> dynsym = ELF::getSectionByName(_header, ".dynsym");
        if (!dynsym.has_value()) return;

        Elf64_Shdr *dynsymHeader = dynsym->getHeader();
        _dynSymbols.reserve(dynsymHeader->sh_size / sizeof(Elf64_Sym));
        for (Elf64_Off offset = 0; offset < dynsymHeader->sh_size; offset += sizeof(Elf64_Sym)) {
            Elf64_Sym *sym = reinterpret_cast<Elf64_Sym *>(_header.buffer + dynsymHeader->sh_offset + offset);
            _dynSymbols.emplace_back(_header, sym, offset / sizeof(Elf64_Sym));
        }

        std::optional<Section> relaplt = ELF::getSectionByName(_header, ".rela.plt");
        if (!relaplt.has_value()) return;

        int relocationIndex = 0;
        Elf64_Shdr *relapltHeader = relaplt->getHeader();
        for (Elf64_Off offset = 0; offset < relapltHeader->sh_size; offset += sizeof(Elf64_Rela), relocationIndex++) {
            Elf64_Rela *rela = reinterpret_cast<Elf64_Rela *>(_header.buffer + relapltHeader->sh_offset + offset);
            int symIndex = ELF64_R_SYM(rela->r_info);
            ELF::DynSymbol& dynSymbol = _dynSymbols[symIndex];
            dynSymbol._relocationIndex = relocationIndex;
            dynSymbol._rela = rela;
        }
    }
/*
        Elf64_Off offset = 0;
        while (offset < symtab.value().header->sh_size) {
            this->_symbols.emplace_back(symtab.header->sh_offset + offset, reinterpret_cast<Elf64_Sym *>(this->_ehdr.buffer + symtab.header->sh_offset + offset), symtab);
            offset += sizeof(Elf64_Sym);
        }

        try {
            Section dynsym = this->getSectionByType(SHT_DYNSYM);
            Section reladyn = this->getSectionByName(".rela.plt");
            Section plt = this->getSectionByName(".plt");

            int relocationIndex = 0;
            for (Elf64_Off relOffset = 0; relOffset < reladyn.header->sh_size; relOffset += sizeof(Elf64_Rela), relocationIndex++) {
                Elf64_Rela *rela = reinterpret_cast<Elf64_Rela *>(this->_ehdr.buffer + reladyn.header->sh_offset + relOffset);
                int symIndex = ELF64_R_SYM(rela->r_info);
                int symOffset = dynsym.header->sh_offset + (symIndex * sizeof(Elf64_Sym));
                Elf64_Sym *sym = reinterpret_cast<Elf64_Sym *>(this->_ehdr.buffer + dynsym.header->sh_offset + (symIndex * sizeof(Elf64_Sym)));

                Symbol symbol(*this, symOffset, sym, dynsym);
                this->_symbols.push_back(symbol);
            }
        } catch (const std::runtime_error& e) {
            return;
        }
    }*/

    void ElfFile::_parsePrograms() {
        _segments.reserve(this->_header.elf64_ehdr->e_phnum);
        for (int segmentIndex = 0; segmentIndex < this->_header.elf64_ehdr->e_phnum; segmentIndex++) {
            Elf64_Off segmentHeaderOffset = segmentIndex * _header.elf64_ehdr->e_phentsize;
            Elf64_Phdr *segmentHeader = reinterpret_cast<Elf64_Phdr *>(_header.buffer + _header.elf64_ehdr->e_phoff + segmentHeaderOffset);

            _segments.emplace_back(_header, segmentHeader);
        }
    }

    void ElfFile::_parseSections() {
        _sections.reserve(this->_header.elf64_ehdr->e_shnum);
        for (int sectionIndex = 0; sectionIndex < this->_header.elf64_ehdr->e_shnum; sectionIndex++) {
            Elf64_Off sectionHeaderOffset = sectionIndex * _header.elf64_ehdr->e_shentsize;
            Elf64_Shdr *sectionHeader = reinterpret_cast<Elf64_Shdr *>(_header.buffer + _header.elf64_ehdr->e_shoff + sectionHeaderOffset);

            _sections.emplace_back(_header, sectionHeader);
        }
    }

    ElfFile::ElfFile(std::filesystem::path path) : _path(path) {
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("File not found");
        }

        struct stat fileStat;
        if (stat(path.c_str(), &fileStat) < 0) throw std::system_error(errno, std::generic_category());

        _fd = open(this->_path.c_str(), O_RDONLY);
        if (_fd < 0) throw std::system_error(errno, std::generic_category());

        _buffer = static_cast<char *>(mmap(nullptr, fileStat.st_size, PROT_READ, MAP_PRIVATE, _fd, 0));
        if (_buffer == MAP_FAILED) throw std::system_error(errno, std::generic_category());

        _header.buffer = _buffer;

        this->_parsePrograms();
        this->_parseSections();
        this->_parseSymbols();
        this->_parseDynSymbols();
    }

    ElfFile::~ElfFile() {
        munmap(this->_buffer, sizeof(Elf64_Ehdr));
        close(this->_fd);
    }
}
