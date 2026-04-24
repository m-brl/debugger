#include "dwarf/DwarfFile.hpp"
#include "dwarf/Dwarf.hpp"

#include <fcntl.h>

namespace dwarf {
    Tree<std::shared_ptr<Die>> readDie(Dwarf_Debug& dw_dbg, Dwarf_Die root_die) {
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



    void DwarfFile::_loadFrameDescriptionEntries() {
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
        for (int i = 0; i < fde_count; i++) {
            Dwarf_Cie cie{};
            dwarf_get_cie_of_fde(fdes[i], &cie, &dw_error);
            _debugFdes.push_back(std::shared_ptr<dwarf::Fde>(new dwarf::Fde(fdes[i], cie)));
        }
        _debugFdesRaw = fdes;

        status = dwarf_get_fde_list_eh(this->_dw_dbg, &cies, &cie_count, &fdes, &fde_count, &dw_error);
        if (status == DW_DLV_ERROR) {
            const char *error_msg = dwarf_errmsg(dw_error);
            throw std::runtime_error("Failed to retrieve CIEs and FDEs from EH_FRAME: " + std::string(error_msg));
        }
        for (int i = 0; i < fde_count; i++) {
            Dwarf_Cie cie{};
            dwarf_get_cie_of_fde(fdes[i], &cie, &dw_error);
            _debugHeFdes.push_back(std::shared_ptr<dwarf::Fde>(new dwarf::Fde(fdes[i], cie)));
        }
        _debugHeFdesRaw = fdes;
    }

    void DwarfFile::_loadSourceLines(Dwarf_Die cu_die) {
        Dwarf_Error dw_error{};
        Dwarf_Small table_count{};
        Dwarf_Unsigned version{};
        Dwarf_Line_Context line_context{};

        int status = dwarf_srclines_b(cu_die, &version, &table_count, &line_context, &dw_error);
        if (status != DW_DLV_OK) return;

        Dwarf_Line *_lines = nullptr;
        Dwarf_Signed line_count{};
        dwarf_srclines_from_linecontext(line_context, &_lines, &line_count, &dw_error);
        for (int i = 0; i < line_count; i++) {
            char *file_name = nullptr;
            dwarf_linesrc(_lines[i], &file_name, &dw_error);
            if (!_sourceFiles.contains(file_name)) {
                try {
                    _sourceFiles[file_name] = std::make_shared<SourceFile>(file_name);
                } catch (const std::runtime_error& e) {
                    continue;
                }
            }
            _debugLines.push_back(std::make_shared<dwarf::Line>(_lines[i]));
        }
    }

    void DwarfFile::_loadCompilationUnits() {
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

        int status = dwarf_next_cu_header_e(_dw_dbg, dw_is_info, &cu_die, &dw_cu_header_length, &dw_version_stamp, &dw_abbrev_offset, &dw_address_size, &dw_length_size, &dw_extension_size, &dw_type_signature, &dw_typeoffset, &dw_next_cu_header_offset, &dw_header_cu_type, &dw_error);
        if (status != DW_DLV_OK) {
            return;
        }

        std::vector<Tree<std::shared_ptr<Die>>> children;
        while (status == DW_DLV_OK) {
            auto childTree = readDie(_dw_dbg, cu_die);
            children.push_back(childTree);
            _loadSourceLines(cu_die);

            status = dwarf_next_cu_header_e(_dw_dbg, dw_is_info, &cu_die, &dw_cu_header_length, &dw_version_stamp, &dw_abbrev_offset, &dw_address_size, &dw_length_size, &dw_extension_size, &dw_type_signature, &dw_typeoffset, &dw_next_cu_header_offset, &dw_header_cu_type, &dw_error);
        }
        _debugTree = Tree<std::shared_ptr<Die>>(std::make_shared<dwarf::DieRoot>(), children);

    }

    void DwarfFile::_loadFile() {
        int fd = open(this->_path.c_str(), O_RDONLY);
        if (fd < 0) throw std::runtime_error("Failed to load DWARF file");

        Dwarf_Handler dw_errhand{};
        Dwarf_Ptr dw_errarg{};
        Dwarf_Error dw_error{};

        int status = dwarf_init_b(fd, 0, dw_errhand, dw_errarg, &_dw_dbg, &dw_error);
        if (status != DW_DLV_OK) {
            close(fd);
            throw std::runtime_error("Failed to initialize DWARF debugging");
        }

        _loadCompilationUnits();
        _loadFrameDescriptionEntries();
    }

    DwarfFile::DwarfFile(std::filesystem::path path) : _path(path) {
        _loadFile();
    }

    DwarfFile::~DwarfFile() {
        if (_debugTree.has_value())
            _debugTree->children.clear();
        _debugLines.clear();
        dwarf_finish(_dw_dbg);
    }
}
