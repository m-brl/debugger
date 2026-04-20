#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include <linux/limits.h>

const char *module_name = "allocation";
const char *server_path = "./preload/allocation/allocation_server.so";
const char *client_path = "./preload/allocation/allocation_client.so";

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

int load(pid_t pid) {
    char fifo_path[256];

    snprintf(fifo_path, sizeof(fifo_path), "/tmp/allocation_fifo_%d", pid);
    fifo_fd = open(fifo_path, O_RDONLY);
    return fifo_fd == -1 ? -1 : 0;
}

int unload() {
    if (fifo_fd != -1) {
        close(fifo_fd);
    }
    return 0;
}

void tick() {
    struct allocation_info info;

    while (read(fifo_fd, &info, sizeof(info)) == sizeof(info)) {
        switch (info.type) {
            case ALLOC:
                break;
            case FREE:
                break;
        }
    }
}
