#include <CD74HC4067.h>

const int mux_pin_row = 6;  // select a pin to share with the 16 channels of the CD74HC4067
const int mux_pin_col = 7;  // select a pin to share with the 16 channels of the CD74HC4067

CD74HC4067 muxRow(9, 8, 5, -1);
CD74HC4067 muxCol(4, 3, 2, 1);

#define PINS_NUM 26

#define PINS_ROW 8   //OUTPUT
#define PINS_COL 18  //INPUT

uint8_t pinRows[PINS_ROW] = { 128, 129, 130, 131, 132, 133, 134, 135 };
uint8_t pinCols[PINS_COL] = { 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 18, 19 };

uint16_t delayMicrosInRead = 20;
uint16_t delayMicrosInWrite = 20;

bool keyStatusMatrix[PINS_ROW][PINS_COL] = {};


//====================================================================================================
#include "secret.h"


// #ifdef BLE
  #include <BleKeyboard.h>
// #endif

const uint8_t keyMatrix[PINS_ROW][PINS_COL] = {
/*0*/ { 0, 0, KEY_RIGHT_ARROW, KEY_U_ACUTE, KEY_U_UMLAUT, 'p', KEY_INSERT, 'j', 0, KEY_RIGHT_CTRL, 0, 'q', 0, '3', 't', 'u', '1', 0 },
/*1*/ { 0, KEY_DELETE, KEY_DOWN_ARROW, KEY_LEFT_ARROW, 0, KEY_A_ACUTE, KEY_RIGHT_GUI, ' ', 0, 0, 0, 'y', KEY_RIGHT_ALT, 0, 'v', 'b', 0, 0 },
/*2*/ { KEY_PAGE_DOWN, 0, KEY_E_ACUTE, KEY_HOME, 0x2D, '.', 0, ',', 0, 0, 0, 'x', KEY_LEFT_ALT, 'c', 'f', 'n', 'a', 0 },
/*3*/ { 0, KEY_PAGE_UP, KEY_U_DOUBLE_ACUTE, KEY_UP_ARROW, 0, 0, 0, 'm', KEY_RIGHT_SHIFT, 0, 0, 's', 0, 'd', 'g', 'h', KEY_CAPS_LOCK, 0 },
/*4*/ { 0, 0, KEY_RETURN, KEY_I_ACUTE, KEY_O_DOUBLE_ACUTE, 'l', 0, 'k', 0, 0, KEY_LEFT_GUI, 'w', 0, 'e', 'r', 'z', KEY_TAB, 0 },
/*5*/ { 0, 0, KEY_BACKSPACE, KEY_O_ACUTE, KEY_O_UMLAUT, 'o', 0, 'i', 0, KEY_LEFT_CTRL, 0, '2', 0, '4', '5', '7', '0', 0 },
/*6*/ { 0, 0, /*KEY_PAUSE*/ 0, KEY_PRTSC, KEY_F12, '9', 0, KEY_F8, KEY_LEFT_SHIFT, 0, 0, KEY_F2, 0, KEY_F4, '6', '8', KEY_F1, 0 }, //17
/*7*/ { 0, 0, KEY_END, 0, KEY_F11, KEY_F10, 0, KEY_F9, 0, 0, 0, KEY_F3, 0, KEY_F5, KEY_F6, KEY_F7, KEY_ESC, 0 }//17
};


#ifdef OTA
  #if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <AsyncTCP.h>
#endif

#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

const char* ssid = OTA_SSID;
const char* password = OTA_PASSWORD;

AsyncWebServer server(80);

unsigned long ota_progress_millis = 0;


#endif

// #ifdef BLE
  BleKeyboard bleKeyboard;
// #endif

//====================================================================================================

void readCols(uint8_t channelRow) {
  int keyButton = 0;
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

    

    if(keyStatusMatrix[channelRow][channelCol] != pinStat) {//state changed
      keyButton = keyMatrix[channelRow][channelCol];
      if (pinStat) {  //if this button is pressed down, so was not before
      Serial.println("Pressing");
      #ifdef BLE
        bleKeyboard.press(keyButton);
      #endif
      } else {  //if this button is NOT pressed down, and was before
      Serial.println("Releaseing");
      #ifdef BLE
        bleKeyboard.release(keyButton);
      #endif
      }

      keyStatusMatrix[channelRow][channelCol] = pinStat;
      
      Serial.print(channelRow);
      Serial.print(" : ");
      Serial.print(channelCol);
      Serial.print(" = ");
      Serial.println(keyButton);

    }


  }

}

//====================================================================================================

void writeRows() {
  uint8_t channelRow;
  for (int i = 0; i < PINS_ROW; i++) {
    channelRow = pinRows[i] - 128;
    muxRow.channel(channelRow);
    delayMicroseconds(delayMicrosInWrite);
    readCols(channelRow);
  }
}

//====================================================================================================

void pinSetup() {
  pinMode(mux_pin_row, OUTPUT);          //seting row as output
  pinMode(mux_pin_col, INPUT_PULLDOWN);  //setting col as input

  pinMode(18, INPUT_PULLDOWN);  //one more col pin
  pinMode(19, INPUT_PULLDOWN);  //one more col pin

  digitalWrite(mux_pin_row, HIGH);
  
  #ifdef OTA
  pinMode(OTA_PIN, INPUT_PULLUP);  //ota pin
  #endif
}

//====================================================================================================
void onOTAStart() {
  #ifdef OTA
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
  #endif
}

void onOTAProgress(size_t current, size_t final) {
  #ifdef OTA
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
  #endif
}

void onOTAEnd(bool success) {
  #ifdef OTA
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
  #endif
}


void wifiConnect() {
  #ifdef OTA
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! This is ElegantOTA AsyncDemo.");
  });

  ElegantOTA.begin(&server);    // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
  Serial.println("HTTP server started");
  #endif
}

//====================================================================================================

void setup() {
  delay(100);
  Serial.begin(115200);

  delay(10);
  Serial.println("Starting!");
  delay(100);
  Serial.println("Setting pins!");
  pinSetup();
  Serial.println("Pins set!");
  delay(100);
  #ifdef OTA
  if(!digitalRead(OTA_PIN)) {
    Serial.println("OTA IS Starting!");

    wifiConnect();
    return;
  }
  #endif

  // #ifdef BLE
    Serial.println("Starting BLE work!");
  bleKeyboard.begin();
  // #endif

  Serial.println("Setpu done!");
}

void loop() {
  // Serial.println("Loop!");
  #ifdef OTA
  if(!digitalRead(OTA_PIN)) {
    ElegantOTA.loop();
    return;
  }
  #endif
  
  #ifdef BLE
  if (bleKeyboard.isConnected()) {
  #endif
    // Serial.println("meh1!");
    writeRows();
    delay(10);
    // Serial.println("meh2!");
  #ifdef BLE
  } else {
    Serial.println("Waiting 5 seconds for BT connection...");
    delay(1000);
  }
  #endif
   
}
