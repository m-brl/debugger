#pragma once

#include <memory>
#include <mutex>

#include "Process.hpp"

class ContextManager {
private:
    static std::shared_ptr<ContextManager> _instance;
    static std::mutex _instanceMutex;

    ContextManager() = default;

    std::shared_ptr<Process> _currentProcess;
    std::vector<std::shared_ptr<Process>> _processes;

public:
    ~ContextManager() = default;
    ContextManager(const ContextManager&) = delete;
    ContextManager& operator=(const ContextManager&) = delete;
    static ContextManager& getInstance();

    std::shared_ptr<Process> getCurrentProcess() const;
    void setCurrentProcess(std::shared_ptr<Process> process);

    std::vector<std::shared_ptr<Process>> getProcesses();
    void registerProcess(std::shared_ptr<Process> process);
    void unregisterProcess(std::shared_ptr<Process> process);
};
