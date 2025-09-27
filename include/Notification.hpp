#pragma once

#include <cstring>
#include <format>
#include <memory>
#include <queue>
#include <string>

class Notification {
    private:
        std::string _message;

    public:
        Notification(std::string message) : _message(message) {}
        ~Notification() = default;

        std::string what() const {
            return _message;
        }
};

class NotificationManager {
    private:
        static std::shared_ptr<NotificationManager> _instance;
        static std::mutex _instanceMutex;

        std::queue<Notification> _eventQueue;

        NotificationManager() = default;

    public:
        NotificationManager(const NotificationManager&) = delete;
        NotificationManager& operator=(const NotificationManager&) = delete;
        static NotificationManager& getInstance();

        void addNotification(Notification notification);
        Notification getNextNotification();
        bool hasNotification() const;
};
