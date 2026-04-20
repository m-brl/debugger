#include "display/DisplayManager.hpp"

DisplayManager& DisplayManager::getInstance() {
    std::lock_guard<std::mutex> lock(_instanceMutex);
    if (_instance == nullptr) {
        _instance = std::shared_ptr<DisplayManager>(new DisplayManager());
    }
    return *_instance;
}

void DisplayManager::setDisplayInterface(std::shared_ptr<display::IDisplayInterface> displayInterface) {
    _displayInterface = displayInterface;
}

std::shared_ptr<display::IDisplayInterface> DisplayManager::getDisplayInterface() {
    return _displayInterface;
}

std::shared_ptr<DisplayManager> DisplayManager::_instance = nullptr;
std::mutex DisplayManager::_instanceMutex;
