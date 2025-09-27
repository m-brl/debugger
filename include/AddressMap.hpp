#pragma once

#include "File.hpp"

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
    std::vector<Address> _addresses;
    std::map<std::string, std::shared_ptr<ELF::ExecutableFile>> _loadedFiles;
    std::shared_ptr<ELF::ExecutableFile> _exeFile;

    void _parseMapsLine(std::string line);
    void _loadMapsFile();

public:
    explicit AddressMap(std::filesystem::path exePath, pid_t pid);
    ~AddressMap() = default;

    Address getAddress(Elf64_Addr addr) const;

    std::shared_ptr<ELF::ExecutableFile> getFile(Elf64_Addr) const;
    std::shared_ptr<ELF::ExecutableFile> getExeFile() const { return _exeFile; }

    std::vector<std::shared_ptr<ELF::ExecutableFile>> getLoadedFiles() const;
    std::vector<Address> getAddresses() const { return _addresses; }
};
