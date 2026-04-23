#pragma once

#include "elf/Segment.hpp"
#include "elf/Section.hpp"
#include "elf/Symbol.hpp"

#include <ranges>
#include <filesystem>
#include <functional>
#include <vector>
#include <span>
#include <map>

#include <elf.h>

namespace ELF {
    class ElfFile {
        private:
            std::filesystem::path _path;
            int _fd;
            char *_buffer;

            Header _header;

            std::vector<Segment> _segments;
            std::vector<Section> _sections;
            std::vector<Symbol> _symbols;
            std::vector<DynSymbol> _dynSymbols;

            void _parseSymbols();
            void _parseDynSymbols();
            void _parsePrograms();
            void _parseSections();

        public:
            explicit ElfFile(std::filesystem::path path);
            ~ElfFile();

            std::filesystem::path getPath() const { return _path; }

            std::span<const Segment> getSegments() const { return _segments; }
            std::span<const Section> getSections() const { return _sections; }
            std::span<const Symbol> getSymbols() const { return _symbols; }
            std::span<const DynSymbol> getDynSymbols() const { return _dynSymbols; }

            auto getProgramByType(Elf64_Word type) {
                auto data = this->_segments | std::views::filter([type](const Segment& segment) {
                        return segment.getHeader()->p_type == type;
                        });
                return data;
            }

            std::optional<Section> getSectionByName(std::string name) {
                return ELF::getSectionByName(_header, name);
            }

            std::optional<Section> getSectionByType(Elf64_Word type) {
                return ELF::getSectionByType(_header, type);
            }
    };
}
