#pragma once

#include "elf/Header.hpp"
#include "elf/Section.hpp"

#include <string_view>
#include <optional>

namespace ELF {
    class Symbol {
        private:
            Header _header;

            Elf64_Sym *_sym;

            std::optional<std::string_view> _cachedName;

        public:
            Symbol(Header header, Elf64_Sym *sym) : _header(header), _sym(sym) {}
            ~Symbol() = default;

            std::optional<std::string_view> getName() {
                if (this->_cachedName.has_value()) {
                    return this->_cachedName.value();
                }

                auto strtab = getSectionByName(this->_header, ".strtab");
                if (!strtab.has_value()) return std::nullopt;

                _cachedName = strtab->getContent() + this->_sym->st_name;
                return _cachedName.value();
            }

            Elf64_Sym *getSym() const { return _sym; }
    };

    class DynSymbol {
        private:
            friend class ElfFile;

            Header _header;

            Elf64_Sym *_sym;
            std::optional<int> _relocationIndex;
            std::optional<Elf64_Rela *> _rela;

            std::optional<std::string> _cachedName;

        public:
            DynSymbol(Header header, Elf64_Sym *sym) : _header(header), _sym(sym), _relocationIndex(-1) {}
            DynSymbol(Header header, Elf64_Sym *sym, int relocationIndex) : _header(header), _sym(sym), _relocationIndex(relocationIndex) {}
            ~DynSymbol() = default;

            std::optional<std::string> getName() {
                if (this->_cachedName.has_value()) {
                    return this->_cachedName.value();
                }

                auto dynstr = getSectionByName(this->_header, ".dynstr");
                if (!dynstr.has_value()) return std::nullopt;

                _cachedName = dynstr->getContent() + this->_sym->st_name;
                return _cachedName.value();
            }
    };
}
