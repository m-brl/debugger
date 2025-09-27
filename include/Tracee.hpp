#pragma once

#include <string>
#include <exception>
#include <format>
#include <errno.h>
#include <cstring>
#include <sys/stat.h>

#include "utils.hpp"
#include "constant.h"

class Tracee {
public:
    class ReadException : public std::exception {
    private:
        std::string _message;

    public:
        explicit ReadException(std::string message);
        ~ReadException() = default;

        const char *what() const noexcept override;
    };

private:
    std::string _path;
    struct stat _fileStat;
    pid_t _pid;

public:
    explicit Tracee(std::string path);
    ~Tracee();

    pid_t getPid() const;
    void setPid(pid_t pid);
};
