#pragma once

#include <cstdio>
#include <string>
#include <vector>

/**
 * @brief RAII wrapper around a C stdio FILE* handle.
 *
 * Owns exactly one FILE*: the constructor opens it, the destructor closes it.
 * This guarantees the handle is released on every exit path -- including
 * exceptions thrown after construction -- which a bare fopen/fclose pair does
 * not. The type is move- and copy-disabled (see below) so ownership is unique
 * and unambiguous: there is never a second object that could double-fclose.
 *
 */
class FileWrapper {
public:
  /**
   * @brief Open `path` with stdio mode `flag`.
   * @param path Filesystem path to open.
   * @param flag fopen-style mode string ("r", "w", "a", "rb", ...). The caller
   *        is responsible for matching the mode to the operations they intend:
   *        calling read() on a "w" handle or write() on an "r" handle fails at
   *        the stdio level.
   * @throws std::runtime_error if the file cannot be opened (fopen returns null,
   *         e.g. missing file in "r" mode or insufficient permissions).
   */
  FileWrapper(const std::string &path, const std::string &flag);

  ~FileWrapper();

  /**
   * @brief Read the whole file as a vector of lines.
   * @return One std::string per line; the trailing '\n' is retained (fgets
   *         keeps it). A final line without a newline is still returned.
   * @note Lines longer than the internal 4096-byte buffer are split across
   *       multiple vector entries rather than truncated.
   */
  std::vector<std::string> read();

  /**
   * @brief Write `word` verbatim to the file (no added newline).
   * @param word Text to append at the current write position.
   * @throws std::runtime_error if the underlying fputs fails.
   */
  void write(const std::string &word);

  // Copying would duplicate ownership of one FILE*, leading to a double
  // fclose; moving is disabled too to keep the type strictly single-owner and
  // non-relocatable. Delete all four so misuse is a compile error, not a crash.
  FileWrapper(const FileWrapper &other) = delete;
  FileWrapper &operator=(const FileWrapper &other) = delete;

  FileWrapper(FileWrapper &&other) = delete;
  FileWrapper &operator=(FileWrapper &&other) = delete;

private:
  FILE *file; 
};