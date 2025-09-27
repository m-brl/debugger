#pragma once

#include <functional>
#include <memory>
#include <stack>
#include <queue>
#include "ExecutionWorkflow.hpp"

class ICommand {
    public:
        virtual ~ICommand() = default;
        virtual void execute() = 0;
};

class CommandManager {
    private:
        static std::shared_ptr<CommandManager> _instance;
        static std::mutex _instanceMutex;

        std::queue<std::shared_ptr<ICommand>> _commandQueue;
        std::stack<std::shared_ptr<ICommand>> _undoStack;

        CommandManager() = default;

    public:
        CommandManager(const CommandManager&) = delete;
        CommandManager& operator=(const CommandManager&) = delete;
        static CommandManager& getInstance();

        void addCommand(std::shared_ptr<ICommand> command);
        void execute();
};





#pragma region Commands

class QuitCommand : public ICommand {
    private:
        bool &_isRunning;

    public:
        QuitCommand(bool &isRunning) : _isRunning(isRunning) {}
        ~QuitCommand() override = default;

        void execute() override {
            _isRunning = false;
        }
};

class AddBreakpointCommand : public ICommand {
    private:
        ExecutionWorkflow& _executionWorkflow;
        Breakpoint _breakpoint;

    public:
        AddBreakpointCommand(ExecutionWorkflow& workflow, Breakpoint breakpoint) : _executionWorkflow(workflow), _breakpoint(breakpoint) {}
        ~AddBreakpointCommand() override = default;

        void execute() override {
            _breakpoint.setup();
            _executionWorkflow.addBreakpoint(_breakpoint);
        }
};

class RemoveBreakpointCommand : public ICommand {
    private:
        ExecutionWorkflow& _executionWorkflow;
        Breakpoint _breakpoint;

    public:
        RemoveBreakpointCommand(ExecutionWorkflow& workflow, Breakpoint breakpoint) : _executionWorkflow(workflow), _breakpoint(breakpoint) {}
        ~RemoveBreakpointCommand() override = default;

        void execute() override {
            _executionWorkflow.removeBreakpoint(_breakpoint);
        }
};

class ClearBreakpointsCommand : public ICommand {
    private:
        ExecutionWorkflow& _executionWorkflow;

    public:
        ClearBreakpointsCommand(ExecutionWorkflow& workflow) : _executionWorkflow(workflow) {}
        ~ClearBreakpointsCommand() override = default;

        void execute() override {
            _executionWorkflow.clearBreakpoints();
        }
};

class StartCommand : public ICommand {
    private:
        ExecutionWorkflow& _executionWorkflow;

    public:
        StartCommand(ExecutionWorkflow& workflow) : _executionWorkflow(workflow) {}
        ~StartCommand() override = default;

        void execute() override {
            _executionWorkflow.launch();
        }
};

class ContinueCommand : public ICommand {
private:
    ExecutionWorkflow& _executionWorkflow;

public:
    ContinueCommand(ExecutionWorkflow& workflow) : _executionWorkflow(workflow) {}
    ~ContinueCommand() override = default;

    void execute() override {
        _executionWorkflow.continueExecution();
    }

};

class PauseCommand : public ICommand {
private:
    ExecutionWorkflow& _executionWorkflow;

public:
    PauseCommand(ExecutionWorkflow& workflow) : _executionWorkflow(workflow) {}
    ~PauseCommand() override = default;

    void execute() override {
        _executionWorkflow.pauseExecution();
    }
};

class StepCommand : public ICommand {
private:
    ExecutionWorkflow& _executionWorkflow;

public:
    StepCommand(ExecutionWorkflow& workflow) : _executionWorkflow(workflow) {}
    ~StepCommand() override = default;

    void execute() override {
        _executionWorkflow.step();
    }
};

class NextCommand : public ICommand {
private:
    ExecutionWorkflow& _executionWorkflow;

public:
    NextCommand(ExecutionWorkflow& workflow) : _executionWorkflow(workflow) {}
    ~NextCommand() override = default;

    void execute() override {
        _executionWorkflow.next();
    }
};





#pragma endregion

class CommandFactory {
    private:
        static const std::map<std::string, std::function<std::shared_ptr<ICommand>(ExecutionWorkflow&)>> _commandMap;

    public:
        static std::shared_ptr<ICommand> createCommand(const std::string& commandName, ExecutionWorkflow& workflow);
};
