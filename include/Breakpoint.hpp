#pragma once

#include <unistd.h>

#define INT3    0xCC

class Breakpoint {
private:
    pid_t _pid;
    unsigned long _address;
    bool _isEnabled;
    unsigned char _originalByte;

public:
    Breakpoint(pid_t pid, unsigned long address);
    ~Breakpoint();

    void setup();
    void restore();

    long getAddress() const { return _address; }

    bool operator==(const Breakpoint& other) const;
};
