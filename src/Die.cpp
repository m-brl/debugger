#include "Die.hpp"
#include "Logger.hpp"
#include <iostream>
#include <stdexcept>

Die::Die(Dwarf_Die die) : _die(die) {
    std::cout << "> " << getTag() << std::endl;
}

Die::~Die() {}

std::string Die::getTag() {
    Dwarf_Error dw_error{};
    Dwarf_Half tag_id{};
    const char *tag_name_str = nullptr;
    int status = dwarf_tag(this->_die, &tag_id, &dw_error);
    if (status != DW_DLV_OK)
        throw std::runtime_error("Failed to get DIE tag");
    dwarf_get_TAG_name(tag_id, &tag_name_str);
    return tag_name_str;
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
