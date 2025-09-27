#pragma once

#include "utils.hpp"
#include <exception>
#include <filesystem>
#include <vector>
#include <memory>
#include <sys/stat.h>
#include <functional>

#include "Section.hpp"

namespace ELF {
class File {
public:
  class FileNotFound : public std::exception {
  private:
    std::string _message;

  public:
    FileNotFound(std::string message) : _message(std::move(message)) {}

    [[nodiscard]] const char *what() const noexcept override {
      return this->_message.c_str();
    }
  };

private:
  std::filesystem::path _path;
  int _fd;
  struct stat _fileStat;
  Elf_Ehdr _ehdr;
  char _elfClass;

  std::vector<Section> _sections;

  [[nodiscard]] Section _createSection(Elf_Ehdr ehdr, int sectionIndex) const;

  void _parseFile();
  void _openFile();

public:
  explicit File(std::filesystem::path path);
  ~File() = default;

  Section getSectionByName(std::string name);
};
} // namespace ELF
