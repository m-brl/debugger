#pragma once

#include "elf/Header.hpp"

#include <string>
#include <string_view>
#include <optional>

#include <elf.h>

namespace ELF {
    class Section {
        private:
            Header _elfHeader;

            Elf64_Shdr *_header;
            char *_content;

            std::optional<std::string_view> _cachedName;

        public:
            Section(Header elfHeader, Elf64_Off headerOff) : _elfHeader(elfHeader) {
                _header = reinterpret_cast<Elf64_Shdr *>(_elfHeader.buffer + headerOff);
                _content = _elfHeader.buffer + _header->sh_offset;
            }
            Section(Header elfHeader, Elf64_Shdr *header) : _elfHeader(elfHeader), _header(header) {
                _content = _elfHeader.buffer + _header->sh_offset;
            }
            ~Section() = default;

            std::optional<std::string_view> getName() {
                if (this->_cachedName.has_value()) {
                    return this->_cachedName.value();
                }

                auto shstrtabHeader = reinterpret_cast<Elf64_Shdr *>(_elfHeader.buffer + _elfHeader.elf64_ehdr->e_shoff + (_elfHeader.elf64_ehdr->e_shstrndx * _elfHeader.elf64_ehdr->e_shentsize));
                auto shstrtab = _elfHeader.buffer + shstrtabHeader->sh_offset;
                std::string_view name = std::string_view(shstrtab + _header->sh_name);
                _cachedName = name;
                return name;
            }

            Elf64_Shdr *getHeader() const { return _header; }
            char *getContent() const { return _content; }
    };


    static std::optional<Section> getSectionByName(Header header, std::string name) {
        auto shstrtabHeader = reinterpret_cast<Elf64_Shdr *>(header.buffer + header.elf64_ehdr->e_shoff + (header.elf64_ehdr->e_shstrndx * header.elf64_ehdr->e_shentsize));
        auto shstrtab = header.buffer + shstrtabHeader->sh_offset;

        for (int sectionIndex = 0; sectionIndex < header.elf64_ehdr->e_shnum; sectionIndex++) {
            Elf64_Off sectionHeaderOffset = sectionIndex * header.elf64_ehdr->e_shentsize;
            Elf64_Shdr *sectionHeader = reinterpret_cast<Elf64_Shdr *>(header.buffer + header.elf64_ehdr->e_shoff + sectionHeaderOffset);
            char *sectionName = shstrtab + sectionHeader->sh_name;

            if (name == sectionName) {
                return Section(header, sectionHeader);
            }
        }
        return std::nullopt;
    }

    static std::optional<Section> getSectionByType(Header header, Elf64_Word type) {
        for (int sectionIndex = 0; sectionIndex < header.elf64_ehdr->e_shnum; sectionIndex++) {
            Elf64_Off sectionHeaderOffset = sectionIndex * header.elf64_ehdr->e_shentsize;
            Elf64_Shdr *sectionHeader = reinterpret_cast<Elf64_Shdr *>(header.buffer + header.elf64_ehdr->e_shoff + sectionHeaderOffset);

            if (sectionHeader->sh_type == type) {
                return Section(header, sectionHeader);
            }
        }
        return std::nullopt;
    }
}
