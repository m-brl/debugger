#pragma once

#include <libdwarf.h>
#include <string>

class Die {
private:
    Dwarf_Die _die;

public:
    explicit Die(Dwarf_Die die);
    ~Die();

    std::string getTag();
};
