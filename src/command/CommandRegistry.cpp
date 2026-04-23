#include "command/CommandRegistry.hpp"

namespace command {
    const std::map<std::string, std::function<std::shared_ptr<ICommand>(Process&)>> CommandRegistry::_commandMap = {
    };

    std::shared_ptr<ICommand> CommandRegistry::createCommand(const std::string& commandName, Process& process) {
        auto it = _commandMap.find(commandName);
        if (it != _commandMap.end()) {
            return it->second(process);
        }
        return nullptr;
    }
}
