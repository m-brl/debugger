#include "Process.hpp"
#include "ContextManager.hpp"
#include "Logger.hpp"
#include "command/Command.hpp"

#include <cstring>
#include <fstream>
#include <unistd.h>
#include <dirent.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>

void Process::_getProcessStatus(pid_t pid) {
    int status = 0;
    pid_t result = waitpid(pid, &status, WNOHANG | WUNTRACED);
    if (result == 0 || result == -1) {
        return;
    }

    if (WIFEXITED(status)) {
        _status = CLEAR_FLAG(_status, IS_TRACE_STARTED);

        int exitCode = WEXITSTATUS(status);
        //Notification notification(std::format("Process exited with code: {} ({})", exitCode, status));
        //NotificationManager::getInstance().addNotification(notification);
    }

    if (WIFSIGNALED(status)) {
        _status = CLEAR_FLAG(_status, IS_TRACE_STARTED);

        int signum = WTERMSIG(status);
        //Notification notification(std::format("Process terminated by signal: {} ({})", signum, strsignal(signum)));
        //NotificationManager::getInstance().addNotification(notification);
    }

    if ((status >> 8) == (SIGTRAP | (PTRACE_EVENT_CLONE << 8))) {
        int tid;
        ptrace(PTRACE_GETEVENTMSG, pid, NULL, &tid);
        //Notification notification(std::format("New thread (LWP {})", tid));
        //NotificationManager::getInstance().addNotification(notification);

        _threadPids.push_back(tid);

        ptrace(PTRACE_CONT, tid, NULL, NULL);
        ptrace(PTRACE_CONT, pid, NULL, NULL);
    }

    if ((status >> 8) == (SIGTRAP | (PTRACE_EVENT_EXIT << 8))) {
        //Notification notification(std::format("Thread (LWP {}) is exiting", pid));
        //NotificationManager::getInstance().addNotification(notification);
        std::erase(_threadPids, pid);
    }

    if (WIFSTOPPED(status)) {
        _status = CLEAR_FLAG(_status, IS_TRACE_RUNNING);

        int signum = WSTOPSIG(status);
        if (signum == SIGTRAP) {
            //Notification notification("Process hit a breakpoint (SIGTRAP).");
            //NotificationManager::getInstance().addNotification(notification);
            long rip = ptrace(PTRACE_PEEKUSER, _pid, 8 * RIP, NULL);
            for (auto& bp : _breakpoints) {
                if (bp.getAddress() == rip - 1) {
                    bp.restore();
                    break;
                }
            }
            return;
        }
        //Notification notification(std::format("Process stopped by signal: {} ({})", signum, strsignal(signum)));
        //NotificationManager::getInstance().addNotification(notification);
    }
}

Process::Process() : _pid(-1), _status(0) {
}

Process::~Process() {
}

void Process::setArguments(std::vector<std::string> args) {
    _arguments = args;
}

void Process::launch() {
    if (_arguments.empty()) {
        throw std::runtime_error("No program specified to execute.");
    }

    std::filesystem::path program = std::filesystem::absolute(_arguments[0]);

    char **argv = new char*[_arguments.size() + 1];
    memset(argv, 0, sizeof(char*) * (_arguments.size() + 1));
    for (size_t i = 0; i < _arguments.size(); ++i) {
        argv[i] = const_cast<char *>(_arguments[i].c_str());
    }

    if ((pipe(_stdinPipe) == -1) ||
        (pipe(_stdoutPipe) == -1) ||
        (pipe(_stderrPipe) == -1)) {
        throw std::runtime_error("Failed to create pipes for stdout/stderr.");
    }

    // Executing
    pid_t pid = fork();

    if (pid < 0) {
        throw std::runtime_error("Failed to fork process.");
    }
    if (pid == 0) {
        setpgid(0, 0);
        close(_stdinPipe[1]);
        dup2(_stdinPipe[0], STDIN_FILENO);
        close(_stdinPipe[0]);

        close(_stdoutPipe[0]);
        dup2(_stdoutPipe[1], STDOUT_FILENO);
        close(_stdoutPipe[1]);

        close(_stderrPipe[0]);
        dup2(_stderrPipe[1], STDERR_FILENO);
        close(_stderrPipe[1]);

        raise(SIGSTOP);
        execve(program.c_str(), argv, nullptr);
        exit(127);
    }
    close(_stdinPipe[0]);
    close(_stdoutPipe[1]);
    close(_stderrPipe[1]);

    int flags;

    flags = fcntl(_stdoutPipe[0], F_GETFL, 0);
    fcntl(_stdoutPipe[0], F_SETFL, flags | O_NONBLOCK);

    flags = fcntl(_stderrPipe[0], F_GETFL, 0);
    fcntl(_stderrPipe[0], F_SETFL, flags | O_NONBLOCK);

    delete[] argv;
    _pid = pid;

    // Parent process
    waitpid(_pid, nullptr, WUNTRACED);
    if (ptrace(PTRACE_SEIZE, _pid, NULL, NULL) == -1) {
        throw std::runtime_error("Failed to seize process with ptrace.");
    }
    ptrace(PTRACE_SETOPTIONS, _pid, NULL,
           PTRACE_O_EXITKILL | PTRACE_O_TRACECLONE | PTRACE_O_TRACEEXEC |
               PTRACE_O_TRACEEXIT | PTRACE_O_TRACESYSGOOD);
    ptrace(PTRACE_CONT, _pid, NULL, NULL);

    int status = 0;
    waitpid(_pid, &status, WUNTRACED);
    if ((status >> 8) == (SIGTRAP | (PTRACE_EVENT_EXEC << 8))) {
        _addressMap = AddressMap(program, _pid);
    }

    _status = SET_FLAG(_status, IS_TRACE_STARTED);
}

