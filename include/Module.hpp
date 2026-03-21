#pragma once

#include <dlfcn.h>
#include <filesystem>

class Module {
    private:
        std::filesystem::path _path;
        void *_module;

        char *_module_name;
        char *_client_path;
        void (*_load_module)(pid_t pid);
        void (*_unload_module)();
        void (*_tick_module)();

    public:
        class ModuleNotFound : public std::runtime_error {
            using std::runtime_error::runtime_error;
        };

        Module(std::filesystem::path path);
        ~Module();
};
