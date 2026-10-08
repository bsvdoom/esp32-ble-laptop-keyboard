#pragma once
#include <stdint.h>

// Own the getValue() result until the checked byte has been copied.
template <typename Characteristic>
bool readKeyboardLed(Characteristic* characteristic, uint8_t& led) {
  const auto value = characteristic->getValue();
  if (value.length() == 0) return false;
  led = static_cast<uint8_t>(value[0]);
  return true;
}