void Process::tick() {
    if (!HAS_FLAG(_status, IS_TRACE_STARTED))
        return;

    char buffer[256];
    ssize_t bytesRead;

    bytesRead = read(_stdoutPipe[0], buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        _stdoutLineBuffer += std::string(buffer);
    }
    for (auto it = _stdoutLineBuffer.begin(); it != _stdoutLineBuffer.end(); ) {
        if (*it == '\n') {
            std::string line(_stdoutLineBuffer.begin(), it);
            _stdoutBuffer.push_back(line);
            it = _stdoutLineBuffer.erase(_stdoutLineBuffer.begin(), it + 1);
        } else {
            ++it;
        }
    }

    bytesRead = read(_stderrPipe[0], buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        _stderrLineBuffer += std::string(buffer);
    }
    for (auto it = _stderrLineBuffer.begin(); it != _stderrLineBuffer.end(); ) {
        if (*it == '\n') {
            std::string line(_stderrLineBuffer.begin(), it);
            _stderrBuffer.push_back(line);
            it = _stderrLineBuffer.erase(_stderrLineBuffer.begin(), it + 1);
        } else {
            ++it;
        }
    }

    _getProcessStatus(_pid);
    for (auto tid : _threadPids) {
        _getProcessStatus(tid);
    }
}



void Process::addBreakpoint(Breakpoint breakpoint) {
    _breakpoints.push_back(breakpoint);
}

void Process::removeBreakpoint(Breakpoint breakpoint) {
    auto it = std::ranges::find(_breakpoints.begin(), _breakpoints.end(), breakpoint);
    if (it != _breakpoints.end())
        _breakpoints.erase(it);
}

void Process::clearBreakpoints() {
    _breakpoints.clear();
}

void Process::step() {
    if (!HAS_FLAG(_status, IS_TRACE_STARTED))
        return;

    static int status = 0;
    ptrace(PTRACE_SINGLESTEP, _pid, NULL, NULL);
    waitpid(_pid, &status, 0);
}

void Process::next() {
    if (!HAS_FLAG(_status, IS_TRACE_STARTED))
        return;

    long instruction = ptrace(PTRACE_PEEKUSER, _pid, 8 * RIP, NULL);

    if ((instruction & 0xFF) == 0xE8) {
        unsigned long nextInstruction = instruction + 5;
        auto command = std::shared_ptr<command::ICommand>(new command::AddBreakpointCommand(*this, {_pid, nextInstruction}));
        //CommandManager::getInstance().addCommand(command);
        ptrace(PTRACE_CONT, _pid, NULL, NULL);
    } else {
        ptrace(PTRACE_SINGLESTEP, _pid, NULL, NULL);
    }
}

void Process::continueExecution() {
    if (!HAS_FLAG(_status, IS_TRACE_STARTED))
        return;

    ptrace(PTRACE_CONT, _pid, NULL, NULL);
    for (auto tid : _threadPids) {
        ptrace(PTRACE_CONT, tid, NULL, NULL);
    }
    _status = SET_FLAG(_status, IS_TRACE_RUNNING);
}

void Process::pauseExecution() {
    if (!HAS_FLAG(_status, IS_TRACE_STARTED))
        return;

    ptrace(PTRACE_INTERRUPT, _pid, NULL, NULL);
    for (auto tid : _threadPids) {
        ptrace(PTRACE_INTERRUPT, tid, NULL, NULL);
    }
    waitpid(_pid, nullptr, WUNTRACED);
    for (auto tid : _threadPids) {
        waitpid(tid, nullptr, WUNTRACED);
    }
    _status = CLEAR_FLAG(_status, IS_TRACE_RUNNING);
}

std::vector<int> Process::getFDs() const {
    std::vector<int> fds;
    DIR *dir = opendir(std::format("/proc/{}/fd", _pid).c_str());
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        fds.push_back(atoi(entry->d_name));
    }
    closedir(dir);
    return fds;
}

