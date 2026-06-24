#include <Arduino.h>
#include <Wire.h>
#include <bluefruit.h>
#include "IQS5xx.h"

#define SDA_PIN D6
#define SCL_PIN D7
#define RST_PIN D1
#define RDY_PIN D0
#define LEFT_BUTTON_PIN D2
#define RIGHT_BUTTON_PIN D3

IQS5xx iqs;

BLEDis bledis;
BLEHidAdafruit blehid;
BLEUart bleuart;

static bool bleConnected = false;
static bool active_hold = false;
static uint8_t gestureButtonMask = 0;
static uint8_t physicalButtonMask = 0;
static uint8_t mouseButtons = 0;
static const bool debugLogging = false;

// Tuning constants
static const int movementDiv = 2; // smaller -> faster pointer
static const int scrollDiv = 40;  // larger -> slower scroll
static const int maxScroll = 1;   // clamp scroll magnitude

// Sensitivity / scaling: map 16-bit device delta to int8_t HID deltas
int8_t scaleDelta(int16_t v) {
  int32_t scaled = v / movementDiv;
  if (scaled > 127) return 127;
  if (scaled < -128) return -128;
  if (scaled > 0) return (int8_t)max<int32_t>(scaled, 1);
  if (scaled < 0) return (int8_t)min<int32_t>(scaled, -1);
  return 0;
}

uint8_t readPhysicalButtons(void) {
  uint8_t mask = 0;
  if (digitalRead(LEFT_BUTTON_PIN) == LOW) {
    mask |= 1; 
  }
  if (digitalRead(RIGHT_BUTTON_PIN) == LOW) {
    mask |= 2; 
  }
  return mask;
}

void updateMouseButtonState(uint8_t newMask) {
  if (newMask == mouseButtons) {
    return;
  }

  mouseButtons = newMask;
  if (!bleConnected) {
    return;
  }

  if (newMask == 0) {
    blehid.mouseButtonRelease();
    if (debugLogging) {
      bleuart.println("BTN RELEASE");
    }
  } else {
    blehid.mouseButtonPress(newMask);
    if (debugLogging) {
      bleuart.printf("BTN PRESS 0x%02X\n", newMask);
    }
  }
}

void connect_callback(uint16_t conn_hdl) {
  (void) conn_hdl;
  bleConnected = true;
  digitalWrite(LED_BUILTIN, HIGH);
  updateMouseButtonState(gestureButtonMask | physicalButtonMask);
}

void disconnect_callback(uint16_t conn_hdl, uint8_t reason) {
  (void) conn_hdl;
  (void) reason;
  bleConnected = false;
  digitalWrite(LED_BUILTIN, LOW);
}

void startAdv(void) {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_MOUSE);
  Bluefruit.Advertising.addService(blehid);
  if (debugLogging) {
    Bluefruit.Advertising.addService(bleuart);
  }
  Bluefruit.Advertising.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("NiceNano Touchpad");
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  bledis.setManufacturer("nice!");
  bledis.setModel("NiceNano Touchpad");
  bledis.begin();

  blehid.begin();
  if (debugLogging) {
    bleuart.begin();
  }

  startAdv();

  if (debugLogging) {
    bleuart.println("BLE HID ready");
  }

  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);


  Bluefruit.Periph.setConnInterval(9, 16); // 11.25ms-20ms
  Wire.setPins(SDA_PIN, SCL_PIN);
  Wire.begin();
  Wire.setClock(400000);

  if (!iqs.begin(Wire, 0x74, RDY_PIN, RST_PIN, true)) {
    if (debugLogging) {
      bleuart.println("IQS5xx begin failed");
    }
  } else {
    if (debugLogging) {
      bleuart.println("IQS5xx initialized");
      Wire.beginTransmission(0x74);
      if (Wire.endTransmission() == 0) {
        bleuart.println("IQS5xx ACK");
      } else {
        bleuart.println("IQS5xx NAK");
      }
    }
  }
}

void loop() {
  static unsigned long lastHeartbeat = 0;
  static unsigned long lastPollNoEvent = 0;
  IQS5xxReport rep;
  bool gotReport = iqs.poll(rep);

  uint8_t newPhysicalMask = readPhysicalButtons();
  bool buttonStateChanged = false;
  if (newPhysicalMask != physicalButtonMask) {
    physicalButtonMask = newPhysicalMask;
    buttonStateChanged = true;
  }

  if (!gotReport && bleConnected && millis() - lastPollNoEvent >= 2000) {
    lastPollNoEvent = millis();
    if (debugLogging) {
      uint8_t sys1 = 0, ge0 = 0, ge1 = 0;
      bool regsOk = iqs.readSystemInfo(sys1) && iqs.readGestureEvents(ge0, ge1);
      int rdy = digitalRead(RDY_PIN);
      if (regsOk) {
        bleuart.printf("poll no event RDY=%d SYS1=0x%02X GE0=0x%02X GE1=0x%02X\n", rdy, sys1, ge0, ge1);
      } else {
        bleuart.printf("poll no event RDY=%d READ FAIL\n", rdy);
      }
    }
  }

  if (gotReport) {
    if ((rep.movement || rep.scroll) && bleConnected) {
      int8_t dx = scaleDelta(rep.rel_x);
      int8_t dy = scaleDelta(rep.rel_y);
      if (rep.scroll) {
        int scrollAmt = rep.rel_y / scrollDiv;
        if (scrollAmt == 0) {
          if (abs(rep.rel_y) >= (scrollDiv / 2)) scrollAmt = (rep.rel_y > 0) ? 1 : -1;
        }
        if (scrollAmt > maxScroll) scrollAmt = maxScroll;
        if (scrollAmt < -maxScroll) scrollAmt = -maxScroll;
        if (scrollAmt != 0) {
          blehid.mouseScroll((int8_t)scrollAmt);
          if (debugLogging) {
            bleuart.printf("SCROLL %d\n", scrollAmt);
          }
        }
      } else {
        if (dx != 0 || dy != 0) {
          blehid.mouseMove(dx, dy);
          if (debugLogging) {
            bleuart.printf("M %d %d\n", rep.rel_x, rep.rel_y);
          }
        }
      }
    }

    if (rep.single_tap && bleConnected) {
      if (debugLogging) {
        bleuart.println("TAP1");
      }
      uint8_t savedButtons = mouseButtons;
      blehid.mouseReport((uint8_t)(savedButtons | 1), 0, 0, 0, 0);
      delay(20);
      blehid.mouseReport(savedButtons, 0, 0, 0, 0);
    }

    if (rep.two_finger_tap && bleConnected) {
      if (debugLogging) {
        bleuart.println("TAP2");
      }
      uint8_t savedButtons = mouseButtons;
      blehid.mouseReport((uint8_t)(savedButtons | 2), 0, 0, 0, 0);
      delay(20);
      blehid.mouseReport(savedButtons, 0, 0, 0, 0);
    }

    if (rep.press_and_hold) {
      if (!active_hold) {
        if (debugLogging) {
          bleuart.println("HOLD START");
        }
        active_hold = true;
        gestureButtonMask = 1;
        buttonStateChanged = true;
      }
    } else {
      if (active_hold) {
        if (debugLogging) {
          bleuart.println("HOLD END");
        }
        active_hold = false;
        gestureButtonMask = 0;
        buttonStateChanged = true;
      }
    }
  }

  if (buttonStateChanged) {
    updateMouseButtonState(gestureButtonMask | physicalButtonMask);
  }

  if (debugLogging && bleConnected && millis() - lastHeartbeat >= 1000) {
    lastHeartbeat = millis();
    bleuart.println("HB");
  }

  delay(5);
}