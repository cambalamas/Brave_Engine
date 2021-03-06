#include "cpptools_Strings.hpp"

#include <fstream>
#include <locale>

namespace imog {

// ====================================================================== //
// ====================================================================== //
// Get as std::string the content of a given file
// ====================================================================== //

std::string Strings::fromFile(const std::string& filePath) {
  auto stream  = std::ifstream(filePath);
  using CharIt = std::istreambuf_iterator<char>;
  return std::string(CharIt(stream), CharIt());
}

// ====================================================================== //
// ====================================================================== //
// Split gives a vector with splitted strings
// ====================================================================== //

std::vector<std::string> Strings::split(const std::string& str,
                                        const std::string& delimeter) {
  std::string              token;
  std::vector<std::string> splitted;
  size_t                   ini{0}, end{0};

  // Split and store the string body
  while ((end = str.find(delimeter, ini)) <= str.size()) {
    token = str.substr(ini, end - ini);
    ini   = end + delimeter.size();
    splitted.push_back(token);
  }

  // Store the string tail
  if (ini < str.size()) {
    token = str.substr(ini);
    splitted.push_back(token);
  }

  return splitted;
}

// ====================================================================== //
// ====================================================================== //
// Transform all string chars to lowercase
// ====================================================================== //

std::string Strings::toLower(const std::string& str) {
  std::locale loc;
  std::string out;

  for (auto c : str) out += std::tolower(c, loc);

  return out;
}

// ====================================================================== //
// ====================================================================== //
// Contains ?
// ====================================================================== //

bool Strings::contains(const std::string& str, const std::string& substr) {
  return str.find(substr) != std::string::npos;
}

} // namespace imog
