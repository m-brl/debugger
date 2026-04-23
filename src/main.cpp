#include <elf.h>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <unistd.h>
#include <thread>

#include "AddressMap.hpp"
#include "ContextManager.hpp"
#include "Process.hpp"
#include "ExecutableFile.hpp"
#include "utils.hpp"
#include "Command.hpp"

#include "display/Curses.hpp"
#include "display/DisplayManager.hpp"

bool IS_RUNNING = true;

void sighandler(int signum) {
    switch (signum) {
        case SIGINT:
            {
                auto pauseCommand = std::make_shared<PauseCommand>(*ContextManager::getInstance().getCurrentProcess());
                CommandManager::getInstance().addCommand(pauseCommand);
            }
            break;
        case SIGQUIT:
            {
                auto quitCommand = std::make_shared<QuitCommand>(IS_RUNNING);
                CommandManager::getInstance().addConfirmationCommand(quitCommand);
                break;
            }
        default:
            break;
    }
}

void initDisplay() {
    std::shared_ptr<display::IDisplayInterface> cursesDisplay = std::shared_ptr<display::IDisplayInterface>(new display::CursesDisplay());
    cursesDisplay->init();
    DISPLAY_MANAGER.setDisplayInterface(cursesDisplay);
}

#ifndef UNIT_TEST
int main(int ac, char **av) {
    if (ac < 2) {
        std::cerr << "nope" << std::endl;
        return 1;
    }

    setlocale(LC_ALL, "");
    signal(SIGINT, &sighandler);
    signal(SIGQUIT, &sighandler);

    auto process = std::make_shared<Process>();
    process->setArguments(std::vector<std::string>(av + 1, av + ac));
    process->launch();
    ContextManager::getInstance().registerProcess(process);
    ContextManager::getInstance().setCurrentProcess(process);

    initDisplay();

    while (IS_RUNNING) {
        try {
            CommandManager::getInstance().execute();
        } catch (const std::exception& e) {
        }
        DISPLAY_MANAGER.getDisplayInterface()->tick();
        auto ptr = process.get();
        ptr->tick();
    }

    return 0;
}
#endif
