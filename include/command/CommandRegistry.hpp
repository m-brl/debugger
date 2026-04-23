#pragma once

#include "command/ICommand.hpp"
#include "Process.hpp"

#include <map>
#include <memory>
#include <functional>

namespace command {
    class CommandRegistry {
        private:
            static const std::map<std::string, std::function<std::shared_ptr<ICommand>(Process&)>> _commandMap;

        public:
            static std::shared_ptr<ICommand> createCommand(const std::string& commandName, Process& process);
    };
}
