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

#include "Logger.hpp"
#include "File.hpp"
#include "Tracee.hpp"
#include "utils.hpp"

namespace {
    int display_address(int pid) {
        struct user_regs_struct regs = {};
        ptrace(PTRACE_GETREGS, pid, NULL, &regs);

        uint16_t opcode = ptrace(PTRACE_PEEKDATA, pid, regs.rip, nullptr);
        opcode = SWAP_ENDIAN_16(opcode);
        std::cout  << "step> " << std::hex << regs.rip << " " << opcode << std::endl;

        return 0;
    }

    int execute_child(std::string program) {
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
        while (!WIFEXITED(status)) {
            display_address(pid);
            ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL); // N
            waitpid(pid, &status, 0);
        }
        return 0;
    }
}








union ElfFile {
    Elf64_Ehdr *ehdr;
    char *buffer;
};

namespace {
    void readSections(ElfFile elfFile) {
        Elf64_Shdr *shname_hdr = (Elf64_Shdr *)&elfFile.buffer[elfFile.ehdr->e_shoff + elfFile.ehdr->e_shstrndx * sizeof(Elf64_Shdr)];
        std::cout << shname_hdr->sh_offset << std::endl;

        Elf64_Off shoff = elfFile.ehdr->e_shoff;
        int i = 0;
        std::cout << "no: " << elfFile.ehdr->e_shnum << std::endl;
        while (i < elfFile.ehdr->e_shnum) {
            Elf64_Addr addr = shoff + (i * sizeof(Elf64_Shdr));
            Elf64_Shdr *shdr = (Elf64_Shdr*)&elfFile.buffer[addr];
            i++;
        }
    }

    void readFile(std::string program) {
        ElfFile elfFile;
        struct stat statFile = {};
        stat(program.c_str(), &statFile);

        int fd = open(program.c_str(), O_RDONLY);
        if (fd == 0) {
            return;
        }

        elfFile.ehdr = (Elf64_Ehdr *)mmap(nullptr, statFile.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        readSections(elfFile);
        munmap(elfFile.ehdr, statFile.st_size);

        close(fd);
    }
}

#ifndef UNIT_TEST
int main(int ac, char **av) {

    if (ac < 2) {
        std::cerr << "nope" << std::endl;
        return 1;
    }
    readFile(av[1]);
    ELF::File file(av[1]);
    file.getSectionByName(".text");

    return 0;
    execute_child(av[1]);
    try {
        Tracee tracee(av[1]);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
#endif
