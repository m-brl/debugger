#include "display/Curses.hpp"
#include "ContextManager.hpp"
#include "Command.hpp"
#include "Notification.hpp"

#include <sstream>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/reg.h>

namespace display {
    void CursesDisplay::_displayBox(int startX, int startY, int width, int height, std::string title, std::vector<std::string> content) {
        for (int x = startX; x < startX + width; x++) {
            mvwprintw(_mainWindow, startY, x, "%s", HORIZONTAL);
            mvwprintw(_mainWindow, startY + height - 1, x, "%s", HORIZONTAL);
        }
        mvprintw(startY, startX + 2, "%s", title.c_str());
        for (int y = startY; y < startY + height; y++) {
            mvwprintw(_mainWindow, y, startX, "%s", VERTICAL);
            mvwprintw(_mainWindow, y, startX + width - 1, "%s", VERTICAL);
        }
        mvwprintw(_mainWindow, startY, startX, "%s", CORNER_LT);
        mvwprintw(_mainWindow, startY, startX + width - 1, "%s", CORNER_RT);
        mvwprintw(_mainWindow, startY + height - 1, startX, "%s", CORNER_LB);
        mvwprintw(_mainWindow, startY + height - 1, startX + width - 1, "%s", CORNER_RB);

        int y = startY + 1;
        for (auto& line: content) {
            if (line.length() > static_cast<std::size_t>(width - 2)) {
                line = line.substr(0, width - 5) + "...";
            }
            mvwprintw(_mainWindow, y, startX + 1, "%s", line.c_str());
            y++;
        }
    }

    void CursesDisplay::_fetchNotifications() {
        auto& notificationManager = NotificationManager::getInstance();
        while (notificationManager.hasNotification()) {
            _log.push_back(notificationManager.getNextNotification().what());
        }
    }

    void CursesDisplay::_clear() {
        wclear(_mainWindow);
    }

    void CursesDisplay::_refresh() {
        wrefresh(_mainWindow);
    }

    void CursesDisplay::_display() {
        auto currentWorkflow = ContextManager::getInstance().getCurrentWorkflow();

        // Display file
        _displayBox(0, 0, COLS, 44, "File");
        if (!HAS_FLAG(currentWorkflow->getStatus(), IS_TRACE_RUNNING)) {
            unsigned long rip = ptrace(PTRACE_PEEKUSER, currentWorkflow->getPid(), 8 * RIP, NULL);

            try {
                auto file = currentWorkflow->getAddressMap().getFile(rip);
                LineLocation lineLocation = file->getLineLocation(rip);
                std::size_t first = std::max(1, static_cast<int>(lineLocation.lineNumber) - 20);

                for (std::size_t i = 0; i < 40; i++) {
                    if (first + i > lineLocation.file->getlineCount()) {
                        break;
                    }
                    mvprintw(1 + i, 2, "%s", lineLocation.file->getline(first + i).c_str());
                }
                mvprintw(1 + lineLocation.lineNumber - first, 1, ">");

            } catch (const std::exception& e) {
                mvprintw(1, 1, "Nothing to be displayed (%s)", e.what());
            }
        } else {
            mvwprintw(_mainWindow, 1, 1, "Running");
        }

        // Display stdout
        auto stdoutBuf = currentWorkflow->getStdoutBuffer();
        std::vector<std::string> croppedStdoutBuf(stdoutBuf.rbegin(), stdoutBuf.rbegin() + std::min(static_cast<std::size_t>(5), stdoutBuf.size()));
        _displayBox(0, LINES - 19, COLS / 2, 7, "Stdout", croppedStdoutBuf);

        // Display stderr
        auto stderrBuf = currentWorkflow->getStderrBuffer();
        std::vector<std::string> croppedStderrBuf(stderrBuf.rbegin(), stderrBuf.rbegin() + std::min(static_cast<std::size_t>(5), stderrBuf.size()));
        _displayBox(COLS / 2, LINES - 19, COLS / 2, 7, "Stderr", croppedStderrBuf);

        // Display input
        _displayBox(0, LINES - 12, COLS, 12, "Input", {});
        {
            auto it = _log.rbegin();
            for (std::size_t i = 0; i < 9; i++, it++) {
                if (it == _log.rend()) {
                    break;
                }
                mvwprintw(_mainWindow, LINES - 3 - i, 1, "%s", it->c_str());
            }
            mvwprintw(_mainWindow, LINES - 2, 1, "> %s", _input.c_str());
        }
    }

