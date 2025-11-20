#include "Tracee.hpp"
#include "Logger.hpp"

void Tracee::_loadMaps() {
  for (auto& address: _addressMap.getAddresses()) {
    if (address.pathname.empty() || _maps.contains(address.pathname) ||
        address.pathname[0] != '/') {
      continue;
    }
    try {
      ELF::File file(address.pathname);
      _maps.insert({address.pathname, file});
    } catch (std::exception& e) {
      Logger::log(std::format("Failed to open file"), Logger::ERR);
    }
  }
}

Tracee::Tracee(pid_t pid) : _addressMap(pid) {
  _loadMaps();
}
