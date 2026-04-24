#pragma once

#include "utils.hpp"

#include <functional>
#include <memory>
#include <map>
#include <string>
#include <optional>

#include <libdwarf.h>
#include <dwarf.h>
#include <iostream>
#include <csignal>

#include <sys/reg.h>

struct RegMap {
    int dwarfRegNum;
    int processorRegNum;
    std::string regName;
};

const RegMap RegMapTable[] = {
    {0, RAX, "rax"},
    {1, RDX, "rdx"},
    {2, RCX, "rcx"},
    {3, RBX, "rbx"},
    {4, RSI, "rsi"},
    {5, RDI, "rdi"},
    {6, RBP, "rbp"},
    {7, RSP, "rsp"},
    {8, R8, "r8"},
    {9, R9, "r9"},
    {10, R10, "r10"},
    {11, R11, "r11"},
    {12, R12, "r12"},
    {13, R13, "r13"},
    {14, R14, "r14"},
    {15, R15, "r15"},
    {16, RIP, "rip"},
};

#define REG_MAP_TABLE_SIZE (sizeof(RegMapTable) / sizeof(RegMap))

namespace dwarf {
    class Line {
        protected:
            Dwarf_Line _line;
            std::optional<Dwarf_Addr> _cachedAddress;
            std::optional<std::string> _cachedFileName;
            std::optional<Dwarf_Unsigned> _cachedLineNumber;

        public:
            explicit Line(Dwarf_Line line) : _line(line) {}
            ~Line();

            Dwarf_Line getLine();
            std::optional<Address> getAddress();
            std::optional<std::string> getFileName();
            std::optional<std::size_t> getLineNumber();
    };

    class Die;


    class Cie {
        protected:
            Dwarf_Cie _cie;

        public:
            explicit Cie(Dwarf_Cie cie) : _cie(cie) {
            }
            ~Cie() = default;

            Dwarf_Cie getCie() const { return this->_cie; }
    };

    class Fde {
        protected:
            Dwarf_Fde _fde;
            Dwarf_Addr _lowPC;
            Dwarf_Addr _highPC;
            std::optional<Dwarf_Addr> _returnAddress;
            Dwarf_Unsigned _funcLen;

            std::shared_ptr<Cie> _cie;

        public:
            explicit Fde(Dwarf_Fde fde, Dwarf_Cie cie) : _fde(fde), _cie(std::make_shared<Cie>(cie)) {
                dwarf_get_fde_range(this->_fde, &this->_lowPC, &this->_funcLen, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
                _highPC = this->_lowPC + this->_funcLen;

            }
            ~Fde() = default;

            std::shared_ptr<Cie> getCie() const { return this->_cie; }
            Dwarf_Fde getFde() const { return this->_fde; }
            Address getLowPC() const { return this->_lowPC; }
            Address getHighPC() const { return this->_highPC; }
            std::optional<Address> getReturnAddress();
            bool containsAddress(Dwarf_Addr address) const {
                return address >= this->_lowPC && address < this->_highPC;
            }
    };

    class Die {
        protected:
            Dwarf_Die _die;
            std::optional<Dwarf_Half> _cachedTag;
            std::optional<std::string_view> _cachedTagName;
            std::optional<Dwarf_Addr> _cachedLowPC;

        public:
            explicit Die(Dwarf_Die die) : _die(die) {}
            virtual ~Die();

            Dwarf_Die getDie() const { return this->_die; }

            std::optional<std::uint16_t> getTag();
            std::optional<std::string_view> getTagName();
            std::optional<Address> getLowPC();
    };

    class DieVariable : public Die {
        private:
            std::optional<std::string_view> _cachedName;
            std::optional<std::uint64_t> _cachedVariableTypeOffset;

        public:
            explicit DieVariable(Dwarf_Die die) : Die(die) {}
            ~DieVariable() = default;

            std::optional<std::string_view> getName();
    };

    class DieSubprogram : public Die {
        private:
            std::optional<std::string> _cachedName;
            std::optional<Dwarf_Addr> _cachedHighPC;

        public:
            explicit DieSubprogram(Dwarf_Die die) : Die(die) {}
            ~DieSubprogram() = default;

            std::optional<std::string_view> getName();
            std::optional<Address> getHighPC();
    };

    class DieRoot : public Die {
        public:
            explicit DieRoot() : Die(Dwarf_Die()) {}
            ~DieRoot() = default;
    };



    class DieFactory {
        private:
            static std::map<Dwarf_Half, std::function<std::shared_ptr<Die>(Dwarf_Die)>> _tagMap;

        public:
            static std::shared_ptr<Die> createDie(Dwarf_Die die);
    };
}
