#include "Module.hpp"

Module::Module(std::filesystem::path path) : _path(path) {
    _module = dlopen(_path.c_str(), RTLD_LAZY);
    if (_module == nullptr) {
        throw ModuleNotFound("Failed to load module");
    }

    _load_module = (void (*)(pid_t))dlsym(_module, "load");
    _unload_module = (void (*)())dlsym(_module, "unload");
    _tick_module = (void (*)())dlsym(_module, "tick");
}

Module::~Module() {
    if (_module != nullptr) {
        dlclose(_module);
    }
}
