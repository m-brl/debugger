#include "display/Curses.hpp"
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

    void CursesDisplay::_displayFileView() {
        // Display file
        _displayBox(0, 0, COLS, 41, "File");
        if (HAS_FLAG(_process->getStatus(), IS_TRACE_RUNNING)) {
            mvwprintw(_mainWindow, 1, 1, "Running");
            return;
        }

        Address rip = ptrace(PTRACE_PEEKUSER, _process->getPid(), 8 * RIP, NULL);
        auto rrip = _process->getAddressMap().getStaticAddress(rip);

        if (!rrip.has_value()) {
            mvwprintw(_mainWindow, 1, 1, "RIP: 0x%lx (unknown)", rip);
            return;
        }

        auto file = _process->getAddressMap().getFile(rip);
        if (!file.has_value()) {
            mvwprintw(_mainWindow, 1, 1, "RIP: 0x%lx (no file)", rip);
            return;
        }

        auto lineLocation = file.value()->getDwarfFile()->getLineLocation(rrip.value());
        if (lineLocation.has_value()) {
            std::size_t first = std::max(1, static_cast<int>(lineLocation->lineNumber) - 20);

            for (std::size_t i = 0; i < 40; i++) {
                if (first + i > lineLocation->file->getlineCount()) {
                    break;
                }
                mvprintw(1 + i, 2, "%s", lineLocation->file->getline(first + i).c_str());
            }
            mvprintw(1 + lineLocation->lineNumber - first, 1, ">");
        } else {
            mvprintw(1, 1, "Nothing to be displayed 0x%lx 0x%lx", rip, rrip.value());
        }
    }

    void CursesDisplay::_display() {
        _displayFileView();

        // Display stdout
        auto stdoutBuf = _process->getStdoutBuffer();
        std::vector<std::string> croppedStdoutBuf(stdoutBuf.rbegin(), stdoutBuf.rbegin() + std::min(static_cast<std::size_t>(5), stdoutBuf.size()));
        _displayBox(0, LINES - 19, COLS / 2, 7, "Stdout", croppedStdoutBuf);

        // Display stderr
        auto stderrBuf = _process->getStderrBuffer();
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
            if (_longTextBufferIndex != _longTextBuffer.size()) {
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
        bool resetIt = _longTextBufferIndex == _longTextBuffer.size();
        if (log.size() > 3) {
            for (auto tmp: log) {
                _longTextBuffer.push_back(tmp);
            }
            if (resetIt) {
                _longTextBufferIndex = 0;
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
            auto quitCommand = std::make_shared<command::QuitCommand>(IS_RUNNING);
            _commandManager->addConfirmationCommand(quitCommand);
            return;
        }
        if (command == "continue") {
            auto continueCommand = std::make_shared<command::ContinueCommand>(_process);
            _commandManager->addCommand(continueCommand);
            _log.push_back("Continuing...");
            return;
        }
        if (command == "step") {
            auto stepCommand = std::make_shared<command::StepCommand>(_process);
            _commandManager->addCommand(stepCommand);
            return;
        }
        if (command == "next") {
            auto nextCommand = std::make_shared<command::NextCommand>(_process);
            _commandManager->addCommand(nextCommand);
            return;
        }
        if (command == "pid") {
            pid_t pid = _process->getPid();
            _log.push_back("PID: " + std::to_string(pid));
            return;
        }
        if (command == "threads") {
            int i = 0;
            /*for (auto& process : ContextManager::getInstance().getProcesses()) {
                _log.push_back(std::format("Thread {}: {}", i, process->getPid()));
                i++;
            }*/
        }
        if (command == "symbols") {
            auto file = _process->getAddressMap().getExeFile();
            _addLog(text(file->getElfFile()->getSymbols(), [&](auto symbol) {
                        return std::format("0x{:x} {} ({})", symbol.getSym()->st_value, symbol.getName().value_or(""), file->getPath().string());
                        }));
        }
        if (command == "dies") {
            auto file = _process->getAddressMap().getExeFile();
            _addLog(text(file->getDwarfFile()->getDiesByTag(DW_TAG_subprogram), [&](std::shared_ptr<dwarf::Die> die) {
                        auto dieSubprogram = std::dynamic_pointer_cast<dwarf::DieSubprogram>(die);
                        return std::format("{} at 0x{:x}", dieSubprogram->getName().value_or("unknown"), die->getLowPC().value_or(0));
                        }));
        }

        if (command == "fde") {
            Address rip = ptrace(PTRACE_PEEKUSER, _process->getPid(), 8 * RIP, NULL);
            auto rrip = _process->getAddressMap().getStaticAddress(rip);
            if (!rrip.has_value()) {
                _log.push_back("Failed to retrieve fde");
                return;
            }
            auto fde = _process->getAddressMap().getExeFile()->getDwarfFile()->getFdeAtPc(rrip.value());
            if (!fde.has_value()) {
                _log.push_back("Failed to retrieve fde");
                return;
            }
            _log.push_back(std::format("FDE for RIP 0x{:x}: 0x{:x}-0x{:x}", rip, fde.value()->getLowPC(), fde.value()->getHighPC()));
        }

        if (command == "fdes") {
            _addLog(text(_process->getAddressMap().getExeFile()->getDwarfFile()->getDebugFdes(), [](const std::shared_ptr<dwarf::Fde>& fde) {
                return std::format("0x{:x}-0x{:x}", fde->getLowPC(), fde->getHighPC());
            }));
            _addLog(text(_process->getAddressMap().getExeFile()->getDwarfFile()->getDebugHeFdes(), [](const std::shared_ptr<dwarf::Fde>& fde) {
                return std::format("0x{:x}-0x{:x}", fde->getLowPC(), fde->getHighPC());
            }));
        }
        if (command == "frame") {
            auto stacktrace = _process->getStacktrace();

            for (auto stackframe: stacktrace) {
                auto staticAddress = 0;
                auto fde = stackframe.fde;
                auto file = stackframe.file;
                /*if (!file.has_value()) {
                    _log.push_back(std::format("0x{:x}-0x{:x} (unknown)", fde->getLowPC(), fde->getHighPC()));
                    continue;
                }*/

                auto die = file->getDwarfFile()->getSubprogram(fde->getLowPC());
                if (!die.has_value()) {
                    _log.push_back(std::format("0x{:x}-0x{:x} at 0x{:x} (unknown)", fde->getLowPC(), fde->getHighPC(), staticAddress));
                    continue;
                }
                if (auto subprogramdie = std::dynamic_pointer_cast<dwarf::DieSubprogram>(die.value())) {
                    _log.push_back(std::format("0x{:x}-0x{:x} at 0x{:x} ({})", fde->getLowPC(), fde->getHighPC(), staticAddress, subprogramdie->getName().value_or("unknown")));
                    continue;
                }
                _log.push_back(std::format("0x{:x}-0x{:x} ({})", fde->getLowPC(), fde->getHighPC(), die.value()->getTagName().value()));
            }

            return;
        }

        if (command == "fd") {
            auto fds = _process->getFDs();
            _log.push_back("File Descriptors:");
            for (auto fd : fds) {
                auto fdInfo = _process->getFDInfo(fd);
                _log.push_back(std::format("FD {}: {}", fd, fdInfo.path));
            }
            return;
        }

        if (command == "reload") {
            _process->getAddressMap().reload();
            _log.push_back("Address map reloaded");
        }

        if (command == "rip") {
            auto pid = _process->getPid();
            unsigned long rip = ptrace(PTRACE_PEEKUSER, pid, 8 * RIP, NULL);

            auto addressEntry = _process->getAddressMap().getAddress(rip);
            if (addressEntry.has_value()) {
                std::string path = addressEntry.value().pathname;
                _log.push_back(std::format("RIP: 0x{:x} ({})", rip, path));
            } else {
                _log.push_back(std::format("RIP: 0x{:x} (unknown)", rip));
            }

            auto rrip = _process->getAddressMap().getStaticAddress(rip);
            if (!rrip.has_value()) {
                return;
            }

            std::shared_ptr<dwarf::Line> matchedLine = nullptr;

            try {
                auto file = _process->getAddressMap().getFile(rip);
                if (!file.has_value()) return;
                unsigned long fileOffset = 0;

                for (auto ph: file.value()->getElfFile()->getSegmentsByType(PT_LOAD)) {
                    if (ph.getHeader()->p_vaddr <= rrip.value() && rrip.value() < ph.getHeader()->p_vaddr + ph.getHeader()->p_memsz) {
                        fileOffset = rrip.value() - ph.getHeader()->p_vaddr + ph.getHeader()->p_offset;
                        break;
                    }
                }
                _log.push_back(std::format("Relative address: 0x{:x}", rrip.value()));
                _log.push_back(std::format("File offset: 0x{:x}", fileOffset));
                auto address = _process->getAddressMap().getAddress(rip);
                for (auto& line : file.value()->getDwarfFile()->getDebugLines()) {
                    if (line->getAddress() > rip) {
                        break;
                    }
                    matchedLine = line;
                }
                if (matchedLine) {
                    std::string filename = matchedLine->getFileName().value_or("unknown");
                    Dwarf_Unsigned lineNumber = matchedLine->getLineNumber().value_or(0);
                    _log.push_back("Source: " + filename + ":" + std::to_string(lineNumber));
                }
            } catch (const std::exception& e) {
                return;
            }

            return;
        }

        if (command == "clear") {
            _process->clearStdoutBuffer();
            _process->clearStderrBuffer();
            _log.clear();
        }

        if (command == "status") {
            auto status = _process->getStatus();
            _log.push_back(std::format("Process status: {}{}",
                HAS_FLAG(status, IS_TRACE_STARTED) ? "Started " : "",
                HAS_FLAG(status, IS_TRACE_RUNNING) ? "Running " : ""
            ));
        }

        if (command == "maps") {
            _addLog(text(_process->getAddressMap().getAddresses(), [](const AddressMap::AddressEntry& address) {
                return std::format("0x{:x}-0x{:x} {}", address.start, address.end, address.pathname);
            }));
        }

        /*if (command == "stepout") {
            auto currentFrame = _process->getCurrentFrame();
            if (!currentFrame.has_value()) {
                _log.push_back("No current frame");
                return;
            }
            auto callingFrame = _process->getCallingFrame(currentFrame.value());
            if (!callingFrame.has_value()) {
                _log.push_back("No calling frame");
                return;
            }
            auto command = std::shared_ptr<command::ICommand>(new command::AddBreakpointCommand(_process, {_process->getPid(), callingFrame.value().address}));
            _commandManager->addCommand(command);
            _log.push_back(std::format("Step out to 0x{:x} from 0x{:x}", callingFrame.value().staticAddress, currentFrame.value().staticAddress));
        }*/

        if (command == "break") {
            if (args.size() < 1) {
                _log.push_back("Usage: break <address>");
                return;
            }

            Address address = 0;
            if (args[0].substr(0, 2) == "0x") {
                address = std::stoul(args[0], nullptr, 16);
            } else {
                auto file = _process->getAddressMap().getExeFile();
                if (!file) {
                    _log.push_back("No executable file loaded");
                    return;
                }
                auto addrLine = _process->getAddressMap().getAddress(file->getPath().string());
                if (!addrLine.has_value()) return;

                auto symbol = file->getElfFile()->getSymbolByName(args[0]);
                if (!symbol.has_value()) {
                    _log.push_back("Symbol not found: " + args[0]);
                    return;
                }

                address = symbol->getSym()->st_value;
                /*if (symbol->getSym()->st_value != 0)
                    address = symbol.sym->st_value;
                else {
                    address = symbol.relocatedAddress;
                }*/

                unsigned long fileOffset = 0;

                for (auto ph: file->getElfFile()->getSegmentsByType(PT_LOAD)) {
                    auto header = ph.getHeader();
                    if (header->p_vaddr <= address && address < header->p_vaddr + header->p_memsz) {
                        fileOffset = address - header->p_vaddr + header->p_offset;
                        break;
                    }
                }
                address = fileOffset + addrLine.value().start;

            }
            auto command = std::shared_ptr<command::ICommand>(new command::AddBreakpointCommand(_process, {_process->getPid(), address}));
            _commandManager->addCommand(command);
            _log.push_back(std::format("Breakpoint added at 0x{:x}", address));
        }

    }

    void CursesDisplay::_readStdin() {
        char ch = getch();
        if (ch == ERR) {
            return;
        }

        if (_pendingConfirmation) {
            if (ch == 'y' || ch == 'Y') {
                _commandManager->addCommand(_pendingConfirmation);
            }
            _pendingConfirmation = nullptr;
            return;
        }

        if (_longTextBufferIndex != _longTextBuffer.size()) {
            if (ch == 10 || ch == 'n') {
                _log.push_back(_longTextBuffer[_longTextBufferIndex]);
                _longTextBufferIndex++;
                return;
            }
        }

        if (ch == 10) {
            _log.push_back("> " + _input);
            _handleCommand();
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

    CursesDisplay::CursesDisplay(std::shared_ptr<command::CommandManager> commandManager) : _commandManager(commandManager), _mainWindow(nullptr), _inputHistoryIt(_inputHistory.end()) {

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
        if (now - last < std::chrono::milliseconds(100)) {
            return;
        }
        last = now;

        _readStdin();

        if (_longTextBufferIndex == _longTextBuffer.size()) {
            _longTextBuffer.clear();
            _longTextBufferIndex = 0;
        }
        if (!_pendingConfirmation) {
            _pendingConfirmation = _commandManager->getNextConfirmation();
        }

        _clear();
        _display();
        _refresh();
    }

    void CursesDisplay::setProcess(std::shared_ptr<Process> process) {
        _process = process;
    }
}
