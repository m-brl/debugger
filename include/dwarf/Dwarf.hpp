#pragma once

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
    {15, 15, "r15"},
};

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

            Dwarf_Line getLine() { return this->_line; }
            Dwarf_Addr getAddress();
            std::string getFileName();
            Dwarf_Unsigned getLineNumber();
    };

    class Die;

    class Cie {
        protected:
            Dwarf_Cie _cie;

        public:
            explicit Cie(Dwarf_Cie cie) : _cie(cie) {}
            ~Cie() = default;

            Dwarf_Cie getCie() const { return this->_cie; }
    };

    class Fde {
        protected:
            Dwarf_Fde _fde;
            Dwarf_Addr _lowPC;
            Dwarf_Addr _highPC;
            Dwarf_Unsigned _funcLen;

        public:
            explicit Fde(Dwarf_Fde fde) : _fde(fde) {
                dwarf_get_fde_range(this->_fde, &this->_lowPC, &this->_funcLen, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
                _highPC = this->_lowPC + this->_funcLen;
            }
            ~Fde() = default;

            Dwarf_Fde getFde() const { return this->_fde; }
            Dwarf_Addr getLowPC() const { return this->_lowPC; }
            Dwarf_Addr getHighPC() const { return this->_highPC; }
            bool containsAddress(Dwarf_Addr address) const {
                return address >= this->_lowPC && address < this->_highPC;
            }
    };

    class Die {
        protected:
            Dwarf_Die _die;
            std::optional<Dwarf_Half> _cachedTag;
            std::optional<Dwarf_Addr> _cachedLowPC;
            std::optional<Dwarf_Addr> _cachedHighPC;

        public:
            explicit Die(Dwarf_Die die) : _die(die) {}
            virtual ~Die();

            Dwarf_Die getDie() const { return this->_die; }

            std::string getTagName();
            Dwarf_Addr getLowPC();
    };

    class DieRoot : public Die {
        public:
            explicit DieRoot() : Die(Dwarf_Die()) {}
            ~DieRoot() = default;
    };

    class DieSubprogram : public Die {
        private:
            std::optional<std::string> _cachedSubprogramName;
            std::optional<Dwarf_Addr> _cachedHighPC;

        public:
            explicit DieSubprogram(Dwarf_Die die) : Die(die) {}
            ~DieSubprogram() = default;

            std::string getSubprogramName();
            Dwarf_Addr getHighPC();
    };



    class DieFactory {
        private:
            static std::map<Dwarf_Half, std::function<std::shared_ptr<Die>(Dwarf_Die)>> _tagMap;

        public:
            static std::shared_ptr<Die> createDie(Dwarf_Die die);
    };
}
