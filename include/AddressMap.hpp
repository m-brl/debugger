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
    struct AddressEntry {
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
    std::unordered_map<Elf64_Addr, AddressEntry> _addresses;
    std::map<std::string, std::shared_ptr<ExecutableFile>> _loadedFiles;
    std::shared_ptr<ExecutableFile> _exeFile;

    void _parseMapsLine(std::string line);
    void _loadMapsFile();

public:
    explicit AddressMap(std::filesystem::path exePath, pid_t pid);
    ~AddressMap() = default;

    void reload() { this->_loadMapsFile(); }

    std::optional<Address> getRelativeAddress(Address addr) const;
    AddressEntry getAddress(Elf64_Addr addr) const;
    AddressEntry getAddress(std::string pathname) const;

    std::shared_ptr<ExecutableFile> getFile(Elf64_Addr) const;
    std::shared_ptr<ExecutableFile> getExeFile() const { return _exeFile; }

    std::vector<std::shared_ptr<ExecutableFile>> getLoadedFiles() const;
    std::vector<AddressEntry> getAddresses() const {
        std::vector<AddressEntry> addresses;
        for (const auto& [start, address]: this->_addresses) {
            addresses.push_back(address);
        }
        return addresses;
    }
};
