#pragma once

#include <memory>
#include "Process.hpp"

namespace display {
    class IDisplayInterface {
        public:
            virtual ~IDisplayInterface() = default;

            virtual void init() = 0;
            virtual void tick() = 0;
            virtual void setProcess(std::shared_ptr<Process> process) = 0;
    };
}
