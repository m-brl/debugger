#pragma once

#include "dwarf/Dwarf.hpp"
#include "SourceFile.hpp"
#include "utils.hpp"
#include "utils/Tree.hpp"

#include <filesystem>

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

            std::vector<std::shared_ptr<Cie>> _debugCies;
            Dwarf_Cie *_debugCiesRaw;
            std::vector<std::shared_ptr<Fde>> _debugFdes;
            Dwarf_Fde *_debugFdesRaw;
            std::vector<std::shared_ptr<Cie>> _debugHeCies;
            Dwarf_Cie *_debugHeCiesRaw;
            std::vector<std::shared_ptr<Fde>> _debugHeFdes;
            Dwarf_Fde *_debugHeFdesRaw;

            void _loadFrameDescriptionEntries();
            void _loadSourceLines(Dwarf_Die cu_die);
            void _loadCompilationUnits();
            void _loadFile();

        public:
            explicit DwarfFile(std::filesystem::path path);
            ~DwarfFile();

            auto getDebugCies() const { return _debugCies; }
            auto getDebugFdes() const { return _debugFdes; }
            auto getDebugHeCies() const { return _debugHeCies; }
            auto getDebugHeFdes() const { return _debugHeFdes; }

            std::optional<Address> getSymbolAddress(std::string name) {
                auto symbol = searchTree<std::shared_ptr<Die>>(_debugTree.value(), [name](std::shared_ptr<Die> diePtr) {
auto dieSubprogram = std::dynamic_pointer_cast<DieSubprogram>(diePtr);
                        if (dieSubprogram) {
                            return dieSubprogram->getSubprogramName() == name;
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

        std::optional<std::shared_ptr<Die>> getDieAtPc(Address pc) {
            auto die = searchTree<std::shared_ptr<Die>>(_debugTree.value(), [pc](std::shared_ptr<Die> diePtr) {
                    auto dieSubprogram = std::dynamic_pointer_cast<DieSubprogram>(diePtr);
                        if (!dieSubprogram) return false;
                        auto lowPC = dieSubprogram->getLowPC();
                        auto highPC = dieSubprogram->getHighPC();
                        return lowPC <= pc && pc < highPC;
                    });
            if (!die.has_value()) return std::nullopt;
            return die.value();
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
                lineLocation.lineNumber = matchedLine->getLineNumber();
                std::string filename = matchedLine->getFileName();
                if (_sourceFiles.contains(filename))
                    lineLocation.file = _sourceFiles[filename];
                return lineLocation;
            }
            return std::nullopt;
        }

        std::vector<std::shared_ptr<Line>> getDebugLines() const { return _debugLines; }
    };
}
