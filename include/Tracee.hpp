#pragma once

#include <map>

#include <unistd.h>

#include "AddressMap.hpp"
#include "File.hpp"

class Tracee {
private:
  AddressMap _addressMap;
  std::map<std::string, ELF::File> _maps;

  void _loadMaps();

public:
  explicit Tracee(pid_t pid);
  ~Tracee() = default;
};
