#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

using namespace emscripten;

EMSCRIPTEN_BINDINGS(emscripten_00) {
  register_vector<uint8_t>("Vector");
}
