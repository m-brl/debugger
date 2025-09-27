#pragma once

#include <functional>
#include <memory>
#include <map>
#include <string>
#include <optional>

#include <libdwarf.h>
#include <dwarf.h>

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

    class Die {
        protected:
            Dwarf_Die _die;
            std::optional<Dwarf_Half> _cachedTag;
            std::optional<Dwarf_Addr> _cachedLowPC;

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
        public:
            explicit DieSubprogram(Dwarf_Die die) : Die(die) {}
            ~DieSubprogram() = default;

            std::string getSubprogramName();
    };



    class DieFactory {
        private:
            static std::map<Dwarf_Half, std::function<std::shared_ptr<Die>(Dwarf_Die)>> _tagMap;

        public:
            static std::shared_ptr<Die> createDie(Dwarf_Die die);
    };
}
