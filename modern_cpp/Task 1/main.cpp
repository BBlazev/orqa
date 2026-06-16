#include "file_wrapper.hpp"

int main() {
  FileWrapper file1("input.txt", "r");
  FileWrapper file2("output.txt", "w");

  std::vector<std::string> v = file1.read();

  for (auto &x : v)
    file2.write(x);

  return 0;
}
