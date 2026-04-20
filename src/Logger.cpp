#include "Logger.hpp"

#include <iostream>

void Logger::logOut(std::string message, int flags) {
  std::lock_guard lock(_outMutex);

  std::cout << message;
  if (!HAS_FLAG(flags, NO_ENDL)) {
    std::cout << std::endl;
  }
}

void Logger::logErr(std::string message, int flags) {
  std::lock_guard lock(_outMutex);

  std::cerr << message;
  if (!HAS_FLAG(flags, NO_ENDL)) {
    std::cerr << std::endl;
  }
}

std::mutex Logger::_timeMutex;
std::mutex Logger::_outMutex;
std::mutex Logger::_errMutex;
