#include "IQS5xx.h"

IQS5xx::IQS5xx() : _wire(nullptr), _addr(0x74), _rdy_pin(-1), _rst_pin(-1), _rdy_active_high(false) {}

bool IQS5xx::begin(TwoWire &wire, uint8_t i2c_addr, int rdy_pin, int rst_pin, bool rdy_active_high) {
    _wire = &wire;
    _addr = i2c_addr;
    _rdy_pin = rdy_pin;
    _rst_pin = rst_pin;
    _rdy_active_high = rdy_active_high;

    if (_rst_pin >= 0) {
        pinMode(_rst_pin, OUTPUT);
        digitalWrite(_rst_pin, HIGH);
        delay(1);
        digitalWrite(_rst_pin, LOW);
        delay(10);
        digitalWrite(_rst_pin, HIGH);
        delay(1);
    }

    if (_rdy_pin >= 0) {
        pinMode(_rdy_pin, INPUT_PULLUP);
    }

    return true;
}

bool IQS5xx::readReg8(uint16_t reg, uint8_t &val) {
    if (!_wire) return false;
    _wire->beginTransmission(_addr);
    _wire->write((reg >> 8) & 0xFF);
    _wire->write(reg & 0xFF);
    if (_wire->endTransmission(false) != 0) return false;
    if (_wire->requestFrom((int)_addr, 1) != 1) return false;
    val = _wire->read();
    return true;
}

bool IQS5xx::readReg16(uint16_t reg, int16_t &val) {
    if (!_wire) return false;
    _wire->beginTransmission(_addr);
    _wire->write((reg >> 8) & 0xFF);
    _wire->write(reg & 0xFF);
    if (_wire->endTransmission(false) != 0) return false;
    if (_wire->requestFrom((int)_addr, 2) != 2) return false;
    uint8_t hi = _wire->read();
    uint8_t lo = _wire->read();
    val = (int16_t)((hi << 8) | lo);
    return true;
}

bool IQS5xx::readSystemInfo(uint8_t &sys1) {
    return readReg8(IQS5XX_SYSTEM_INFO_1, sys1);
}

bool IQS5xx::readGestureEvents(uint8_t &ge0, uint8_t &ge1) {
    return readReg8(IQS5XX_GESTURE_EVENTS_0, ge0) && readReg8(IQS5XX_GESTURE_EVENTS_1, ge1);
}

bool IQS5xx::writeEndComm() {
    if (!_wire) return false;
    _wire->beginTransmission(_addr);
    _wire->write((IQS5XX_END_COMM_WINDOW >> 8) & 0xFF);
    _wire->write(IQS5XX_END_COMM_WINDOW & 0xFF);
    _wire->write(0x00);
    return (_wire->endTransmission() == 0);
}

bool IQS5xx::poll(IQS5xxReport &out) {
    out = {0, 0, false, false, false, false, false};

    if (_rdy_pin >= 0) {
        bool ready = digitalRead(_rdy_pin) == (_rdy_active_high ? HIGH : LOW);
        if (!ready) return false;
    }

    uint8_t sys1 = 0;
    if (!readReg8(IQS5XX_SYSTEM_INFO_1, sys1)) return false;

    bool tp_movement = (sys1 & IQS5XX_TP_MOVEMENT) != 0;

    uint8_t ge0 = 0, ge1 = 0;
    readReg8(IQS5XX_GESTURE_EVENTS_0, ge0);
    readReg8(IQS5XX_GESTURE_EVENTS_1, ge1);

    out.movement = tp_movement;
    out.single_tap = (ge0 & IQS5XX_SINGLE_TAP) != 0;
    out.press_and_hold = (ge0 & IQS5XX_PRESS_AND_HOLD) != 0;
    out.two_finger_tap = (ge1 & IQS5XX_TWO_FINGER_TAP) != 0;
    out.scroll = (ge1 & IQS5XX_SCROLL) != 0;

    if (tp_movement || out.scroll) {
        int16_t rx = 0, ry = 0;
        if (!readReg16(IQS5XX_REL_X, rx)) {
            writeEndComm();
            return false;
        }
        if (!readReg16(IQS5XX_REL_Y, ry)) {
            writeEndComm();
            return false;
        }
        out.rel_x = rx;
        out.rel_y = ry;
    }

    writeEndComm();

    return (out.movement || out.single_tap || out.two_finger_tap || out.press_and_hold || out.scroll);
}
