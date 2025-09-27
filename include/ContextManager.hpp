#pragma once

#include <memory>
#include <mutex>

#include <ExecutionWorkflow.hpp>

class ContextManager {
private:
    static std::shared_ptr<ContextManager> _instance;
    static std::mutex _instanceMutex;

    ContextManager() = default;

    std::shared_ptr<ExecutionWorkflow> _currentWorkflow;
    std::vector<std::shared_ptr<ExecutionWorkflow>> _workflows;

public:
    ~ContextManager() = default;
    ContextManager(const ContextManager&) = delete;
    ContextManager& operator=(const ContextManager&) = delete;
    static ContextManager& getInstance();

    std::shared_ptr<ExecutionWorkflow> getCurrentWorkflow() const;
    void setCurrentWorkflow(std::shared_ptr<ExecutionWorkflow> workflow);

    std::vector<std::shared_ptr<ExecutionWorkflow>> getWorkflows();
    void registerWorkflow(std::shared_ptr<ExecutionWorkflow> workflow);
    void unregisterWorkflow(std::shared_ptr<ExecutionWorkflow> workflow);
};
