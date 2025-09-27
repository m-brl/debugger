#pragma once

#include "constant.h"

#include <ctime>
#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <sys/time.h>

#include <fcntl.h>
#include <csignal>
#include <sys/ioctl.h>
#include <cstring>
#include <termios.h>
#include <unistd.h>

class Logger {
public:
    enum Flags { OUT = BIT1,
                 ERR = BIT2,
                 NO_TIME = BIT3 };

private:
    static std::string _input;

    static std::mutex _timeMutex;
    static std::mutex _inMutex;
    static std::mutex _outMutex;
    static std::mutex _errMutex;

    static struct winsize _winsize;
    static struct termios _attrOld;
    static struct termios _attrNew;

    static void _clearLine();
    static void _displayStdinPrompt();
    static void _logOut(std::string message, int flags);
    static void _logErr(std::string message, int flags);

public:
    static void init() {
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &_winsize);
        signal(SIGWINCH, [](int) { ioctl(STDOUT_FILENO, TIOCGWINSZ, &_winsize); });

        tcgetattr(STDIN_FILENO, &_attrOld);
        std::memcpy(&_attrNew, &_attrOld, sizeof(struct termios));
        _attrNew.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &_attrNew);

        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

        _displayStdinPrompt();
    }

    static void restore() {
        tcsetattr(STDIN_FILENO, TCSANOW, &_attrOld);
    }

    template <typename T>
        requires requires(T t) { (std::cout << t); }
    static void log(T message, int flags = 0) {
        _clearLine();
        std::string out;

        if (!HAS_FLAG(flags, NO_TIME)) {
            std::lock_guard lock(_timeMutex);
            time_t now = {};
            struct timeval tv = {};
            struct tm *timeinfo = {};

            time(&now);
            gettimeofday(&tv, nullptr);
            timeinfo = localtime(&now);

            out += std::format("[{}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}] ",
                               timeinfo->tm_year + 1900, timeinfo->tm_mon + 1,
                               timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min,
                               timeinfo->tm_sec, tv.tv_usec / 1000);
        }

        out += message;

        if (HAS_FLAG(flags, OUT)) {
            _logOut(out, flags);
        }
        if (HAS_FLAG(flags, ERR)) {
            _logErr(out, flags);
        }
        if (!HAS_FLAG(flags, OUT) && !HAS_FLAG(flags, ERR)) {
            _logOut(out, flags);
        }
        _displayStdinPrompt();
    }

    static std::string readStdin() {
        std::lock_guard lock(_inMutex);
        char buf[255] = {};
        ssize_t ret = read(STDIN_FILENO, buf, sizeof(buf) + 1);
        for (int i = 0; i < ret; i++) {
            _input += buf[i];
        }

        for (auto& c: _input) {
            if (c == '\n') {
                _input.clear();
            }
        }
        _displayStdinPrompt();
        return {};
    }

    template <typename T>
    static void rawLog(T t) {
        char *ptr = reinterpret_cast<char *>(&t);

        for (size_t i = 0; i < sizeof(T); i++) {
            //std::cout << std::format("{:02X} ", static_cast<unsigned char>(ptr[i]));
        }
        //std::cout << std::endl;
    }
};
