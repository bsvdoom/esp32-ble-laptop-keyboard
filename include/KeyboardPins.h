#pragma once

// Existing wiring only; shared with ADC conflict validation. No pin reassignment.
namespace keyboardPins {
constexpr int rowSignal = 6, colSignal = 7;
constexpr int rowS0 = 9, rowS1 = 8, rowS2 = 5, rowS3 = -1;
constexpr int colS0 = 4, colS1 = 3, colS2 = 2, colS3 = 1;
constexpr int directCol0 = 18, directCol1 = 19;
// Serial.begin defaults in the pinned esp32c3 Arduino variant.
constexpr int serialTx = 21, serialRx = 20;
constexpr bool used(int pin) {
  return pin == rowSignal || pin == colSignal || pin == rowS0 || pin == rowS1 ||
         pin == rowS2 || pin == colS0 || pin == colS1 || pin == colS2 ||
         pin == colS3 || pin == directCol0 || pin == directCol1 ||
         pin == serialTx || pin == serialRx;
}
constexpr bool freeAdc1(int pin) { return pin >= 0 && pin <= 4 && !used(pin); }
}
