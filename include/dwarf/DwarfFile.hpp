#pragma once

#include "dwarf/Dwarf.hpp"
#include "SourceFile.hpp"
#include "utils.hpp"
#include "utils/Tree.hpp"

#include <filesystem>
#include <format>

#include <libdwarf.h>
#include <dwarf.h>

namespace dwarf {
    class DwarfFile {
        private:
            std::filesystem::path _path;
            Dwarf_Debug _dw_dbg;

            std::optional<Tree<std::shared_ptr<Die>>> _debugTree;

            std::vector<std::shared_ptr<Line>> _debugLines;
            std::map<std::string, std::shared_ptr<SourceFile>> _sourceFiles;

            std::vector<std::shared_ptr<Fde>> _debugFdes;
            Dwarf_Fde *_debugFdesRaw;
            std::vector<std::shared_ptr<Fde>> _debugHeFdes;
            Dwarf_Fde *_debugHeFdesRaw;

            void _loadFrameDescriptionEntries();
            void _loadSourceLines(Dwarf_Die cu_die);
            void _loadCompilationUnits();
            void _loadFile();

        public:
            explicit DwarfFile(std::filesystem::path path);
            ~DwarfFile();

            auto getDebugFdes() const { return _debugFdes; }
            auto getDebugHeFdes() const { return _debugHeFdes; }

            std::optional<Address> getSymbolAddress(std::string name) {
                auto symbol = searchTree<std::shared_ptr<Die>>(_debugTree.value(), [name](std::shared_ptr<Die> diePtr) {
                        auto dieSubprogram = std::dynamic_pointer_cast<DieSubprogram>(diePtr);
                        if (dieSubprogram) {
                        return dieSubprogram->getName() == name;
                        }
                        return false;
                        });
                if (!symbol.has_value()) return std::nullopt;
                return symbol.value()->getLowPC();
            }

        std::optional<std::shared_ptr<Fde>> getFdeAtPc(Address pc) {
            for (auto& fde: this->_debugFdes) {
                if (fde->containsAddress(pc)) {
                    return fde;
                }
            }
            for (auto& fde: this->_debugHeFdes) {
                if (fde->containsAddress(pc)) {
                    return fde;
                }
            }
            return std::nullopt;
        }

        std::optional<std::shared_ptr<Die>> getSubprogram(Address pc) {
            std::cerr << std::format("Searching for subprogram containing PC: 0x{:x}\n", pc);
            auto die = searchTree<std::shared_ptr<Die>>(_debugTree.value(), [pc](std::shared_ptr<Die> diePtr) {
                    auto dieSubprogram = std::dynamic_pointer_cast<DieSubprogram>(diePtr);
                        if (!dieSubprogram) return false;
                        auto lowPC = dieSubprogram->getLowPC();
                        auto highPC = dieSubprogram->getHighPC();
                        std::cerr << std::format("Checking DIE {}: lowPC=0x{:x}, highPC=0x{:x}\n", dieSubprogram->getName().value_or("unknown"), lowPC.value_or(0), highPC.value_or(0));
                        return lowPC <= pc && pc < highPC;
                    });
            if (!die.has_value()) return std::nullopt;
            return die.value();
        }

        std::vector<std::shared_ptr<Die>> getDiesByTag(Dwarf_Half tag) {
            std::vector<std::shared_ptr<Die>> dies;
            searchTree<std::shared_ptr<Die>>(_debugTree.value(), [&](std::shared_ptr<Die> diePtr) {
                if (diePtr->getTag() == tag) {
                    dies.push_back(diePtr);
                }
                return false;
            });
            return dies;
        }

        std::optional<LineLocation> getLineLocation(Address address) {
            std::shared_ptr<Line> matchedLine = nullptr;
            for (auto& line: _debugLines) {
                if (line->getAddress() > address) {
                    break;
                }
                matchedLine = line;
            }
            if (matchedLine) {
                LineLocation lineLocation{};
                lineLocation.lineNumber = matchedLine->getLineNumber().value();
                std::string filename = matchedLine->getFileName().value();
                if (_sourceFiles.contains(filename))
                    lineLocation.file = _sourceFiles[filename];
                return lineLocation;
            }
            return std::nullopt;
        }

        std::vector<std::shared_ptr<Line>> getDebugLines() const { return _debugLines; }
    };
}
