#include <CD74HC4067.h>
#include "Diagnostics.h"
#include "KeyboardState.h"
#include "KeyboardSession.h"
#include "KeyboardPins.h"
#include "BatteryAdcConfig.h"
#include "BatteryAdcSource.h"
#include <esp_timer.h>

const int mux_pin_row = keyboardPins::rowSignal;
const int mux_pin_col = keyboardPins::colSignal;

CD74HC4067 muxRow(keyboardPins::rowS0, keyboardPins::rowS1, keyboardPins::rowS2, keyboardPins::rowS3);
CD74HC4067 muxCol(keyboardPins::colS0, keyboardPins::colS1, keyboardPins::colS2, keyboardPins::colS3);

#define PINS_NUM 26

#define PINS_ROW 8   //OUTPUT
#define PINS_COL 18  //INPUT

uint8_t pinRows[PINS_ROW] = { 128, 129, 130, 131, 132, 133, 134, 135 };
uint8_t pinCols[PINS_COL] = { 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, keyboardPins::directCol0, keyboardPins::directCol1 };

uint16_t delayMicrosInRead = 20;
uint16_t delayMicrosInWrite = 20;

PhysicalMatrix rawSamples = {};
KeyboardState keyboardState;
KeyboardSession keyboardSession;
static_assert(PINS_ROW == 8 && PINS_COL == 18, "Physical matrix dimensions must match");


//====================================================================================================


// Raw HID usages; GPIO sampling is independent of BLE connection state.
#include "KeyboardHid.h"
#include "KeyboardLayout.h"
const auto& keyMatrix = keyboardLayout::matrix;
KeyboardHid bleKeyboard;
BatteryAdcSource* createAdcBattery() {
  if (!batteryAdcConfig::enabled) return nullptr;
  static BatteryAdcSource source(batteryAdcConfig::configuration());
  return &source;
}
BatteryAdcSource* const adcBattery = createAdcBattery();
#ifdef KEYBOARD_BATTERY_COMPILE_CHECK
#include "../tools/battery_compile_fixture.h"
static_assert(!batteryAdcConfig::enabled, "Never mix ADC backend and synthetic battery fixture");
BatterySource* const batterySource = &batteryCompileFixture;
#else
BatterySource* const batterySource = adcBattery;
#endif

//====================================================================================================

void readCols(uint8_t channelRow, PhysicalMatrix& samples) {
  int pinToRead = 0;
  uint8_t channelCol = 0;
  uint8_t pinStat = 0;
  
  for (int i = 0; i < PINS_COL; i++) {
    if (pinCols[i] >= 128 + 16 && pinCols[i] < 128 + 16 + 16) {
      channelCol = pinCols[i] - (128 + 16);
      muxCol.channel(channelCol);
      pinToRead = mux_pin_col;
    } else {
      pinToRead = pinCols[i];
      channelCol = pinToRead - 2; //6/trüük ez itt most 18 és 19 pes inre h sorban jönnenek 15 után
    }
    delayMicroseconds(delayMicrosInRead);

    pinStat = digitalRead(pinToRead);

    

    samples[channelRow][channelCol] = pinStat != 0;


  }

}

//====================================================================================================

void sampleMatrix(PhysicalMatrix& samples) {
  uint8_t channelRow;
  for (int i = 0; i < PINS_ROW; i++) {
    channelRow = pinRows[i] - 128;
    muxRow.channel(channelRow);
    delayMicroseconds(delayMicrosInWrite);
    readCols(channelRow, samples);
  }
}

//====================================================================================================

void pinSetup() {
  pinMode(mux_pin_row, OUTPUT);          //seting row as output
  pinMode(mux_pin_col, INPUT_PULLDOWN);  //setting col as input

  pinMode(keyboardPins::directCol0, INPUT_PULLDOWN);
  pinMode(keyboardPins::directCol1, INPUT_PULLDOWN);

  digitalWrite(mux_pin_row, HIGH);
  
}

//====================================================================================================
void setup() {
  delay(100);
  Serial.setTxBufferSize(1024);
  Serial.begin(115200);
  diagnostics::begin();

  delay(10);
  delay(100);
  pinSetup();
  if (batteryAdcConfig::enabled) {
    const bool ready = adcBattery->begin();
    Serial.printf("BATTERY_ADC init=%d status=%u calibration=efuse_TP_required fallback=none\n",
                  ready, static_cast<unsigned>(adcBattery->status()));
    for (int i = 0; i < 4; ++i) {
      adcBattery->poll(static_cast<uint32_t>(millis()));
      delay(30);
    }
  }
  delay(100);
  // #ifdef BLE
  diagnostics::stackInit(bleKeyboard.begin(batterySource));
  // #endif

}

void loop() {
  const int64_t loopStart = esp_timer_get_time();
  const auto ota = bleKeyboard.otaStatus();
  if (!ota.active()) {
    const int64_t scanStart = esp_timer_get_time();
    sampleMatrix(rawSamples);
    const auto scan = keyboardState.processScan(rawSamples, static_cast<uint32_t>(millis()), keyMatrix);
    diagnostics::scanProcessed(scan.stableChanges, scan.reportChanged);
    const uint32_t nowMs = static_cast<uint32_t>(millis());
    keyboardSession.update(bleKeyboard.prepareSession(), keyboardState, keyMatrix, nowMs);
    KeyboardSession::Request request;
    if (keyboardSession.nextAttempt(nowMs, request)) {
      const auto result = bleKeyboard.submitReport(request.report, request.generation);
      keyboardSession.complete(request, result, bleKeyboard.linkSnapshot());
    }
    diagnostics::session(keyboardSession.metrics());
    diagnostics::scanDuration(esp_timer_get_time() - scanStart);
    if (batteryAdcConfig::enabled) adcBattery->poll(static_cast<uint32_t>(millis()));
    bleKeyboard.pollBattery(static_cast<uint32_t>(millis())); // Keyboard always has priority.
  }
  diagnostics::ota(ota);
  bleKeyboard.pollOta();
  delay(10); // Preserve cooperative scheduling; scan continues even without a connection.
  diagnostics::poll();
  diagnostics::loopDuration(esp_timer_get_time() - loopStart);
}
