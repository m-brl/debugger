#pragma once

#include "display/IDisplayInterface.hpp"

#include <memory>
#include <mutex>

class DisplayManager {
private:
    static std::shared_ptr<DisplayManager> _instance;
    static std::mutex _instanceMutex;

    std::shared_ptr<display::IDisplayInterface> _displayInterface;

    DisplayManager() = default;

public:
    ~DisplayManager() = default;
    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;
    static DisplayManager& getInstance();

    void setDisplayInterface(std::shared_ptr<display::IDisplayInterface> displayInterface);
    std::shared_ptr<display::IDisplayInterface> getDisplayInterface();
};

#define DISPLAY_MANAGER DisplayManager::getInstance()
