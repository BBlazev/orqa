#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <vector>

class FileWrapper {
public:
  FileWrapper(const std::string &path, const std::string &flag);
  ~FileWrapper();

  std::vector<std::string> read();
  void write(const std::string &path_to_write);

  FileWrapper(const FileWrapper &other) = delete;
  FileWrapper &operator=(const FileWrapper &other) = delete;

  FileWrapper(FileWrapper &&other) = delete;
  FileWrapper &operator=(FileWrapper &&other) = delete;

private:
  FILE *file;
};
