#pragma once

#include "SourceFile.hpp"
#include "display/IDisplayInterface.hpp"

#include <curses.h>
#include <memory>
#include "Command.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>

#define HORIZONTAL  "\u2500"
#define VERTICAL    "\u2502"
#define CORNER_LT   "\u250C"
#define CORNER_RT   "\u2510"
#define CORNER_LB   "\u2514"
#define CORNER_RB   "\u2518"
#define T_RIGHT     "\u251C"
#define T_LEFT      "\u2524"
#define T_DOWN      "\u252C"
#define T_UP        "\u2534"
#define CROSS       "\u253C"

extern bool IS_RUNNING;

namespace display {
    class CursesDisplay : public IDisplayInterface {
        private:
            enum DisplayType {
                FILE_VIEW,
                HEXADUMP_VIEW
            } displayType;

            WINDOW *_mainWindow;

            std::shared_ptr<SourceFile> _currentSourceFile;

            void _displayBox(int startX, int startY, int width, int height, std::string title, std::vector<std::string> content = {});

            void _clear();
            void _refresh();
            void _displayFileView();
            void _display();

            void _addLog(std::string log);
            void _addLog(std::vector<std::string> log);

            std::vector<std::string> _longTextBuffer;
            std::vector<std::string>::iterator _longTextBufferIt;

            std::shared_ptr<ICommand> _pendingConfirmation;
            std::vector<std::string> _log;
            std::vector<std::string> _inputHistory;
            std::vector<std::string>::iterator _inputHistoryIt;
            std::string _input;

            void _handleCommand();
            void _readStdin();


        public:
            CursesDisplay();
            ~CursesDisplay();

            void init() override;

            void tick() override;
    };
}
