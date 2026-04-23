#pragma once

#include <string>

namespace command {
    class ICommand {
        public:
            virtual ~ICommand() = default;
            virtual void execute() = 0;
            virtual std::string getDescription() const { return std::string("Generic Command"); }
    };
}