    void CursesDisplay::_handleCommand() {
        std::istringstream iss(_input);
        std::string command;
        std::vector<std::string> args;
        iss >> command;
        std::string arg;
        while (iss >> arg) {
            args.push_back(arg);
        }

        auto workflow = ContextManager::getInstance().getCurrentWorkflow();

        if (command == "display") {
            if (args.size() < 1) {
                _log.push_back("Usage: display <file|hexdump>");
                return;
            }
            if (args[0] == "file") {
                displayType = FILE_VIEW;
                _log.push_back("Switched to file view");
                return;
            }
            if (args[0] == "hexdump") {
                displayType = HEXADUMP_VIEW;
                _log.push_back("Switched to hexdump view");
                return;
            }
            _log.push_back("Unknown display type: " + args[0]);
            return;
        }
        if (command == "quit") {
            auto quitCommand = std::make_shared<QuitCommand>(IS_RUNNING);
            CommandManager::getInstance().addCommand(quitCommand);
            _log.push_back("Quitting...");
            return;
        }
        if (command == "continue") {
            auto continueCommand = std::make_shared<ContinueCommand>(*ContextManager::getInstance().getCurrentWorkflow());
            CommandManager::getInstance().addCommand(continueCommand);
            _log.push_back("Continuing...");
            return;
        }
        if (command == "step") {
            auto stepCommand = std::make_shared<StepCommand>(*ContextManager::getInstance().getCurrentWorkflow());
            CommandManager::getInstance().addCommand(stepCommand);
            return;
        }
        if (command == "next") {
            auto nextCommand = std::make_shared<NextCommand>(*ContextManager::getInstance().getCurrentWorkflow());
            CommandManager::getInstance().addCommand(nextCommand);
            return;
        }
        if (command == "pid") {
            pid_t pid = ContextManager::getInstance().getCurrentWorkflow()->getPid();
            _log.push_back("PID: " + std::to_string(pid));
            return;
        }
        if (command == "threads") {
            int i = 0;
            for (auto& workflow : ContextManager::getInstance().getWorkflows()) {
                _log.push_back(std::format("Thread {}: {}", i, workflow->getPid()));
                i++;
            }
        }
        if (command == "symbols") {
            auto file = workflow->getAddressMap().getExeFile();
            _log.push_back("Symbols of " + file->getPath().string() + ":");
            for (auto& symbol : file->getSymbols()) {
                if (symbol.sym->st_value == 0)
                    continue;
                _log.push_back(std::format("0x{:x} {} ({:x})", symbol.sym->st_value, file->getSymbolName(symbol), symbol.relocatedAddress));
            }
        }
        if (command == "frame") {
            unsigned long rip = ptrace(PTRACE_PEEKUSER, workflow->getPid(), 8 * RIP, NULL);

            Dwarf_Fde *fdeList;
            Dwarf_Error err;
            if (dwarf_get_fde_list(workflow->getAddressMap().getExeFile()->getDwarfDebug(), nullptr, nullptr, &fdeList, nullptr, &err) != DW_DLV_OK) {
                _log.push_back("Failed to get FDE list");
                return;
            }





        }

        if (command == "rip") {
            auto pid = workflow->getPid();
            unsigned long rip = ptrace(PTRACE_PEEKUSER, pid, 8 * RIP, NULL);

            try {
                std::string path = ContextManager::getInstance().getCurrentWorkflow()->getAddressMap().getAddress(rip).pathname;
                _log.push_back(std::format("RIP: 0x{:x} ({})", rip, path));
            } catch (const std::exception& e) {
                _log.push_back(std::format("RIP: 0x{:x} (unknown)", rip));
            }

            std::shared_ptr<dwarf::Line> matchedLine = nullptr;

            try {
                auto file = workflow->getAddressMap().getFile(rip);
                for (auto& line : file->getDebugLines()) {
                    if (line->getAddress() > rip) {
                        break;
                    }
                    matchedLine = line;
                }
                if (matchedLine) {
                    std::string filename = matchedLine->getFileName();
                    Dwarf_Unsigned lineNumber = matchedLine->getLineNumber();
                    _log.push_back("Source: " + filename + ":" + std::to_string(lineNumber));
                }
            } catch (const std::exception& e) {
                return;
            }

            return;
        }

        if (command == "clear") {
            ContextManager::getInstance().getCurrentWorkflow()->clearStdoutBuffer();
            ContextManager::getInstance().getCurrentWorkflow()->clearStderrBuffer();
        }
        if (command == "break") {
            if (args.size() < 1) {
                _log.push_back("Usage: break <address>");
                return;
            }
            Elf64_Addr address = 0;
            if (args[0].substr(0, 2) == "0x") {
                address = std::stoul(args[0], nullptr, 16);
            } else {
                auto file = workflow->getAddressMap().getExeFile();
                ELF::Symbol symbol = file->getSymbolByName(args[0]);
                if (symbol.sym->st_value != 0)
                    address = symbol.sym->st_value;
                else {
                    address = symbol.relocatedAddress;
                }
            }
            std::shared_ptr<ICommand> command = std::shared_ptr<ICommand>(new AddBreakpointCommand(*workflow, {workflow->getPid(), address}));
            CommandManager::getInstance().addCommand(command);
            _log.push_back(std::format("Breakpoint added at 0x{:x}", address));
        }

    }

    void CursesDisplay::_readStdin() {
        char ch = getch();
        if (ch == ERR) {
            return;
        }
        if (ch == 10) {
            _handleCommand();
            _input.clear();
            return;
        }
        if (ch == 127) {
            if (!_input.empty()) {
                _input.pop_back();
            }
            return;
        }
        _input.push_back(ch);
    }

    CursesDisplay::CursesDisplay() : _mainWindow(nullptr) {

    }

    CursesDisplay::~CursesDisplay() {
        endwin();
    }

    void CursesDisplay::init() {
        _mainWindow = initscr();
        nodelay(_mainWindow, TRUE);
    }

    void CursesDisplay::tick() {
        static auto last = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (now - last < std::chrono::milliseconds(10)) {
            return;
        }
        last = now;

        _readStdin();
        _fetchNotifications();

        _clear();
        _display();
        _refresh();
    }
}
