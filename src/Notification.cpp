#include "Notification.hpp"

std::shared_ptr<NotificationManager> NotificationManager::_instance;
std::mutex NotificationManager::_instanceMutex;

NotificationManager& NotificationManager::getInstance() {
    std::lock_guard<std::mutex> lock(_instanceMutex);
    if (_instance == nullptr) {
        _instance = std::shared_ptr<NotificationManager>(new NotificationManager());
    }
    return *_instance;
}

void NotificationManager::addNotification(Notification notification) {
    _eventQueue.push(notification);
}

Notification NotificationManager::getNextNotification() {
    auto event = _eventQueue.front();
    _eventQueue.pop();
    return event;
}

bool NotificationManager::hasNotification() const {
    return !_eventQueue.empty();
}
