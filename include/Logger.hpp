#pragma once

#include <mutex>
#include <string>

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
  static void log(const char *message, int flags = 0);
};
