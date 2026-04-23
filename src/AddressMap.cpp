#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>

#include "AddressMap.hpp"
#include "constant.h"
#include "utils.hpp"

void AddressMap::_parseMapsLine(std::string line) {
    if (line.at(line.size() - 1) == '\n') {
        line.pop_back();
    }
    auto tokens = split(line, ' ');

    AddressEntry address{};

    // Parsing addresses
    std::sscanf(tokens[0].c_str(), "%lx-%lx", &address.start, &address.end);

    if (_addresses.contains(address.start)) {
        return;
    }

    // Parsing permissions
    if (tokens[1][0] == 'r')
        address.perms = SET_FLAG(address.perms, BIT4);
    if (tokens[1][1] == 'w')
        address.perms = SET_FLAG(address.perms, BIT3);
    if (tokens[1][2] == 'x')
        address.perms = SET_FLAG(address.perms, BIT2);
    if (tokens[1][3] == 'p')
        address.perms = SET_FLAG(address.perms, BIT1);

    // Parsing offset
    std::sscanf(tokens[2].c_str(), "%lx", &address.offset);

    // Parsing dev
    address.dev = tokens[3];

    // Parsing inode
    std::sscanf(tokens[4].c_str(), "%d", &address.inode);

    // Parsing pathname
    if (tokens.size() >= 6) {
        address.pathname = tokens[5];
    }

    if (!address.pathname.empty() && address.pathname[0] != '[') {
        if (!_loadedFiles.contains(address.pathname)) {
            if (std::filesystem::exists(address.pathname)) {
                auto file = std::make_shared<ExecutableFile>(address.pathname);
                _loadedFiles[address.pathname] = file;
                if (std::filesystem::equivalent(address.pathname, _exePath.string())) {
                    _exeFile = file;
                }
            }
        }
    }

    _addresses.insert({address.start, address});
}

void AddressMap::_loadMapsFile() {
    std::filesystem::path path(std::format("/proc/{}/maps", _pid));
    std::ifstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open {}", path.string()));
    }
    while (!file.eof()) {
        std::string line;
        std::getline(file, line);
        if (line.empty())
            continue;
        _parseMapsLine(line);
    }
}

AddressMap::AddressMap(std::filesystem::path exePath, pid_t pid) : _pid(pid), _exePath(exePath) {
    this->_loadMapsFile();
}

std::optional<Address> AddressMap::getRelativeAddress(Address addr) const {
    for (const auto& [start, address]: this->_addresses) {
        if (addr >= address.start && addr < address.end) {
            return addr - address.start + address.offset;
        }
    }
    return std::nullopt;
}

AddressMap::AddressEntry AddressMap::getAddress(Elf64_Addr addr) const {
    for (const auto& [start, address]: this->_addresses) {
        if (addr >= address.start && addr < address.end) {
            return address;
        }
    }
    throw std::runtime_error("Address not found");
}

AddressMap::AddressEntry AddressMap::getAddress(std::string pathname) const {
    for (const auto& [start, address]: this->_addresses) {
        if (address.pathname == pathname) {
            return address;
        }
    }
    throw std::runtime_error("Address not found");
}

std::shared_ptr<ExecutableFile> AddressMap::getFile(Elf64_Addr saddress) const {
    AddressEntry address = this->getAddress(saddress);
    if (_loadedFiles.contains(address.pathname)) {
        return _loadedFiles.at(address.pathname);
    }
    throw std::runtime_error("No loaded executable file found");
}

std::vector<std::shared_ptr<ExecutableFile>> AddressMap::getLoadedFiles() const {
    std::vector<std::shared_ptr<ExecutableFile>> files;
    for (const auto& [path, file] : _loadedFiles) {
        files.push_back(file);
    }
    return files;
}
