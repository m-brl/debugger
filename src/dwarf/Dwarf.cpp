#include "dwarf/Dwarf.hpp"
#include <iostream>
#include <stdexcept>

#include <libdwarf.h>
#include <dwarf.h>

namespace dwarf {
    Line::~Line() {
    }

    Dwarf_Line Line::getLine() {
        return this->_line;
    }

    std::optional<Address> Line::getAddress() {
        Dwarf_Error dw_error{};
        Dwarf_Addr address{};

        if (this->_cachedAddress.has_value()) {
            return this->_cachedAddress.value();
        }
        if (dwarf_lineaddr(this->_line, &address, &dw_error) != DW_DLV_OK) {
            return std::nullopt;
        }
        this->_cachedAddress = address;
        return address;
    }

    std::optional<std::string> Line::getFileName() {
        Dwarf_Error dw_error{};
        char *filename = nullptr;

        if (this->_cachedFileName.has_value()) {
            return this->_cachedFileName.value();
        }
        if (dwarf_linesrc(_line, &filename, &dw_error) != DW_DLV_OK) {
            return std::nullopt;
        }
        _cachedFileName = filename;
        return _cachedFileName.value();
    }

    std::optional<std::size_t> Line::getLineNumber() {
        Dwarf_Error dw_error{};
        Dwarf_Unsigned line_number{};

        if (this->_cachedLineNumber.has_value()) {
            return this->_cachedLineNumber.value();
        }
        if (dwarf_lineno(this->_line, &line_number, &dw_error) != DW_DLV_OK) {
            return std::nullopt;
        }
        this->_cachedLineNumber = line_number;
        return line_number;
    }





    Die::~Die() {
        //dwarf_dealloc_die(this->_die);
    }

    std::optional<std::uint16_t> Die::getTag() {
        Dwarf_Error dw_error{};
        Dwarf_Half tag_id{};

        if (this->_cachedTag.has_value()) {
            return this->_cachedTag.value();
        }
        if (dwarf_tag(this->_die, &tag_id, &dw_error) != DW_DLV_OK) {
            return std::nullopt;
        }
        _cachedTag = tag_id;
        return tag_id;
    }

    std::optional<std::string_view> Die::getTagName() {
        auto tag = getTag();
        if (!tag.has_value()) return std::nullopt;

        const char *tag_name_str = nullptr;
        dwarf_get_TAG_name(tag.value(), &tag_name_str);
        _cachedTagName = tag_name_str;
        return _cachedTagName;
    }

    std::optional<Address> Die::getLowPC() {
        Dwarf_Error dw_error{};
        Dwarf_Addr low_pc{};

        if (this->_cachedLowPC.has_value()) {
            return this->_cachedLowPC.value();
        }
        if (dwarf_lowpc(this->_die, &low_pc, &dw_error) != DW_DLV_OK) {
            return std::nullopt;
        }
        this->_cachedLowPC = low_pc;
        return low_pc;
    }




    std::optional<std::string_view> DieVariable::getName() {
        Dwarf_Error dw_error{};
        Dwarf_Attribute attr_name{};

        if (this->_cachedName.has_value()) {
            return this->_cachedName.value();
        }

        if (dwarf_attr(this->_die, DW_AT_name, &attr_name, &dw_error) != DW_DLV_OK) {
            return std::nullopt;
        }

        char *variable_name{};
        dwarf_formstring(attr_name, &variable_name, &dw_error);
        this->_cachedName = variable_name;
        return this->_cachedName;
    }

    std::optional<std::string_view> DieSubprogram::getName() {
        Dwarf_Error dw_error{};
        Dwarf_Attribute attr_name{};

        if (this->_cachedName.has_value()) {
            return this->_cachedName.value();
        }

        if (dwarf_attr(this->_die, DW_AT_name, &attr_name, &dw_error) != DW_DLV_OK) {
            return std::nullopt;
        }

        char *subprogram_name{};
        dwarf_formstring(attr_name, &subprogram_name, &dw_error);
        this->_cachedName = subprogram_name;
        return this->_cachedName;
    }

    std::optional<Address> DieSubprogram::getHighPC() {
        Dwarf_Error dw_error{};
        Dwarf_Addr highpc;
        Dwarf_Half high_pc_form{};
        enum Dwarf_Form_Class highpc_class;

        if (this->_cachedHighPC.has_value()) {
            return _cachedHighPC.value();
        }
        if (dwarf_highpc_b(_die, &highpc, &high_pc_form, &highpc_class, &dw_error) != DW_DLV_OK) {
            return std::nullopt;
        }
        if (highpc_class == DW_FORM_CLASS_ADDRESS) {
            _cachedHighPC = highpc;
        }
        if (highpc_class == DW_FORM_CLASS_CONSTANT) {
            _cachedHighPC = _cachedLowPC.value_or(0) + highpc;
        }
        return _cachedHighPC.value();
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
        {DW_TAG_subprogram, [](auto die){ return std::make_shared<DieSubprogram>(die); }},
        {DW_TAG_variable, [](auto die){ return std::make_shared<DieVariable>(die); }},
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
