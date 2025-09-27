#include "Command.hpp"

std::shared_ptr<CommandManager> CommandManager::_instance = nullptr;
std::mutex CommandManager::_instanceMutex;

CommandManager& CommandManager::getInstance() {
    std::lock_guard<std::mutex> lock(_instanceMutex);
    if (_instance == nullptr) {
        _instance = std::shared_ptr<CommandManager>(new CommandManager());
    }
    return *_instance;
}

void CommandManager::addCommand(std::shared_ptr<ICommand> command) {
    _commandQueue.push(command);
}

void CommandManager::execute() {
    while (!_commandQueue.empty()) {
        std::shared_ptr<ICommand> command = _commandQueue.front();
        command->execute();
        _undoStack.push(command);
        _commandQueue.pop();
    }
}

const std::map<std::string, std::function<std::shared_ptr<ICommand>(ExecutionWorkflow&)>> CommandFactory::_commandMap = {
};

std::shared_ptr<ICommand> CommandFactory::createCommand(const std::string& commandName, ExecutionWorkflow& workflow) {
    auto it = _commandMap.find(commandName);
    if (it != _commandMap.end()) {
        return it->second(workflow);
    }
    return nullptr;
}
