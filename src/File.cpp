#include "File.hpp"
#include "constant.h"
#include "utils/Tree.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ranges>
#include <fstream>
#include <iostream>
#include <dwarf.h>
#include <dwarf.h>
#include <libdwarf.h>


namespace ELF {
    void ExecutableFile::_parseSymbols() {
        try {
            SectionHeader symtab = this->getSectionByType(SHT_SYMTAB);
            Elf64_Off offset = 0;
            while (offset < symtab.header->sh_size) {
                this->_symbols.emplace_back(symtab.header->sh_offset + offset, reinterpret_cast<Elf64_Sym *>(this->_ehdr.buffer + symtab.header->sh_offset + offset), symtab);
                offset += sizeof(Elf64_Sym);
            }
        } catch (const std::runtime_error& e) {
            return;
        }

        try {
            SectionHeader dynsym = this->getSectionByType(SHT_DYNSYM);
            SectionHeader reladyn = this->getSectionByName(".rela.plt");
            SectionHeader plt = this->getSectionByName(".plt");

            int relocationIndex = 0;
            for (Elf64_Off relOffset = 0; relOffset < reladyn.header->sh_size; relOffset += sizeof(Elf64_Rela), relocationIndex++) {
                Elf64_Rela *rela = reinterpret_cast<Elf64_Rela *>(this->_ehdr.buffer + reladyn.header->sh_offset + relOffset);
                int symIndex = ELF64_R_SYM(rela->r_info);
                int symOffset = dynsym.header->sh_offset + (symIndex * sizeof(Elf64_Sym));
                Elf64_Sym *sym = reinterpret_cast<Elf64_Sym *>(this->_ehdr.buffer + dynsym.header->sh_offset + (symIndex * sizeof(Elf64_Sym)));

                Symbol symbol(symOffset, sym, dynsym);
                symbol.relocatedAddress = plt.header->sh_addr + ((relocationIndex + 1) * 16);
                this->_symbols.push_back(symbol);
            }
        } catch (const std::runtime_error& e) {
            return;
        }
    }

    void ExecutableFile::_parsePrograms() {
        for (int programIndex = 0; programIndex < this->_ehdr.elf64_ehdr->e_phnum; programIndex++) {
            ProgramHeader program{};

            program.headerOff = this->_ehdr.elf64_ehdr->e_phoff + (programIndex * sizeof(Elf64_Phdr));
            program.header = reinterpret_cast<Elf64_Phdr *>(this->_ehdr.buffer + program.headerOff);
            program.contentOff = program.header->p_offset;
            program.content = this->_ehdr.buffer + program.contentOff;
            this->_programs.push_back(program);
        }
    }

    void ExecutableFile::_parseSections() {
        for (int sectionIndex = 0; sectionIndex < this->_ehdr.elf64_ehdr->e_shnum; sectionIndex++) {
            SectionHeader section{};
            section.headerOff = this->_ehdr.elf64_ehdr->e_shoff + (sectionIndex * sizeof(Elf64_Shdr));
            section.header = reinterpret_cast<Elf64_Shdr *>(this->_ehdr.buffer + section.headerOff);
            section.contentOff = section.header->sh_offset;
            section.content = this->_ehdr.buffer + section.contentOff;
            this->_sections.push_back(section);
        }
    }

    Tree<std::shared_ptr<dwarf::Die>> readDie(Dwarf_Debug& dw_dbg, Dwarf_Die root_die) {
        Dwarf_Error dw_error{};

        // Read child
        Dwarf_Bool dw_is_info = 1;
        Dwarf_Die child_die{};
        Dwarf_Die sibling_die{};
        std::vector<Tree<std::shared_ptr<dwarf::Die>>> children;

        int status = dwarf_child(root_die, &child_die, &dw_error);

        if (status != DW_DLV_OK) {
            return Tree(dwarf::DieFactory::createDie(root_die));
        }

        while (status == DW_DLV_OK) {
            children.push_back(readDie(dw_dbg, child_die));
            status = dwarf_siblingof_b(dw_dbg, child_die, dw_is_info, &sibling_die, &dw_error);
            child_die = sibling_die;
        }

        return Tree(dwarf::DieFactory::createDie(root_die), children);
    }

    void ExecutableFile::_readLines(Dwarf_Die cu_die) {
        Dwarf_Error dw_error{};

        Dwarf_Small table_count{};
        Dwarf_Unsigned version{};
        Dwarf_Line_Context line_context{};



        int status = dwarf_srclines_b(cu_die, &version, &table_count, &line_context, &dw_error);
        if (status != DW_DLV_OK) {
            return;
        }

        Dwarf_Line *_lines = nullptr;
        Dwarf_Signed line_count{};
        dwarf_srclines_from_linecontext(line_context, &_lines, &line_count, &dw_error);
        for (int i = 0; i < line_count; i++) {
            char *file_name = nullptr;
            dwarf_linesrc(_lines[i], &file_name, &dw_error);
            if (!_sourceFiles.contains(file_name)) {
                try {
                    _sourceFiles.emplace(file_name, std::make_shared<SourceFile>(std::filesystem::path(file_name)));
                } catch (const std::runtime_error& e) {
                    continue;
                }
            }
            _debugLines.push_back(std::make_shared<dwarf::Line>(_lines[i]));
        }
    }

