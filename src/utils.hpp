#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

//
// assertion
//

#define ASSERT(EXPR)                                                           \
  if (!static_cast<bool>(EXPR)) {                                              \
    std::ostringstream ostream;                                                \
    ostream << "[" << __FILE__ << ":" << __LINE__ << "] " << #EXPR;            \
    throw std::runtime_error{ostream.str()};                                   \
  }

//
// pretty print
//
namespace std {

template <class... Ts>
ostream &operator<<(ostream &ostr, const tuple<Ts...> &xs) {
  ostr << "(";
  bool sep = false;
  apply(
      [&](auto &&...x) { ((ostr << (sep ? ", " : "") << x, sep = true), ...); },
      xs);
  return ostr << ")";
}

template <class T1, class T2>
ostream &operator<<(ostream &ostr, const pair<T1, T2> &x) {
  return ostr << tie(x.first, x.second);
}

template <class T, class = decltype(begin(declval<T>())),
          class = enable_if_t<!is_same<T, string>::value>>
ostream &operator<<(ostream &ostr, const T &xs) {
  ostr << "{";
  bool sep = false;
  for (auto &x : xs) {
    ostr << (sep ? ", " : "") << x;
    sep = true;
  }
  return ostr << "}";
}

} // namespace std

#define dbg(...)                                                               \
  do {                                                                         \
    std::cout << #__VA_ARGS__ ": " << std::make_tuple(__VA_ARGS__)             \
              << std::endl;                                                    \
  } while (0)

//
// file io
//

namespace utils {

std::vector<uint8_t> read_file(const std::string &filename) {
  std::ifstream istr(filename, std::ios::binary);
  ASSERT(istr.is_open());
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(istr)),
                            std::istreambuf_iterator<char>());
  return data;
}

} // namespace utils

//
// cli
//

namespace utils {

struct Cli {
  const int argc;
  const char **argv;

  template <typename T> T parse(const char *s) {
    std::istringstream stream{s};
    T result;
    stream >> result;
    return result;
  }

  template <typename T> std::optional<T> argument(const std::string &flag) {
    for (auto i = 1; i < argc; i++) {
      if (argv[i] == flag && i + 1 < argc) {
        return std::optional{parse<T>(argv[i + 1])};
      }
    }
    return {};
  }
};

} // namespace utils
