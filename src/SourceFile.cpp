#include "SourceFile.hpp"
#include <fstream>

SourceFile::SourceFile(std::filesystem::path path) : _path(path) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Source file not found");
    }
    load();
}

void SourceFile::load() {
    _lines.clear();

    stat(this->_path.c_str(), &this->_fileStat);

    std::ifstream fileStream(_path);
    std::string line;
    while (std::getline(fileStream, line)) {
        this->_lines.push_back(line);
    }
}

bool SourceFile::isUpToDate() {
    struct stat currentStat{};
    stat(this->_path.c_str(), &currentStat);
    return currentStat.st_mtime == this->_fileStat.st_mtime;
}

std::string SourceFile::getline(std::size_t lineNumber) const {
    if (lineNumber == 0 || lineNumber > _lines.size()) {
        throw std::out_of_range("Line number out of range");
    }
    return _lines[lineNumber - 1];
}

std::vector<std::string> SourceFile::getlines(std::size_t startLine, std::size_t endLine) const {
    if (startLine == 0 || endLine > _lines.size() || startLine > endLine) {
        throw std::out_of_range("Line numbers out of range");
    }
    return std::vector<std::string>(_lines.begin() + startLine - 1, _lines.begin() + endLine);
}

std::vector<std::string> SourceFile::getlines() const {
    return _lines;
}

std::size_t SourceFile::getlineCount() const {
    return _lines.size();
}

std::string SourceFile::operator[](std::size_t index) const {
    return _lines[index];
}
