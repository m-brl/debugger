#include "Breakpoint.hpp"

#include <stdexcept>
#include <format>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>

Breakpoint::Breakpoint(pid_t pid, unsigned long address) : _pid(pid), _address(address), _isEnabled(true), _originalByte(0) {
}

Breakpoint::~Breakpoint() {
}

void Breakpoint::setup() {
    unsigned long data = ptrace(PTRACE_PEEKDATA, _pid, _address, nullptr);
    _originalByte = static_cast<unsigned char>(data & 0xFF);
    data = (data & ~0xFF) | INT3;
    if (ptrace(PTRACE_POKEDATA, _pid, _address, data) == -1) {
        throw std::runtime_error(std::format("Failed to set breakpoint at address 0x{:x} ({})", _address, _pid));
    }
}

void Breakpoint::restore() {
    unsigned long data = ptrace(PTRACE_PEEKDATA, _pid, _address, nullptr);
    data = (data & ~0xFF) | _originalByte;
    ptrace(PTRACE_POKEDATA, _pid, _address, data);

    long rip = ptrace(PTRACE_PEEKUSER, _pid, 8 * RIP, nullptr);
    rip -= 1;
    ptrace(PTRACE_POKEUSER, _pid, 8 * RIP, rip);
}

bool Breakpoint::operator==(const Breakpoint& other) const {
    return _pid == other._pid && _address == other._address;
}
