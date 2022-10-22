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

#define ASSERT(EXPR)                                                \
  if (!static_cast<bool>(EXPR)) {                                   \
    std::ostringstream ostream;                                     \
    ostream << "[" << __FILE__ << ":" << __LINE__ << "] " << #EXPR; \
    throw std::runtime_error{ostream.str()};                        \
  }

//
// pretty print
//
namespace std {

// std::tuple
template <class... Ts>
ostream& operator<<(ostream& ostr, const tuple<Ts...>& xs) {
  ostr << "(";
  bool sep = false;
  apply(
      [&](auto&&... x) { ((ostr << (sep ? ", " : "") << x, sep = true), ...); },
      xs);
  return ostr << ")";
}

// std::pair
template <class T1, class T2>
ostream& operator<<(ostream& ostr, const pair<T1, T2>& x) {
  return ostr << tie(x.first, x.second);
}

// "container" except std::string
template <class T,
          class = decltype(begin(declval<T>())),
          class = enable_if_t<!is_same<T, string>::value>>
ostream& operator<<(ostream& ostr, const T& xs) {
  ostr << "{";
  bool sep = false;
  for (auto& x : xs) {
    ostr << (sep ? ", " : "") << x;
    sep = true;
  }
  return ostr << "}";
}

}  // namespace std

#define dbg(...)                                                   \
  do {                                                             \
    std::cout << #__VA_ARGS__ ": " << std::make_tuple(__VA_ARGS__) \
              << std::endl;                                        \
  } while (0)

//
// file io
//

namespace utils {

std::vector<uint8_t> readFile(const std::string& filename) {
  std::ifstream istr(filename, std::ios::binary);
  ASSERT(istr.is_open());
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(istr)),
                            std::istreambuf_iterator<char>());
  return data;
}

void writeFile(const std::string& filename, const std::vector<uint8_t>& data) {
  std::ofstream ostr(filename, std::ios::binary);
  ASSERT(ostr.is_open());
  ostr.write(reinterpret_cast<const char*>(data.data()), data.size());
}

}  // namespace utils

//
// cli
//

namespace utils {

struct Cli {
  const int argc;
  const char** argv;
  std::vector<std::string> flags = {};

  std::string help() {
    std::ostringstream ostr;
    ostr << argv[0];
    for (auto flag : flags) {
      ostr << " (" << flag << " ?)";
    }
    return ostr.str();
  }

  template <typename T>
  T parse(const char* s) {
    std::istringstream stream{s};
    T result;
    stream >> result;
    return result;
  }

  template <typename T>
  std::optional<T> argument(const std::string& flag) {
    flags.push_back(flag);
    for (auto i = 1; i < argc; i++) {
      if (argv[i] == flag && i + 1 < argc) {
        return std::optional{parse<T>(argv[i + 1])};
      }
    }
    return {};
  }
};

}  // namespace utils

//
// RAII wrapper (cf. https://stackoverflow.com/a/42060129)
//

template <class F>
struct defer {
  F f;
  ~defer() { f(); }
};

struct defer_helper {
  template <class F>
  defer<F> operator*=(F f) {
    return {f};
  }
};

#define _DEFER_VAR2(x) _defer_var_##x
#define _DEFER_VAR1(x) _DEFER_VAR2(x)
#define DEFER auto _DEFER_VAR1(__LINE__) = defer_helper{} *= [&]()
