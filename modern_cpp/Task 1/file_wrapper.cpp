#include <cstdio>

#include <stdexcept>
#include <string>

#include "file_wrapper.hpp"

FileWrapper::FileWrapper(const std::string &path, const std::string &flag) {
  file = ::fopen(path.c_str(), flag.c_str());
  if (!file)
    throw std::runtime_error("File failed to open\n");
}

FileWrapper::~FileWrapper() {
  if (file)
    ::fclose(file);
}

std::vector<std::string> FileWrapper::read() {

  std::vector<std::string> words;
  char buf[4096];
  while (std::fgets(buf, sizeof(buf), file))
    words.push_back(buf);

  return words;
}

void FileWrapper::write(const std::string &word) {
  if (fputs(word.c_str(), file) == EOF)
    throw std::runtime_error("Write failed");
}
