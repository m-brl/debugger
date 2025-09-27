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
#include "ExecutionWorkflow.hpp"
#include "File.hpp"
#include "utils.hpp"
#include "Command.hpp"

#include "display/Curses.hpp"
#include "display/DisplayManager.hpp"

bool IS_RUNNING = true;

void sighandler(int signum) {
    switch (signum) {
        case SIGINT:
            {
                auto pauseCommand = std::make_shared<PauseCommand>(*ContextManager::getInstance().getCurrentWorkflow());
                CommandManager::getInstance().addCommand(pauseCommand);
            }
            break;
        case 3:
        {
            IS_RUNNING = false;
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

    auto workflow = std::make_shared<ExecutionWorkflow>();
    workflow->setArguments(std::vector<std::string>(av + 1, av + ac));
    workflow->launch();
    workflow->continueExecution();
    ContextManager::getInstance().registerWorkflow(workflow);
    ContextManager::getInstance().setCurrentWorkflow(workflow);

    initDisplay();

    while (IS_RUNNING) {
        CommandManager::getInstance().execute();
        DISPLAY_MANAGER.getDisplayInterface()->tick();
        workflow->tick();
    }

    return 0;
}
#endif
