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

#include "Process.hpp"
#include "command/Command.hpp"
#include "command/CommandManager.hpp"

#include "display/Curses.hpp"

bool IS_RUNNING = true;

std::atomic<bool> G_SIGINT_RECEIVED(false);
std::atomic<bool> G_SIGQUIT_RECEIVED(false);

void sighandler(int signum) {
    switch (signum) {
        case SIGINT:
            G_SIGINT_RECEIVED.store(true);
            break;
        case SIGQUIT:
            G_SIGQUIT_RECEIVED.store(true);
            break;
        default:
            break;
    }
}

void sigcommand(std::shared_ptr<command::CommandManager> commandManager, std::shared_ptr<Process> process) {
    if (G_SIGINT_RECEIVED.exchange(false)) {
        auto pauseCommand = std::make_shared<command::PauseCommand>(process);
        commandManager->addCommand(pauseCommand);
    }

    if (G_SIGQUIT_RECEIVED.exchange(false)) {
        auto quitCommand = std::make_shared<command::QuitCommand>(IS_RUNNING);
        commandManager->addConfirmationCommand(quitCommand);
    }
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

    auto commandManager = std::make_shared<command::CommandManager>();

    auto cursesDisplay = std::make_shared<display::CursesDisplay>(commandManager);
    cursesDisplay->init();

    auto process = std::make_shared<Process>();
    process->setArguments(std::vector<std::string>(av + 1, av + ac));
    cursesDisplay->setProcess(process);
    process->launch();

    //ContextManager::getInstance().registerProcess(process);
    //ContextManager::getInstance().setCurrentProcess(process);

    while (IS_RUNNING) {
        sigcommand(commandManager, process);
        commandManager->execute();
        cursesDisplay->tick();
        auto ptr = process.get();
        ptr->tick();
    }

    return 0;
}
#endif
