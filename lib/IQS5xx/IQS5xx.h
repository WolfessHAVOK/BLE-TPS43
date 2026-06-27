/*
 * Copyright (c) 2025 Mariano Uvalle
 * SPDX-License-Identifier: MIT
 */

//MIT License

//Copyright (c) 2025 Mariano Uvalle

//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:

//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.

//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#pragma once

#include <Arduino.h>
#include <Wire.h>

// Minimal subset of IQS5xx register definitions (from ZMK driver)
#define IQS5XX_NUM_FINGERS 0x0011
#define IQS5XX_REL_X 0x0012
#define IQS5XX_REL_Y 0x0014
#define IQS5XX_END_COMM_WINDOW 0xEEEE

#define IQS5XX_SYSTEM_INFO_0 0x000F
#define IQS5XX_SYSTEM_INFO_1 0x0010
#define IQS5XX_GESTURE_EVENTS_0 0x000D
#define IQS5XX_GESTURE_EVENTS_1 0x000E

#define IQS5XX_TP_MOVEMENT (1 << 0)
#define IQS5XX_SINGLE_TAP (1 << 0)
#define IQS5XX_PRESS_AND_HOLD (1 << 1)
#define IQS5XX_TWO_FINGER_TAP (1 << 0)
#define IQS5XX_SCROLL (1 << 1)

struct IQS5xxReport {
    int16_t rel_x;
    int16_t rel_y;
    bool movement;
    bool single_tap;
    bool two_finger_tap;
    bool press_and_hold;
    bool scroll;
};

class IQS5xx {
public:
    IQS5xx();
    bool begin(TwoWire &wire = Wire, uint8_t i2c_addr = 0x74, int rdy_pin = -1, int rst_pin = -1, bool rdy_active_high = false);
    bool poll(IQS5xxReport &out);
    bool readSystemInfo(uint8_t &sys1);
    bool readGestureEvents(uint8_t &ge0, uint8_t &ge1);

private:
    TwoWire *_wire;
    uint8_t _addr;
    int _rdy_pin;
    int _rst_pin;
    bool _rdy_active_high;

    bool readReg8(uint16_t reg, uint8_t &val);
    bool readReg16(uint16_t reg, int16_t &val);
    bool writeEndComm();
};
