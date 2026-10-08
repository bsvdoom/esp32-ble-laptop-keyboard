#include "../include/KeyboardLedValue.h"
#include <cassert>
#include <string>

struct Characteristic {
  std::string bytes;
  std::string getValue() const { return bytes; }
};

int main() {
  Characteristic empty{std::string()};
  uint8_t led = 0x55;
  assert(!readKeyboardLed(&empty, led));
  assert(led == 0x55);
  Characteristic one{std::string(1, static_cast<char>(0x80))};
  assert(readKeyboardLed(&one, led));
  assert(led == 0x80);
  one.bytes[0] = 0;
  assert(readKeyboardLed(&one, led));
  assert(led == 0);
}
