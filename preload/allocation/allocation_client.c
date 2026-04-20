#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <linux/limits.h>

enum allocation_type {
    ALLOC,
    FREE
};

struct allocation_info {
    enum allocation_type type;
    size_t size;
    void *ptr;
};

static int fifo_fd = -1;

__attribute__((constructor)) void ctor() {
    pid_t pid = getpid();
    char fifo_path[PATH_MAX];

    snprintf(fifo_path, sizeof(fifo_path), "/tmp/allocation_fifo_%d", pid);
    fifo_fd = open(fifo_path, O_WRONLY);
}

__attribute__((destructor)) void dtor() {
    if (fifo_fd != -1) {
        close(fifo_fd);
    }
}

extern void *__real_malloc(size_t size);

void *__wrap_malloc(size_t size) {
    struct allocation_info info;
    void *ptr = __real_malloc(size);

    if (ptr == NULL) {
        return NULL;
    }

    info.type = ALLOC;
    info.size = size;
    info.ptr = ptr;
    write(fifo_fd, &info, sizeof(info));
    return ptr;
}

extern void __real_free(void *ptr);

void __wrap_free(void *ptr) {
    struct allocation_info info;

    info.type = FREE;
    info.ptr = ptr;
    write(fifo_fd, &info, sizeof(info));

    __real_free(ptr);
}
