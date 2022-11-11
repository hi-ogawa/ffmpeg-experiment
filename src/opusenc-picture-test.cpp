#include "opusenc-picture.hpp"
#include "utils.hpp"

int main(const int argc, const char* argv[]) {
  utils::Cli cli{argc, argv};
  auto inFile = cli.argument("--in");
  ASSERT(inFile);

  auto data = utils::readFile(inFile.value());
  auto encoded = opusenc_picture::encode(data);
  dbg(encoded.size());
  std::puts(encoded.c_str());

  return 0;
}

/*
cmake --build build/native/Debug -- opusenc-picture-test
./build/native/Debug/opusenc-picture-test --in ./misc/test.jpg
*/
