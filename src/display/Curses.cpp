#include "display/Curses.hpp"
#include "ContextManager.hpp"
#include "Command.hpp"
#include "utils.hpp"

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



    void CursesDisplay::_clear() {
        wclear(_mainWindow);
    }

    void CursesDisplay::_refresh() {
        wrefresh(_mainWindow);
    }

    void CursesDisplay::_display() {
        auto currentProcess = ContextManager::getInstance().getCurrentProcess();

        // Display file
        _displayBox(0, 0, COLS, 41, "File");
        if (!HAS_FLAG(currentProcess->getStatus(), IS_TRACE_RUNNING)) {
            Address rip = ptrace(PTRACE_PEEKUSER, currentProcess->getPid(), 8 * RIP, NULL);
            Address rrip = currentProcess->getAddressMap().getRelativeAddress(rip);

            auto file = currentProcess->getAddressMap().getFile(rip);
            auto lineLocation = file->getDwarfFile()->getLineLocation(rrip);
            if (!lineLocation.has_value()) {
                std::size_t first = std::max(1, static_cast<int>(lineLocation->lineNumber) - 20);

                for (std::size_t i = 0; i < 40; i++) {
                    if (first + i > lineLocation->file->getlineCount()) {
                        break;
                    }
                    mvprintw(1 + i, 2, "%s", lineLocation->file->getline(first + i).c_str());
                }
                mvprintw(1 + lineLocation->lineNumber - first, 1, ">");
            } else {
                mvprintw(1, 1, "Nothing to be displayed");
            }


        } else {
            mvwprintw(_mainWindow, 1, 1, "Running");
        }

        // Display stdout
        auto stdoutBuf = currentProcess->getStdoutBuffer();
        std::vector<std::string> croppedStdoutBuf(stdoutBuf.rbegin(), stdoutBuf.rbegin() + std::min(static_cast<std::size_t>(5), stdoutBuf.size()));
        _displayBox(0, LINES - 19, COLS / 2, 7, "Stdout", croppedStdoutBuf);

        // Display stderr
        auto stderrBuf = currentProcess->getStderrBuffer();
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
            if (_longTextBufferIt != _longTextBuffer.end()) {
                mvprintw(LINES - 2, 1, "More...");
            } else if (_pendingConfirmation) {
                mvprintw(LINES - 2, 1, "Are you sure do you want to %s (y/n)", _pendingConfirmation->getDescription().c_str());
            } else {
                mvwprintw(_mainWindow, LINES - 2, 1, "> %s", _input.c_str());
            }
        }
    }




    void CursesDisplay::_addLog(std::string log) {
        _log.push_back(log);
    }

    void CursesDisplay::_addLog(std::vector<std::string> log) {
        bool resetIt = _longTextBufferIt == _longTextBuffer.end();
        if (log.size() > 3) {
            for (auto tmp: log) {
                _longTextBuffer.push_back(tmp);
            }
            if (resetIt) {
                _longTextBufferIt = _longTextBuffer.begin();
            }
        } else {
            for (auto tmp: log) {
                _log.push_back(tmp);
            }
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

        auto process = ContextManager::getInstance().getCurrentProcess();

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
        if (command == "quit" || command == "exit") {
            auto quitCommand = std::make_shared<QuitCommand>(IS_RUNNING);
            CommandManager::getInstance().addConfirmationCommand(quitCommand);
            return;
        }
        if (command == "continue") {
            auto continueCommand = std::make_shared<ContinueCommand>(*ContextManager::getInstance().getCurrentProcess());
            CommandManager::getInstance().addCommand(continueCommand);
            _log.push_back("Continuing...");
            return;
        }
        if (command == "step") {
            auto stepCommand = std::make_shared<StepCommand>(*ContextManager::getInstance().getCurrentProcess());
            CommandManager::getInstance().addCommand(stepCommand);
            return;
        }
        if (command == "next") {
            auto nextCommand = std::make_shared<NextCommand>(*ContextManager::getInstance().getCurrentProcess());
            CommandManager::getInstance().addCommand(nextCommand);
            return;
        }
        if (command == "pid") {
            pid_t pid = ContextManager::getInstance().getCurrentProcess()->getPid();
            _log.push_back("PID: " + std::to_string(pid));
            return;
        }
        if (command == "threads") {
            int i = 0;
            for (auto& process : ContextManager::getInstance().getProcesses()) {
                _log.push_back(std::format("Thread {}: {}", i, process->getPid()));
                i++;
            }
        }
        if (command == "symbols") {
            auto file = process->getAddressMap().getExeFile();
            _addLog(text(file->getElfFile()->getSymbols(), [&](auto symbol) {
                return std::format("0x{:x} {}", symbol.getSym()->st_value, symbol.getName().value_or(""));
            }));
        }
        if (command == "fdes") {
            _addLog(text(process->getAddressMap().getExeFile()->getDwarfFile()->getDebugFdes(), [](const std::shared_ptr<dwarf::Fde>& fde) {
                return std::format("0x{:x}-0x{:x}", fde->getLowPC(), fde->getHighPC());
            }));
            _addLog(text(process->getAddressMap().getExeFile()->getDwarfFile()->getDebugHeFdes(), [](const std::shared_ptr<dwarf::Fde>& fde) {
                return std::format("0x{:x}-0x{:x}", fde->getLowPC(), fde->getHighPC());
            }));
        }
        if (command == "frame") {
            auto stacktrace = ContextManager::getInstance().getCurrentProcess()->getStacktrace();

            for (auto fde: stacktrace) {
                auto file = process->getAddressMap().getFile(fde->getLowPC());
                auto die = file->getDwarfFile()->getDieAtPc(fde->getLowPC());
                if (!die.has_value()) {
                    _log.push_back(std::format("0x{:x}-0x{:x} (unknown)", fde->getLowPC(), fde->getHighPC()));
                    continue;
                }
                _log.push_back(std::format("0x{:x}-0x{:x} ({})", fde->getLowPC(), fde->getHighPC(), die.value()->getTagName()));
            }

            return;
        }

        if (command == "fd") {
            auto fds = process->getFDs();
            _log.push_back("File Descriptors:");
            for (auto fd : fds) {
                auto fdInfo = process->getFDInfo(fd);
                _log.push_back(std::format("FD {}: {}", fd, fdInfo.path));
            }
            return;
        }

        if (command == "reload") {
            process->getAddressMap().reload();
            _log.push_back("Address map reloaded");
        }

        if (command == "rip") {
            auto pid = process->getPid();
            unsigned long rip = ptrace(PTRACE_PEEKUSER, pid, 8 * RIP, NULL);

            try {
                std::string path = ContextManager::getInstance().getCurrentProcess()->getAddressMap().getAddress(rip).pathname;
                _log.push_back(std::format("RIP: 0x{:x} ({})", rip, path));
            } catch (const std::exception& e) {
                _log.push_back(std::format("RIP: 0x{:x} (unknown)", rip));
            }
            unsigned long relativeAddress = process->getAddressMap().getRelativeAddress(rip);
            _log.push_back(std::format("Relative address pre: 0x{:x}", relativeAddress));

            std::shared_ptr<dwarf::Line> matchedLine = nullptr;

            try {
                auto file = process->getAddressMap().getFile(rip);
                unsigned long fileOffset = 0;

                for (auto ph: file->getElfFile()->getProgramByType(PT_LOAD)) {
                    if (ph.getHeader()->p_vaddr <= relativeAddress && relativeAddress < ph.getHeader()->p_vaddr + ph.getHeader()->p_memsz) {
                        fileOffset = relativeAddress - ph.getHeader()->p_vaddr + ph.getHeader()->p_offset;
                        break;
                    }
                }
                _log.push_back(std::format("Relative address: 0x{:x}", relativeAddress));
                _log.push_back(std::format("File offset: 0x{:x}", fileOffset));
                auto address = process->getAddressMap().getAddress(rip);
                for (auto& line : file->getDwarfFile()->getDebugLines()) {
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
            ContextManager::getInstance().getCurrentProcess()->clearStdoutBuffer();
            ContextManager::getInstance().getCurrentProcess()->clearStderrBuffer();
        }

        if (command == "status") {
            auto status = process->getStatus();
            _log.push_back(std::format("Process status: {}{}",
                HAS_FLAG(status, IS_TRACE_STARTED) ? "Started " : "",
                HAS_FLAG(status, IS_TRACE_RUNNING) ? "Running " : ""
            ));
        }

        if (command == "maps") {
            _addLog(text(process->getAddressMap().getAddresses(), [](const AddressMap::Address& address) {
                return std::format("0x{:x}-0x{:x} {}", address.start, address.end, address.pathname);
            }));
        }

        if (command == "break") {
            /*if (args.size() < 1) {
                _log.push_back("Usage: break <address>");
                return;
            }
            Elf64_Addr address = 0;
            if (args[0].substr(0, 2) == "0x") {
                address = std::stoul(args[0], nullptr, 16);
            } else {
                auto file = process->getAddressMap().getExeFile();
                if (!file) {
                    _log.push_back("No executable file loaded");
                    return;
                }
                AddressMap::Address addrLine = process->getAddressMap().getAddress(file->getPath().string());
                ELF::Symbol symbol = file->getSymbolByName(args[0]);
                if (symbol.sym->st_value != 0)
                    address = symbol.sym->st_value;
                else {
                    address = symbol.relocatedAddress;
                }

                unsigned long fileOffset = 0;

                for (auto ph: file->getProgramByType(PT_LOAD)) {
                    if (ph.header->p_vaddr <= address && address < ph.header->p_vaddr + ph.header->p_memsz) {
                        fileOffset = address - ph.header->p_vaddr + ph.header->p_offset;
                        break;
                    }
                }
                address = fileOffset + addrLine.start;

            }
            std::shared_ptr<ICommand> command = std::shared_ptr<ICommand>(new AddBreakpointCommand(*process, {process->getPid(), address}));
            CommandManager::getInstance().addCommand(command);
            _log.push_back(std::format("Breakpoint added at 0x{:x}", address));*/
        }

    }

    void CursesDisplay::_readStdin() {
        char ch = getch();
        if (ch == ERR) {
            return;
        }

        if (_pendingConfirmation) {
            if (ch == 'y' || ch == 'Y') {
                CommandManager::getInstance().addCommand(_pendingConfirmation);
            }
            _pendingConfirmation = nullptr;
            return;
        }

        if (_longTextBufferIt != _longTextBuffer.end()) {
            if (ch == 10 || ch == 'n') {
                _log.push_back(*_longTextBufferIt);
                _longTextBufferIt++;
                return;
            }
        }

        if (ch == 10) {
            _log.push_back("> " + _input);
            try {
                _handleCommand();
            } catch (const std::exception& e) {
                _log.push_back(std::string("Error: ") + e.what());
            }
            _inputHistory.push_back(_input);
            _inputHistoryIt = _inputHistory.end();
            _input.clear();
            return;
        }

        if (ch == 127) {
            if (!_input.empty()) {
                _input.pop_back();
            }
            return;
        }
        if (ch == 27) {
            ch = getch();
            if (ch == 91) {
                ch = getch();
                switch (ch) {
                    case 65: // Up
                        if (_inputHistoryIt != _inputHistory.begin())
                            _inputHistoryIt--;
                        _input = *_inputHistoryIt;
                        _log.push_back("Up arrow pressed");
                        return;
                    case 66: // Down
                        if (_inputHistoryIt != _inputHistory.end() - 1)
                            _inputHistoryIt++;
                        _input = *_inputHistoryIt;
                        _log.push_back("Down arrow pressed");
                        return;
                    case 67: // Right
                        _log.push_back("Right arrow pressed");
                        break;
                    case 68: // Left
                        _log.push_back("Left arrow pressed");
                        break;
                }
            }
        }
        _input.push_back(ch);
    }

    CursesDisplay::CursesDisplay() : _mainWindow(nullptr), _inputHistoryIt(_inputHistory.end()) {

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

        if (_longTextBufferIt == _longTextBuffer.end()) {
            _longTextBuffer.clear();
            _longTextBufferIt = _longTextBuffer.end();
        }
        if (!_pendingConfirmation) {
            _pendingConfirmation = CommandManager::getInstance().getNextConfirmation();
        }

        _clear();
        _display();
        _refresh();
    }
}
