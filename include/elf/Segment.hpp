#pragma once

#include "elf/Header.hpp"

#include <elf.h>

namespace ELF {
    class Segment {
        private:
            Header _elfHeader;

            Elf64_Phdr *_header;
            char *_content;

        public:
            Segment(Header elfHeader, Elf64_Off headerOff) : _elfHeader(elfHeader) {
                _header = reinterpret_cast<Elf64_Phdr *>(_elfHeader.buffer + headerOff);
                _content = _elfHeader.buffer + _header->p_offset;
            }
            Segment(Header elfHeader, Elf64_Phdr *header) : _elfHeader(elfHeader), _header(header) {
                _content = _elfHeader.buffer + _header->p_offset;
            }
            ~Segment() = default;

            Elf64_Phdr *getHeader() const { return _header; }
            char *getContent() const { return _content; }
    };
};
