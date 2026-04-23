#include "command/CommandManager.hpp"

namespace command {
    void CommandManager::addCommand(std::shared_ptr<ICommand> command) {
        _commandQueue.push(command);
    }

    void CommandManager::addConfirmationCommand(std::shared_ptr<ICommand> command) {
        _confirmationQueue.push(command);
    }

    void CommandManager::execute() {
        while (!_commandQueue.empty()) {
            std::shared_ptr<ICommand> command = _commandQueue.front();
            _undoStack.push(command);
            _commandQueue.pop();
            command->execute();
        }
    }

    std::shared_ptr<ICommand> CommandManager::getNextConfirmation() {
        if (_confirmationQueue.empty()) {
            return nullptr;
        }

        std::shared_ptr<ICommand> command = _confirmationQueue.front();
        _confirmationQueue.pop();
        return command;
    }
}