    void ExecutableFile::_readCiesAndFdes() {
        Dwarf_Error dw_error{};
        Dwarf_Cie *cies = nullptr;
        Dwarf_Signed cie_count{};
        Dwarf_Fde *fdes = nullptr;
        Dwarf_Signed fde_count{};

        int status = dwarf_get_fde_list(this->_dw_dbg, &cies, &cie_count, &fdes, &fde_count, &dw_error);
        if (status == DW_DLV_ERROR) {
            const char *error_msg = dwarf_errmsg(dw_error);
            throw std::runtime_error("Failed to retrieve CIEs and FDEs from FRAME: " + std::string(error_msg));
        }

        for (int i = 0; i < cie_count; i++) {
            _debugCies.push_back(std::shared_ptr<dwarf::Cie>(new dwarf::Cie(cies[i])));
        }
        _debugCiesRaw = cies;
        for (int i = 0; i < fde_count; i++) {

            _debugFdes.push_back(std::shared_ptr<dwarf::Fde>(new dwarf::Fde(fdes[i])));
        }
        _debugFdesRaw = fdes;

        status = dwarf_get_fde_list_eh(this->_dw_dbg, &cies, &cie_count, &fdes, &fde_count, &dw_error);
        if (status == DW_DLV_ERROR) {
            const char *error_msg = dwarf_errmsg(dw_error);
            throw std::runtime_error("Failed to retrieve CIEs and FDEs from EH_FRAME: " + std::string(error_msg));
        }
        for (int i = 0; i < cie_count; i++) {
            _debugHeCies.push_back(std::shared_ptr<dwarf::Cie>(new dwarf::Cie(cies[i])));
        }
        _debugHeCiesRaw = cies;
        for (int i = 0; i < fde_count; i++) {
            _debugHeFdes.push_back(std::shared_ptr<dwarf::Fde>(new dwarf::Fde(fdes[i])));
        }
        _debugHeFdesRaw = fdes;
    }

    void ExecutableFile::_readCU() {
        Dwarf_Bool dw_is_info = 1;
        Dwarf_Die cu_die{};
        Dwarf_Unsigned dw_cu_header_length{};
        Dwarf_Half dw_version_stamp{};
        Dwarf_Off dw_abbrev_offset{};
        Dwarf_Half dw_address_size{};
        Dwarf_Half dw_length_size{};
        Dwarf_Half dw_extension_size{};
        Dwarf_Sig8 dw_type_signature{};
        Dwarf_Unsigned dw_typeoffset{};
        Dwarf_Unsigned dw_next_cu_header_offset{};
        Dwarf_Half dw_header_cu_type{};
        Dwarf_Error dw_error{};

        int status = dwarf_next_cu_header_e(
            _dw_dbg, dw_is_info, &cu_die, &dw_cu_header_length, &dw_version_stamp,
            &dw_abbrev_offset, &dw_address_size, &dw_length_size, &dw_extension_size,
            &dw_type_signature, &dw_typeoffset, &dw_next_cu_header_offset,
            &dw_header_cu_type, &dw_error);
        if (status != DW_DLV_OK) {
            return;
        }

        std::vector<Tree<std::shared_ptr<dwarf::Die>>> children;
        while (status == DW_DLV_OK) {
            auto childTree = readDie(_dw_dbg, cu_die);
            children.push_back(childTree);
            _readLines(cu_die);

            status = dwarf_next_cu_header_e(
                _dw_dbg, dw_is_info, &cu_die, &dw_cu_header_length, &dw_version_stamp,
                &dw_abbrev_offset, &dw_address_size, &dw_length_size,
                &dw_extension_size, &dw_type_signature, &dw_typeoffset,
                &dw_next_cu_header_offset, &dw_header_cu_type, &dw_error);
        }
        _debugTree = Tree<std::shared_ptr<dwarf::Die>>(std::make_shared<dwarf::DieRoot>(), children);
    }

    void ExecutableFile::_parseDebug() {
        lseek(this->_fd, 0, SEEK_SET);

        Dwarf_Handler dw_errhand{};
        Dwarf_Ptr dw_errarg{};
        Dwarf_Error dw_error{};

        if (dwarf_init_b(this->_fd, 0, dw_errhand, dw_errarg, &_dw_dbg, &dw_error) != DW_DLV_OK) {
            return;
        }
        _readCU();
        _readCiesAndFdes();
    }

    void ExecutableFile::_parseFile() {
        this->_parsePrograms();
        this->_parseSections();
        this->_parseSymbols();
        this->_parseDebug();
    }

