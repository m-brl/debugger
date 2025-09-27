#include "Tracee.hpp"

// Exception
Tracee::ReadException::ReadException(std::string message) :
    _message(message) {}

const char *Tracee::ReadException::what() const noexcept {
    return this->_message.c_str();
}

// Tracee
Tracee::Tracee(std::string path) :
    _path(path) {
    int status = stat(this->_path.c_str(), &this->_fileStat);

    if (status != 0) {
        throw ReadException(std::format("{}: {}: {}", PROGRAM_NAME, strerror(errno), this->_path));
    }
}

Tracee::~Tracee() {}

pid_t Tracee::getPid() const {
    return this->_pid;
}

void Tracee::setPid(pid_t pid) {
    this->_pid = pid;
}
