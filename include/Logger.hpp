#pragma once

#include <ctime>
#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <sys/time.h>

#include "constant.h"

class Logger {
public:
  enum Flags { OUT = BIT1, ERR = BIT2, NO_ENDL = BIT3, NO_TIME = BIT4 };

private:
  static std::mutex _timeMutex;
  static std::mutex _outMutex;
  static std::mutex _errMutex;

  static void logOut(std::string message, int flags);
  static void logErr(std::string message, int flags);

public:
  template <typename T>
    requires requires(T t) { (std::cout << t); }
  static void log(T message, int flags = 0) {
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
};
