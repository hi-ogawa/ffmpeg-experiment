#include <stdexcept>
#include <sstream>
#include <tuple>

#define ASSERT(EXPR)                                                 \
  if(!static_cast<bool>(EXPR)) {                                    \
    std::ostringstream ostream;                                     \
    ostream << "[" << __FILE__ << ":" << __LINE__ << "] " << #EXPR; \
    throw std::runtime_error{ostream.str()};                        \
  }


// quick pretty print
namespace std {
  template<class ...Ts>        ostream& operator<<(ostream& o, const tuple<Ts...>& x) { o << "("; bool b = 0; apply([&](auto&&... y){ ((o << (b ? ", " : "") << y, b = 1), ...); }, x); return o << ")"; }
  template<class T1, class T2> ostream& operator<<(ostream& o, const pair<T1, T2>& x) { return o << tie(x.first, x.second); }
  template<class T, class = decltype(begin(declval<T>())), class = enable_if_t<!is_same<T, string>::value>>
  ostream& operator<<(ostream& o, const T& x) { o << "{"; bool b = 0; for (auto& y : x) { o << (b ? ", " : "") << y; b = 1; } return o << "}"; }
}

#define dbg(...) do { std::cout << #__VA_ARGS__ ": " << std::make_tuple(__VA_ARGS__) << std::endl; } while (0)
