# BLE-TPS43

Driver taken and adapted from [AYM1607's ZMK Driver Code](https://github.com/AYM1607/zmk-driver-azoteq-iqs5xx)

```
pio run -e adafruit_feather_nrf52840
```
Build a UF2
```
HEX=$(find .pio/build -name '*.hex' | head -n1)
python ~/.platformio/packages/framework-arduinoadafruitnrf52/tools/uf2conv/uf2conv.py \
  -f 0xADA52840 -c -o .pio/build/adafruit_feather_nrf52840/firmware.uf2 "$HEX"
```

## Pictures provide something to look at within such an order of context 
<img src="https://github.com/wolfessnaomitest/BLE-TPS43/blob/main/IMG_20260622_154443730_1.jpg">
