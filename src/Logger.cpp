#include "Logger.hpp"

#include <iostream>

void Logger::_clearLine() {
    std::lock_guard lock(_outMutex);

    //std::cout << "\r";
    //for (unsigned int i = 0; i < _winsize.ws_col - 1; i++) {
        //std::cout << " ";
    //}
    //std::cout << "\r";
}

void Logger::_displayStdinPrompt() {
    std::lock_guard lock(_outMutex);

    //std::cout << "> " << _input;
    //std::cout.flush();
}

void Logger::_logOut(std::string message, int flags) {
    std::lock_guard lock(_outMutex);

    //std::cout << message << std::endl;
}

void Logger::_logErr(std::string message, int flags) {
    std::lock_guard lock(_outMutex);

    std::cerr << message << std::endl;
}

std::string Logger::_input;

std::mutex Logger::_timeMutex;
std::mutex Logger::_inMutex;
std::mutex Logger::_outMutex;
std::mutex Logger::_errMutex;

struct winsize Logger::_winsize;
struct termios Logger::_attrOld;
struct termios Logger::_attrNew;