    void ExecutableFile::_openFile() {
        this->_fd = open(this->_path.c_str(), O_RDONLY);
        fstat(this->_fd, &this->_fileStat);

        this->_ehdr.buffer = reinterpret_cast<char *>(mmap(
            nullptr, this->_fileStat.st_size, PROT_READ, MAP_PRIVATE, this->_fd, 0));
    }

    ExecutableFile::ExecutableFile(std::filesystem::path path) : _path(path), _fd(-1), _fileStat(), _ehdr({}) {
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("File not found");
        }
        this->_openFile();
        this->_parseFile();
    }

    ExecutableFile::~ExecutableFile() {
        // Need to clear the list to call destructor before dwarf_finish
        _debugTree.children.clear();
        _debugLines.clear();
        dwarf_finish(_dw_dbg);
    }

    Dwarf_Addr ExecutableFile::getSymbolAddress(std::string name) {
        try {

            auto symbol = searchTree<std::shared_ptr<dwarf::Die>>(_debugTree,
                [name](std::shared_ptr<dwarf::Die> diePtr) {
                    auto dieSubprogram = std::dynamic_pointer_cast<dwarf::DieSubprogram>(diePtr);
                    if (dieSubprogram) {
                        std::cout << "Checking symbol: " << dieSubprogram->getSubprogramName() << std::endl;
                        return dieSubprogram->getSubprogramName() == name;
                    } else {
                    std::cout << "Not a subprogram die." << std::endl;
                    }
                    return false;
                });
            return symbol->getLowPC();

        } catch (const std::runtime_error& e) {
            return 0;
        }
    }

    std::shared_ptr<dwarf::Fde> ExecutableFile::getFdeAtPc(long rip) {
        for (auto& fde: this->_debugFdes) {
            if (fde->containsAddress(rip)) {
                return fde;
            }
        }
        for (auto& fde: this->_debugHeFdes) {
            if (fde->containsAddress(rip)) {
                return fde;
            }
        }
        throw std::runtime_error(FMT("FDE not found at the address: 0x{:x}", rip));
    }

    std::shared_ptr<dwarf::Die> ExecutableFile::getDieAtPc(long rip) {
        try {
            auto die = searchTree<std::shared_ptr<dwarf::Die>>(_debugTree,
                [rip](std::shared_ptr<dwarf::Die> diePtr) {
                    auto dieSubprogram = std::dynamic_pointer_cast<dwarf::DieSubprogram>(diePtr);
                    if (!dieSubprogram)
                        return false;
                    auto lowPC = dieSubprogram->getLowPC();
                    auto highPC = dieSubprogram->getHighPC();
                    return lowPC <= rip && rip < highPC;
                });
            return die;
        } catch (const std::runtime_error& e) {
            throw std::runtime_error(FMT("DIE not found at the address: 0x{:x}", rip));
        }
    }

    /*auto ExecutableFile::getProgramByType(Elf64_Word type) {
        auto data = this->_programs | std::views::filter([type](const ProgramHeader& program) {
            return program.header->p_type == type;
        });
        if (data.empty()) {
            throw std::runtime_error("Program not found");
        }
        return data;
    }*/

    SectionHeader ExecutableFile::getSectionByName(std::string name) {
        for (auto& section: this->_sections) {
            char *sectionName =
                this->_sections[this->_ehdr.elf64_ehdr->e_shstrndx].content +
                section.header->sh_name;
            if (name == std::string(sectionName)) {
                return section;
            }
        }
        throw std::runtime_error("Section not found");
    }

    SectionHeader ExecutableFile::getSectionByType(Elf64_Word type) {
        for (auto& section: this->_sections) {
            if (section.header->sh_type == type) {
                return section;
            }
        }
        throw std::runtime_error("Section not found");
    }

    Symbol ExecutableFile::getSymbolByOffset(Elf64_Off offset) {
        for (auto& symbol: this->_symbols) {
            if (symbol.sym->st_value == offset) {
                return symbol;
            }
        }
        throw std::runtime_error("Symbol not found");
    }

    std::string ExecutableFile::getSymbolName(Symbol symbol) {
        auto linkSectionIndex = symbol.section.header->sh_link;
        SectionHeader strtab = this->_sections[linkSectionIndex];
        char *symbolName = strtab.content + symbol.sym->st_name;
        return std::string(symbolName);
    }

    Symbol ExecutableFile::getSymbolByName(std::string name) {
        for (auto& symbol: this->_symbols) {
            std::string symbolName = this->getSymbolName(symbol);
            if (symbolName == name) {
                return symbol;
            }
        }
        throw std::runtime_error("Symbol not found");
    }

    LineLocation ExecutableFile::getLineLocation(Dwarf_Addr address) {
        std::shared_ptr<dwarf::Line> matchedLine = nullptr;
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
        throw std::runtime_error("Line not found for the given address");
    }

    std::vector<std::shared_ptr<dwarf::Line>> ExecutableFile::getDebugLines() const {
        return this->_debugLines;
    }
} // namespace ELF
