#pragma once

#include "ExecutableFile.hpp"

#include <elf.h>
#include <string>
#include <unistd.h>
#include <memory>
#include <map>
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
    std::filesystem::path _exePath;
    std::unordered_map<Elf64_Addr, Address> _addresses;
    std::map<std::string, std::shared_ptr<ExecutableFile>> _loadedFiles;
    std::shared_ptr<ExecutableFile> _exeFile;

    void _parseMapsLine(std::string line);
    void _loadMapsFile();

public:
    explicit AddressMap(std::filesystem::path exePath, pid_t pid);
    ~AddressMap() = default;

    void reload() { this->_loadMapsFile(); }

    Elf64_Addr getRelativeAddress(Elf64_Addr addr) const;
    Address getAddress(Elf64_Addr addr) const;
    Address getAddress(std::string pathname) const;

    std::shared_ptr<ExecutableFile> getFile(Elf64_Addr) const;
    std::shared_ptr<ExecutableFile> getExeFile() const { return _exeFile; }

    std::vector<std::shared_ptr<ExecutableFile>> getLoadedFiles() const;
    std::vector<Address> getAddresses() const {
        std::vector<Address> addresses;
        for (const auto& [start, address]: this->_addresses) {
            addresses.push_back(address);
        }
        return addresses;
    }
};
