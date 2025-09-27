#include "ExecutionWorkflow.hpp"
#include "ContextManager.hpp"
#include "Logger.hpp"
#include "Notification.hpp"
#include "Command.hpp"

#include <cstring>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>

void ExecutionWorkflow::_getProcessStatus(pid_t pid) {
    int status = 0;
    pid_t result = waitpid(pid, &status, WNOHANG | WUNTRACED);
    if (result == 0 || result == -1) {
        return;
    }

    if (WIFEXITED(status)) {
        _status = CLEAR_FLAG(_status, IS_TRACE_STARTED);

        int exitCode = WEXITSTATUS(status);
        Notification notification(std::format("Process exited with code: {} ({})", exitCode, status));
        NotificationManager::getInstance().addNotification(notification);
    }

    if (WIFSIGNALED(status)) {
        _status = CLEAR_FLAG(_status, IS_TRACE_STARTED);

        int signum = WTERMSIG(status);
        Notification notification(std::format("Process terminated by signal: {} ({})", signum, strsignal(signum)));
        NotificationManager::getInstance().addNotification(notification);
    }

    if ((status >> 8) == (SIGTRAP | (PTRACE_EVENT_CLONE << 8))) {
        int tid;
        ptrace(PTRACE_GETEVENTMSG, pid, NULL, &tid);
        Notification notification(std::format("New thread (LWP {})", tid));
        NotificationManager::getInstance().addNotification(notification);

        _threadPids.push_back(tid);

        ptrace(PTRACE_CONT, tid, NULL, NULL);
        ptrace(PTRACE_CONT, pid, NULL, NULL);
    }

    if ((status >> 8) == (SIGTRAP | (PTRACE_EVENT_EXIT << 8))) {
        Notification notification(std::format("Thread (LWP {}) is exiting", pid));
        NotificationManager::getInstance().addNotification(notification);
        std::erase(_threadPids, pid);
    }

    if (WIFSTOPPED(status)) {
        _status = CLEAR_FLAG(_status, IS_TRACE_RUNNING);

        int signum = WSTOPSIG(status);
        if (signum == SIGTRAP) {
            Notification notification("Process hit a breakpoint (SIGTRAP).");
            NotificationManager::getInstance().addNotification(notification);
            long rip = ptrace(PTRACE_PEEKUSER, _pid, 8 * RIP, NULL);
            for (auto& bp : _breakpoints) {
                if (bp.getAddress() == rip - 1) {
                    bp.restore();
                    break;
                }
            }
            return;
        }
        Notification notification(std::format("Process stopped by signal: {} ({})", signum, strsignal(signum)));
        NotificationManager::getInstance().addNotification(notification);
    }
}

ExecutionWorkflow::ExecutionWorkflow() {
}

ExecutionWorkflow::~ExecutionWorkflow() {
}

void ExecutionWorkflow::setArguments(std::vector<std::string> args) {
    _arguments = args;
}

void ExecutionWorkflow::launch() {
    if (_arguments.empty()) {
        throw std::runtime_error("No program specified to execute.");
    }

    std::filesystem::path program = std::filesystem::absolute(_arguments[0]);

    char **argv = new char*[_arguments.size() + 1];
    for (size_t i = 0; i < _arguments.size(); ++i) {
        argv[i] = const_cast<char *>(_arguments[i].c_str());
    }
    argv[_arguments.size()] = nullptr;

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
    waitpid(_pid, nullptr, WUNTRACED);
    _addressMap = AddressMap(program, _pid);

    _status = SET_FLAG(_status, IS_TRACE_STARTED);
}

void ExecutionWorkflow::tick() {
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



void ExecutionWorkflow::addBreakpoint(Breakpoint breakpoint) {
    _breakpoints.push_back(breakpoint);
}

void ExecutionWorkflow::removeBreakpoint(Breakpoint breakpoint) {
    auto it = std::ranges::find(_breakpoints.begin(), _breakpoints.end(), breakpoint);
    if (it != _breakpoints.end())
        _breakpoints.erase(it);
}

void ExecutionWorkflow::clearBreakpoints() {
    _breakpoints.clear();
}

void ExecutionWorkflow::step() {
    if (!HAS_FLAG(_status, IS_TRACE_STARTED))
        return;

    static int status = 0;
    ptrace(PTRACE_SINGLESTEP, _pid, NULL, NULL);
    waitpid(_pid, &status, 0);
}

void ExecutionWorkflow::next() {
    if (!HAS_FLAG(_status, IS_TRACE_STARTED))
        return;

    long instruction = ptrace(PTRACE_PEEKUSER, _pid, 8 * RIP, NULL);

    if ((instruction & 0xFF) == 0xE8) {
        unsigned long nextInstruction = instruction + 5;
        std::shared_ptr<ICommand> command = std::shared_ptr<ICommand>(new AddBreakpointCommand(*this, {_pid, nextInstruction}));
        CommandManager::getInstance().addCommand(command);
        ptrace(PTRACE_CONT, _pid, NULL, NULL);
    } else {
        ptrace(PTRACE_SINGLESTEP, _pid, NULL, NULL);
    }
}

void ExecutionWorkflow::continueExecution() {
    if (!HAS_FLAG(_status, IS_TRACE_STARTED))
        return;

    ptrace(PTRACE_CONT, _pid, NULL, NULL);
    for (auto tid : _threadPids) {
        ptrace(PTRACE_CONT, tid, NULL, NULL);
    }
    _status = SET_FLAG(_status, IS_TRACE_RUNNING);
}

void ExecutionWorkflow::pauseExecution() {
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
    CLEAR_FLAG(_status, IS_TRACE_RUNNING);
}
