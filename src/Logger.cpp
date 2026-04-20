#include "Logger.hpp"

#include <ctime>
#include <format>
#include <iostream>
#include <sys/time.h>

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

void Logger::log(const char *message, int flags) {
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
    logOut(out, flags);
  }
  if (HAS_FLAG(flags, ERR)) {
    logErr(out, flags);
  }
  if (!HAS_FLAG(flags, OUT) && !HAS_FLAG(flags, ERR)) {
    logOut(out, flags);
  }
}

std::mutex Logger::_timeMutex;
std::mutex Logger::_outMutex;
std::mutex Logger::_errMutex;
