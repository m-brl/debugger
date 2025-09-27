#include "Dwarf.hpp"
#include "Logger.hpp"
#include <iostream>
#include <stdexcept>

#include <libdwarf.h>
#include <dwarf.h>

namespace dwarf {
    Line::~Line() {
    }

    Dwarf_Addr Line::getAddress() {
        Dwarf_Error dw_error{};
        Dwarf_Addr address{};

        if (this->_cachedAddress.has_value()) {
            return this->_cachedAddress.value();
        }

        if (dwarf_lineaddr(this->_line, &address, &dw_error) != DW_DLV_OK) {
            throw std::runtime_error("Failed to retrieve line address");
        }
        this->_cachedAddress = address;
        return address;
    }

    std::string Line::getFileName() {
        Dwarf_Error dw_error{};
        char *filename = nullptr;

        if (this->_cachedFileName.has_value()) {
            return this->_cachedFileName.value();
        }

        if (dwarf_linesrc(_line, &filename, &dw_error) != DW_DLV_OK) {
            throw std::runtime_error("Failed to retrieve line address");
        }
        _cachedFileName = filename;
        return _cachedFileName.value();
    }

    Dwarf_Unsigned Line::getLineNumber() {
        Dwarf_Error dw_error{};
        Dwarf_Unsigned line_number{};

        if (this->_cachedLineNumber.has_value()) {
            return this->_cachedLineNumber.value();
        }

        if (dwarf_lineno(this->_line, &line_number, &dw_error) != DW_DLV_OK) {
            throw std::runtime_error("Failed to retrieve line number");
        }
        this->_cachedLineNumber = line_number;
        return line_number;
    }



    Die::~Die() {
        //dwarf_dealloc_die(this->_die);
    }

    std::string Die::getTagName() {
        Dwarf_Error dw_error{};
        Dwarf_Half tag_id{};

        if (this->_cachedTag.has_value()) {
            tag_id = this->_cachedTag.value();
        } else {
            int status = dwarf_tag(this->_die, &tag_id, &dw_error);
            if (status != DW_DLV_OK) {
                throw std::runtime_error("Failed to retrieve die tag");
            }
            this->_cachedTag = tag_id;
        }
        const char *tag_name_str = nullptr;
        dwarf_get_TAG_name(tag_id, &tag_name_str);
        return tag_name_str;
    }

    Dwarf_Addr Die::getLowPC() {
        Dwarf_Error dw_error{};
        Dwarf_Addr low_pc{};

        if (this->_cachedLowPC.has_value()) {
            return this->_cachedLowPC.value();
        }

        int res = dwarf_lowpc(this->_die, &low_pc, &dw_error);
        if (res != DW_DLV_OK) {
            throw std::runtime_error("Failed to retrieve low PC");
        }
        this->_cachedLowPC = low_pc;
        return low_pc;
    }



    std::string DieSubprogram::getSubprogramName() {
        Dwarf_Error dw_error{};
        Dwarf_Attribute attr_name{};
        if (dwarf_attr(this->_die, DW_AT_name, &attr_name, &dw_error) != DW_DLV_OK) {
            throw std::runtime_error("Failed to get subprogram name attribute");
        }
        char *subprogram_name{};
        dwarf_formstring(attr_name, &subprogram_name, &dw_error);

        Dwarf_Addr low_pc{};
        if (dwarf_attr(this->_die, DW_AT_low_pc, &attr_name, &dw_error) == DW_DLV_OK) {
            dwarf_formaddr(attr_name, &low_pc, &dw_error);
        }

        return FMT("{} (0x{:x})", std::string(subprogram_name), low_pc);
    }



    std::shared_ptr<Die> DieFactory::createDie(Dwarf_Die die) {
        Dwarf_Error dw_error{};
        Dwarf_Half tag_id{};

        int status = dwarf_tag(die, &tag_id, &dw_error);
        if (status != DW_DLV_OK) {
            throw std::runtime_error("Failed to retrieve die tag");
        }

        auto it = _tagMap.find(tag_id);
        if (it != _tagMap.end()) {
            return it->second(die);
        }
        return std::make_shared<Die>(die);
    }

    std::map<Dwarf_Half, std::function<std::shared_ptr<Die>(Dwarf_Die)>> DieFactory::_tagMap {
        {DW_TAG_subprogram, [](auto die){ return std::make_shared<Die>(die); }}
    };
}


/*dwarf_child(root_die, &child_die, &dw_error);

  while ((status = dwarf_siblingof_b(dw_dbg, child_die, dw_is_info,
  &child_die, &dw_error)) != DW_DLV_NO_ENTRY) { Dwarf_Half child_tag_name {};
  status = dwarf_tag(child_die, &child_tag_name, &dw_error);
  if (child_tag_name == DW_TAG_variable) {
  Dwarf_Attribute attr_name {};
  Dwarf_Error dw_error {};
  if (dwarf_attr(child_die, DW_AT_name, &attr_name, &dw_error) ==
  DW_DLV_OK) { char *var_name {}; dwarf_formstring(attr_name, &var_name,
  &dw_error); Logger::log(FMT("    Variable name: {}",
  std::string(var_name)));
  }
  const char *child_tag_name_str = nullptr;
  dwarf_get_TAG_name(child_tag_name, &child_tag_name_str);
  Logger::log(FMT("  Child DIE tag: {}",
  std::string(child_tag_name_str)));
  }
  std::cout << "Sibling DIE read successfully" << std::endl;
  }*/
