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

std::shared_ptr<Process> ContextManager::getCurrentProcess() const {
    return _currentProcess;
}

void ContextManager::setCurrentProcess(std::shared_ptr<Process> process) {
    _currentProcess = process;
}

std::vector<std::shared_ptr<Process>> ContextManager::getProcesses() {
    return _processes;
}

void ContextManager::registerProcess(std::shared_ptr<Process> process) {
    _processes.push_back(process);
}

void ContextManager::unregisterProcess(std::shared_ptr<Process> process) {
    auto it = std::find(_processes.begin(), _processes.end(), process);
    if (it != _processes.end()) {
        _processes.erase(it);
    }
}
