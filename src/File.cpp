#include "File.hpp"
#include "Logger.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dwarf.h>
#include <libdwarf.h>

namespace ELF {
    void File::_parseSymbols() {
        auto symtab = this->getSectionByName(".symtab");

        Elf64_Off offset = 0;
        while (offset < symtab.header->sh_size) {
            this->_symbols.emplace_back(
                symtab.header->sh_offset + offset,
                reinterpret_cast<Elf64_Sym *>(this->_ehdr.buffer +
                                              symtab.header->sh_offset + offset));
            offset += sizeof(Elf64_Sym);
        }
    }

    void File::_parseSections() {
        for (int sectionIndex = 0; sectionIndex < this->_ehdr.elf64_ehdr->e_shnum;
             sectionIndex++) {
            Section section{};
            section.headerOff =
                this->_ehdr.elf64_ehdr->e_shoff + (sectionIndex * sizeof(Elf64_Shdr));
            section.header =
                reinterpret_cast<Elf64_Shdr *>(this->_ehdr.buffer + section.headerOff);
            section.contentOff = section.header->sh_offset;
            section.content = this->_ehdr.buffer + section.contentOff;
            this->_sections.push_back(section);
        }
    }

    Tree<Die> readDie(Dwarf_Debug* dw_dbg, Dwarf_Die root_die) {
/*#pragma region DebugPrint
            {
                Dwarf_Error dw_error{};
                Dwarf_Half tag_id{};
                const char *tag_name_str = nullptr;
                int status = dwarf_tag(root_die, &tag_id, &dw_error);
                if (status != DW_DLV_OK)
                    throw std::runtime_error("Failed to get DIE tag");
                dwarf_get_TAG_name(tag_id, &tag_name_str);
                Logger::rawLog(root_die);
            }
#pragma endregion*/
        Dwarf_Error dw_error{};

        // Read child
        Dwarf_Bool dw_is_info = 1;
        Dwarf_Die child_die{};
        Dwarf_Die sibling_die{};
        std::vector<Tree<Die>> children;

        int status = dwarf_child(root_die, &child_die, &dw_error);

        if (status != DW_DLV_OK) {
            return Tree(Die(root_die));
        }

        while (status == DW_DLV_OK) {
            children.push_back(readDie(dw_dbg, child_die));
            status = dwarf_siblingof_b(*dw_dbg, child_die, dw_is_info, &sibling_die, &dw_error);
            child_die = sibling_die;
        }

        return Tree(Die(root_die), children);
    }

    void File::_readCU(Dwarf_Debug* dw_dbg) {
        Dwarf_Bool dw_is_info = 1;
        Dwarf_Die dw_cu_die{};
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
            *dw_dbg, dw_is_info, &dw_cu_die, &dw_cu_header_length, &dw_version_stamp,
            &dw_abbrev_offset, &dw_address_size, &dw_length_size, &dw_extension_size,
            &dw_type_signature, &dw_typeoffset, &dw_next_cu_header_offset,
            &dw_header_cu_type, &dw_error);
        if (status != DW_DLV_OK) {
            char *err_msg = dwarf_errmsg(dw_error);
            Logger::log(FMT("Failed to read compilation unit header: {}", err_msg),
                        Logger::ERR);
            return;
        }

        while (status == DW_DLV_OK) {
            auto tmp = readDie(dw_dbg, dw_cu_die);
            this->_debugTree.push_back(tmp);
            status = dwarf_next_cu_header_e(
                *dw_dbg, dw_is_info, &dw_cu_die, &dw_cu_header_length, &dw_version_stamp,
                &dw_abbrev_offset, &dw_address_size, &dw_length_size,
                &dw_extension_size, &dw_type_signature, &dw_typeoffset,
                &dw_next_cu_header_offset, &dw_header_cu_type, &dw_error);
        }
    }

    void File::_parseDebug() {
        lseek(this->_fd, 0, SEEK_SET);

        Dwarf_Handler dw_errhand{};
        Dwarf_Ptr dw_errarg{};
        Dwarf_Debug dw_dbg{};
        Dwarf_Error dw_error{};

        if (dwarf_init_b(this->_fd, 0, dw_errhand, dw_errarg, &dw_dbg, &dw_error) !=
            DW_DLV_OK) {
            char *errMsg = dwarf_errmsg(dw_error);
            Logger::log(
                FMT("Failed to initialize DWARF debug information: {}", errMsg));
            return;
        }
        _readCU(&dw_dbg);
        std::cout << "---" << std::endl;
        std::cout << "$" << _debugTree.at(0).data.getTag() << std::endl;
        displayDieTree();

        //dwarf_finish(dw_dbg);
    }

    void File::_displayDieTree(Tree<Die> node, int depth) {
        for (int i = 0; i < depth; i++) {
            std::cout << "  ";
        }

        Logger::log(FMT("DIE Tag: {}", node.data.getTag()), Logger::NO_TIME);
        for (auto& child: node.children) {
            _displayDieTree(child, depth + 1);
        }
    }

    void File::_parseFile() {
        this->_parseSections();
        this->_parseSymbols();
        this->_parseDebug();
    }

    void File::_openFile() {
        this->_fd = open(this->_path.c_str(), O_RDONLY);
        fstat(this->_fd, &this->_fileStat);

        this->_ehdr.buffer = reinterpret_cast<char *>(mmap(
            nullptr, this->_fileStat.st_size, PROT_READ, MAP_PRIVATE, this->_fd, 0));
    }

    File::File(std::filesystem::path path) : _path(path), _fd(-1), _fileStat(0), _ehdr({}) {
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("File not found");
        }
        this->_openFile();
        this->_parseFile();
    }

    Section File::getSectionByName(std::string name) {
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

    Symbol File::getSymbolByOffset(Elf64_Off offset) {
        for (auto& symbol: this->_symbols) {
            if (symbol.sym->st_value == offset) {
                return symbol;
            }
        }
        throw std::runtime_error("Symbol not found");
    }

    void File::displayDieTree() {
        for (auto cu: _debugTree) {
            Logger::log(FMT("DIE Tag: {}", cu.data.getTag()), Logger::NO_TIME);
            _displayDieTree(cu, 1);
        }
    }
} // namespace ELF
