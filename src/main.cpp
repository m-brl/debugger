#include <unistd.h>
#include <fcntl.h>
#include <elf.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <exception>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/ptrace.h>

#include "AddressMap.hpp"
#include "Logger.hpp"
#include "File.hpp"
#include "utils.hpp"

namespace {
  int display_address(int pid, ELF::File& file, AddressMap& addressMap) {
    struct user_regs_struct regs = {};
    ptrace(PTRACE_GETREGS, pid, NULL, &regs);

    try {
      //Logger::log(std::format("{:x}", regs.rip));
      auto address = addressMap.getAddress(regs.rip);
    } catch(std::runtime_error& e) {
    }

    return 0;
  }

  int execute_child(std::string program, ELF::File& file) {
    int pid = fork();

    if (pid < 0) {
      return EXIT_FAILURE;
    }
    if (pid == 0) {
      ptrace(PTRACE_TRACEME, 0, NULL, NULL);
      execl(program.c_str(), program.c_str(), NULL);
      return EXIT_SUCCESS;
    }


    int status = 0;
    waitpid(pid, &status, 0);
    AddressMap addressMap(pid);
    while (!WIFEXITED(status)) {
      display_address(pid, file, addressMap);
      ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL);
      waitpid(pid, &status, 0);
    }
    return 0;
  }
}

#ifndef UNIT_TEST
int main(int ac, char **av) {
  if (ac < 2) {
    std::cerr << "nope" << std::endl;
    return 1;
  }
  ELF::File file(av[1]);
  execute_child(av[1], file);

  return 0;
}
#endif