FDInfo Process::getFDInfo(int fd) const {
    FDInfo info {};

    info.fd = fd;

    {
        char buf[PATH_MAX];
        size_t size = readlink(std::format("/proc/{}/fd/{}", _pid, fd).c_str(), buf, sizeof(buf) - 1);
        buf[size] = '\0';
        info.path = std::string(buf);
    }

    {
        std::ifstream fdinfoFile(std::format("/proc/{}/fdinfo/{}", _pid, fd));
        std::string line;

        std::getline(fdinfoFile, line);
        sscanf(line.c_str(), "pos: %d", &info.pos);
        std::getline(fdinfoFile, line);
        sscanf(line.c_str(), "flags: %o", &info.flags);
        std::getline(fdinfoFile, line);
        sscanf(line.c_str(), "mnt_id: %d", &info.mnt_id);
        std::getline(fdinfoFile, line);
        sscanf(line.c_str(), "ino: %d", &info.ino);
    }

    return info;
}

void Process::injectModule() {
    char path[PATH_MAX] = {};
    strcpy(path, "./file/tmp.so");

    struct user_regs_struct regs, original_regs;
    ptrace(PTRACE_GETREGS, _pid, NULL, &regs);
    memcpy(&original_regs, &regs, sizeof(regs));

    regs.rsp -= 128;

    size_t size = strlen(path) / 8 + 1;
    for (size_t i = 0; i < size; ++i) {
        ptrace(PTRACE_POKETEXT, _pid, regs.rsp + i * 8, *((long *)(path + i * 8)));
    }
    ptrace(PTRACE_POKETEXT, _pid, regs.rsp, path);
}

std::vector<std::shared_ptr<dwarf::Fde>> Process::getStacktrace() {
    std::vector<std::shared_ptr<dwarf::Fde>> stacktrace;

    Address rip = ptrace(PTRACE_PEEKUSER, _pid, 8 * RIP, NULL);
    auto rrip = getAddressMap().getRelativeAddress(rip);

    if (!rrip.has_value()) { return stacktrace; }

    auto fde = getAddressMap().getFile(rip)->getDwarfFile()->getFdeAtPc(rrip.value());
    if (!fde.has_value()) { return stacktrace; }
    stacktrace.push_back(fde.value());
    long cfa = 0;

    Dwarf_Small dw_value_type;
    Dwarf_Unsigned dw_offset_relevant;
    Dwarf_Unsigned dw_register;
    Dwarf_Signed dw_offset;
    Dwarf_Block dw_block_content;
    Dwarf_Addr dw_row_pc_out;
    Dwarf_Bool dw_has_more_rows;
    Dwarf_Addr dw_subsequent_pc;
    Dwarf_Error error{};


    dw_has_more_rows = true;
    int i = 5;


    while (dw_has_more_rows && i-- > 0) {
        // Fetching CFA
        int status = dwarf_get_fde_info_for_cfa_reg3_c(fde.value()->getFde(), rrip.value(), &dw_value_type, &dw_offset_relevant, &dw_register, &dw_offset, &dw_block_content, &dw_row_pc_out, &dw_has_more_rows, &dw_subsequent_pc, &error);
        switch (status) {
            case DW_DLV_OK:
                break;
            case DW_DLV_NO_ENTRY:
                return stacktrace;
            case DW_DLV_ERROR:
                return stacktrace;
        }
        if (dw_value_type == DW_EXPR_OFFSET) {
            long reg = ptrace(PTRACE_PEEKUSER, _pid, 8 * RegMapTable[dw_register].processorRegNum, NULL);
            cfa = reg + dw_offset;
        } else {
            // TODO handle
        }


        // Fetching return address
        try {
            if (dwarf_get_fde_info_for_reg3_c(fde.value()->getFde(), 16, rrip.value(), &dw_value_type, &dw_offset_relevant, &dw_register, &dw_offset, &dw_block_content, &dw_row_pc_out, &dw_has_more_rows, &dw_subsequent_pc, &error) != DW_DLV_OK) {
                break;
            }
            if (dw_value_type == DW_EXPR_OFFSET) {
                long returnAddress = cfa + dw_offset;
                rip = ptrace(PTRACE_PEEKDATA, _pid, returnAddress, NULL);
                auto rrip = getAddressMap().getRelativeAddress(rip);
                if (rip == 0 || rip == -1 || !rrip.has_value()) {
                    break;
                }

                try {
                    fde = getAddressMap().getFile(rip)->getDwarfFile()->getFdeAtPc(rrip.value());
                    stacktrace.push_back(fde.value());
                } catch (const std::exception& e) {
                    break;
                }
            } else {
                // TODO handle
                break;
            }
        } catch (const std::exception& e) {
            break;
        }

    }

    return stacktrace;
}
