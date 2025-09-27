#include "File.hpp"
#include "Logger.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace ELF {
Section File::_createSection(Elf_Ehdr ehdr, int sectionIndex) const {
  Section section{};

  if (this->_elfClass == ELFCLASS32) {
    section.headerOff = ehdr.elf32_ehdr->e_shoff + (sectionIndex * sizeof(Elf32_Shdr));
    section.header = reinterpret_cast<Elf32_Shdr*>(ehdr.buffer + std::get<Elf32_Off>(section.headerOff));
    section.contentOff = std::get<Elf32_Shdr*>(section.header)->sh_offset;
    section.content = ehdr.buffer + std::get<Elf32_Off>(section.contentOff);
  } else if (this->_elfClass == ELFCLASS64) {
    section.headerOff = ehdr.elf64_ehdr->e_shoff + (sectionIndex * sizeof(Elf64_Shdr));
    section.header = reinterpret_cast<Elf64_Shdr*>(ehdr.buffer + std::get<Elf64_Off>(section.headerOff));
    section.contentOff = std::get<Elf64_Shdr*>(section.header)->sh_offset;
    section.content = ehdr.buffer + std::get<Elf64_Off>(section.contentOff);
  }
  return section;
}

void File::_parseFile() {
  Elf64_Shdr *shstrtab = reinterpret_cast<Elf64_Shdr *>(this->_ehdr.buffer + this->_ehdr.elf64_ehdr->e_shoff + (this->_ehdr.elf64_ehdr->e_shstrndx * sizeof(Elf64_Shdr)));

  for (int sectionIndex = 0; sectionIndex < this->_ehdr.elf64_ehdr->e_shnum; sectionIndex++) {
    //auto section = createSection(this->_ehdr, sectionIndex);
    //this->_sections.push_back(section);
    continue;
    Elf64_Addr addr = this->_ehdr.elf64_ehdr->e_shoff + (sectionIndex * sizeof(Elf64_Shdr));
    Elf64_Shdr *shdr = reinterpret_cast<Elf64_Shdr *>(this->_ehdr.buffer + addr);
    const char *name = this->_ehdr.buffer + shstrtab->sh_offset + shdr->sh_name;
    Logger::log(name);
  }
}

void File::_openFile() {
  this->_fd = open(this->_path.c_str(), O_RDONLY);
  fstat(this->_fd, &this->_fileStat);

  this->_ehdr.buffer = reinterpret_cast<char *>(mmap(nullptr, this->_fileStat.st_size, PROT_READ, MAP_PRIVATE, this->_fd, 0));
  this->_elfClass = this->_ehdr.buffer[EI_CLASS];
}

File::File(std::filesystem::path path) : _path(path), _fd(-1), _fileStat(0), _ehdr({}) {
  if (!std::filesystem::exists(path)) {
    throw FileNotFound("File not found");
  }
  this->_openFile();
  this->_parseFile();
}

Section File::getSectionByName(std::string name) {
  for (auto& section: this->_sections) {
    /*char *sectionName = this->_sections[this->_ehdr.elf64_ehdr->e_shstrndx].content + section.header->sh_name;
    if (name == std::string(sectionName)) {
      Logger::log(std::format(">{} - {}<", sectionName, name));
      return section;
    }*/
  }
  throw std::runtime_error("Section not found");
}
} // namespace ELF
