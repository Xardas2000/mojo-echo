#include "utils.h"

namespace utils {
std::vector<std::string> split(const std::string& line, char delimiter) {
  std::vector<std::string> strs;
  std::string word;
  for (char c: line) {
    if (c == delimiter) {
      strs.push_back(word);
      word.clear();
    } else {
      word += c;
    }
  }
  if (!word.empty())
    strs.push_back(word);
  return strs;
}

bool starts_with(const std::string& s, const std::string& predicate) {
  return s.find(predicate) == 0;
}
}  // namespace utils
