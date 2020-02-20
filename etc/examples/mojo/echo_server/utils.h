#include <string>
#include <vector>

namespace utils {
std::vector<std::string> split(const std::string& line,
                               char delimiter);

bool starts_with(const std::string& s, const std::string& predicate);
}  // namespace utils
