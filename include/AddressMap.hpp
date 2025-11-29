#pragma once

#include <elf.h>
#include <string>
#include <unistd.h>
#include <vector>

class AddressMap {
public:
    struct Address {
        Elf64_Addr start;
        Elf64_Addr end;
        char perms;
        Elf64_Off offset;
        std::string dev;
        int inode;
        std::string pathname;
    };

private:
    pid_t _pid;
    std::vector<Address> _addresses;

    void _parseMapsLine(std::string line);
    void _loadMapsFile();

public:
    explicit AddressMap(pid_t pid);
    ~AddressMap() = default;

    [[nodiscard]] Address getAddress(Elf64_Addr addr) const;
    [[nodiscard]] std::vector<Address> getAddresses() const { return _addresses; }
};
