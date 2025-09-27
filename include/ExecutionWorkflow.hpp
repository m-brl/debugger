#pragma once

#include "AddressMap.hpp"
#include "Breakpoint.hpp"
#include "constant.h"
#include "File.hpp"

#include <vector>
#include <ranges>
#include <sys/user.h>

#define IS_TRACE_STARTED    BIT0
#define IS_TRACE_RUNNING    BIT1

class ExecutionWorkflow {
    private:
        pid_t _pid;
        std::vector<pid_t> _threadPids;
        int _stdinPipe[2];
        int _stdoutPipe[2];
        int _stderrPipe[2];
        std::string _stdoutLineBuffer;
        std::vector<std::string> _stdoutBuffer;
        std::string _stderrLineBuffer;
        std::vector<std::string> _stderrBuffer;

        std::vector<std::string> _arguments;
        std::vector<Breakpoint> _breakpoints;
        std::vector<Breakpoint> _mallocBreakpoints;

        struct user_regs_struct _registers;

        uint64_t _status;

        std::optional<AddressMap> _addressMap;

        void _getProcessStatus(pid_t pid);

    public:
        ExecutionWorkflow();
        ~ExecutionWorkflow();

        void setArguments(std::vector<std::string> args);
        void launch();

        void tick();

        pid_t getPid() const { return _pid; }
        uint64_t getStatus() const { return _status; }
        std::vector<std::string> getStdoutBuffer() const { return _stdoutBuffer; }
        std::vector<std::string> getStderrBuffer() const { return _stderrBuffer; }
        void clearStdoutBuffer() { _stdoutBuffer.clear(); }
        void clearStderrBuffer() { _stderrBuffer.clear(); }


        AddressMap getAddressMap() {
            if (_addressMap.has_value()) {
                return _addressMap.value();
            }
            throw std::runtime_error("Address map is not available.");
        }

        // Commands
        void addBreakpoint(Breakpoint breakpoint);
        void removeBreakpoint(Breakpoint breakpoint);
        void clearBreakpoints();

        void step();
        void next();
        void continueExecution();
        void pauseExecution();
};
