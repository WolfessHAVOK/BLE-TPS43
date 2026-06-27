# BLE-TPS43

Driver taken and adapted from [AYM1607's ZMK Driver Code](https://github.com/AYM1607/zmk-driver-azoteq-iqs5xx)

The way i build for NiceNanoV2/nrf52ProMicro in platformIO because ive not found easy instruction online is.

Add nice_nano variant folder from [Arduino Core I Found](https://github.com/selimmeric/Adafruit_nRF52_Arduino_Nice-NanoV2) with its contents to /home/$user/.platformio/packages/framework-arduinoadafruitnrf52/variants/
Paying some attention to the platform.ini file may make this part make sense.

Then with the project setup in for 'adafruit feather nrf52840 express'
```
pio run -e adafruit_feather_nrf52840
```
And To Easily Build a UF2

```
HEX=$(find .pio/build -name '*.hex' | head -n1)
python ~/.platformio/packages/framework-arduinoadafruitnrf52/tools/uf2conv/uf2conv.py \
  -f 0xADA52840 -c -o .pio/build/adafruit_feather_nrf52840/firmware.uf2 "$HEX"
```

## Pictures provide something to look at within such an order of context 
<img src="https://github.com/wolfessnaomitest/BLE-TPS43/blob/main/IMG_20260622_154443730_1.jpg">
