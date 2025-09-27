#pragma once

#include <filesystem>
#include <vector>
#include <string>
#include <sys/stat.h>

class SourceFile {
    private:
        std::filesystem::path _path;
        struct stat _fileStat;
        std::vector<std::string> _lines;

    public:
        explicit SourceFile(std::filesystem::path path);
        ~SourceFile() = default;

        void load();
        bool isUpToDate();
        std::string getline(std::size_t lineNumber) const;
        std::vector<std::string> getlines(std::size_t startLine, std::size_t endLine) const;
        std::vector<std::string> getlines() const;
        std::size_t getlineCount() const;
        std::string operator[](std::size_t index) const;
};

struct LineLocation {
    std::shared_ptr<SourceFile> file;
    std::size_t lineNumber;
};
