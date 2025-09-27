#include "ContextManager.hpp"

std::shared_ptr<ContextManager> ContextManager::_instance = nullptr;
std::mutex ContextManager::_instanceMutex;

ContextManager& ContextManager::getInstance() {
    std::lock_guard<std::mutex> lock(_instanceMutex);
    if (_instance == nullptr) {
        _instance = std::shared_ptr<ContextManager>(new ContextManager());
    }
    return *_instance;
}

std::shared_ptr<ExecutionWorkflow> ContextManager::getCurrentWorkflow() const {
    return _currentWorkflow;
}

void ContextManager::setCurrentWorkflow(std::shared_ptr<ExecutionWorkflow> workflow) {
    _currentWorkflow = workflow;
}

std::vector<std::shared_ptr<ExecutionWorkflow>> ContextManager::getWorkflows() {
    return _workflows;
}

void ContextManager::registerWorkflow(std::shared_ptr<ExecutionWorkflow> workflow) {
    _workflows.push_back(workflow);
}

void ContextManager::unregisterWorkflow(std::shared_ptr<ExecutionWorkflow> workflow) {
    auto it = std::find(_workflows.begin(), _workflows.end(), workflow);
    if (it != _workflows.end()) {
        _workflows.erase(it);
    }
}
