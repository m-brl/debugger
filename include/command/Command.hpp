#pragma once

#include "command/ICommand.hpp"
#include "Process.hpp"

namespace command {
    class QuitCommand : public ICommand {
        private:
            bool &_isRunning;

        public:
            QuitCommand(bool &isRunning) : _isRunning(isRunning) {}
            ~QuitCommand() override = default;

            void execute() override {
                _isRunning = false;
            }

            std::string getDescription() const override {
                return std::string("quit the debugger");
            }
    };

    class RunCommand : public ICommand {
        private:
            Process& _executionWorkflow;

        public:
            RunCommand(Process& process) : _executionWorkflow(process) {}
            ~RunCommand() override = default;

            void execute() override {
                _executionWorkflow.continueExecution();
            }
    };

    class AddBreakpointCommand : public ICommand {
        private:
            Process& _executionWorkflow;
            Breakpoint _breakpoint;

        public:
            AddBreakpointCommand(Process& process, Breakpoint breakpoint) : _executionWorkflow(process), _breakpoint(breakpoint) {}
            ~AddBreakpointCommand() override = default;

            void execute() override {
                _breakpoint.setup();
                _executionWorkflow.addBreakpoint(_breakpoint);
            }
    };

    class RemoveBreakpointCommand : public ICommand {
        private:
            Process& _executionWorkflow;
            Breakpoint _breakpoint;

        public:
            RemoveBreakpointCommand(Process& process, Breakpoint breakpoint) : _executionWorkflow(process), _breakpoint(breakpoint) {}
            ~RemoveBreakpointCommand() override = default;

            void execute() override {
                _executionWorkflow.removeBreakpoint(_breakpoint);
            }
    };

    class ClearBreakpointsCommand : public ICommand {
        private:
            Process& _executionWorkflow;

        public:
            ClearBreakpointsCommand(Process& process) : _executionWorkflow(process) {}
            ~ClearBreakpointsCommand() override = default;

            void execute() override {
                _executionWorkflow.clearBreakpoints();
            }
    };

    class ContinueCommand : public ICommand {
        private:
            Process& _executionWorkflow;

        public:
            ContinueCommand(Process& process) : _executionWorkflow(process) {}
            ~ContinueCommand() override = default;

            void execute() override {
                _executionWorkflow.continueExecution();
            }

    };

    class PauseCommand : public ICommand {
        private:
            Process& _executionWorkflow;

        public:
            PauseCommand(Process& process) : _executionWorkflow(process) {}
            ~PauseCommand() override = default;

            void execute() override {
                _executionWorkflow.pauseExecution();
            }
    };

    class StepCommand : public ICommand {
        private:
            Process& _executionWorkflow;

        public:
            StepCommand(Process& process) : _executionWorkflow(process) {}
            ~StepCommand() override = default;

            void execute() override {
                _executionWorkflow.step();
            }
    };

    class NextCommand : public ICommand {
        private:
            Process& _executionWorkflow;

        public:
            NextCommand(Process& process) : _executionWorkflow(process) {}
            ~NextCommand() override = default;

            void execute() override {
                _executionWorkflow.next();
            }
    };
}
