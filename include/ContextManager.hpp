#pragma once

#include <memory>
#include <mutex>

#include "Process.hpp"

class ContextManager {
private:
    std::shared_ptr<Process> _currentProcess;
    std::vector<std::shared_ptr<Process>> _processes;

public:
    explicit ContextManager() = default;
    ~ContextManager() = default;
    ContextManager(const ContextManager&) = delete;
    ContextManager& operator=(const ContextManager&) = delete;

    std::shared_ptr<Process> getCurrentProcess() const;
    void setCurrentProcess(std::shared_ptr<Process> process);

    std::vector<std::shared_ptr<Process>> getProcesses();
    void registerProcess(std::shared_ptr<Process> process);
    void unregisterProcess(std::shared_ptr<Process> process);
};
