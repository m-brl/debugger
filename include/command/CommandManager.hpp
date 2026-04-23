#pragma once

#include "command/ICommand.hpp"

#include <stack>
#include <queue>
#include <memory>

namespace command {
    class CommandManager {
        private:
            std::queue<std::shared_ptr<ICommand>> _commandQueue;
            std::queue<std::shared_ptr<ICommand>> _confirmationQueue;
            std::stack<std::shared_ptr<ICommand>> _undoStack;


        public:
            explicit CommandManager() = default;

            void addCommand(std::shared_ptr<ICommand> command);
            void addConfirmationCommand(std::shared_ptr<ICommand> command);
            void execute();

            std::shared_ptr<ICommand> getNextConfirmation();
    };
}
