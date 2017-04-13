# LPC824 NeoPixel IR Distance Sensor

Infrared distance sensor using HW controlled NeoPixels and the LPC824 MCU. The SCT (State Configurable Timer) is used on the LPC824 to drive the NeoPixels, and the LED ring will adjust based on the distance detected from the Sharp IR distance sensor.

## Parts Used

This codebase makes use of the following parts:

- [LPCXpresso824-MAX Development Board](http://www.nxp.com/products/microcontrollers-and-processors/arm-processors/lpc-cortex-m-mcus/software-tools/lpcxpresso-boards/lpcxpresso824-max-board-for-lpc82x-family-mcus:OM13071) (OM13071)
- [NeoPixel Ring - 16 x 5050 LEDs](https://www.adafruit.com/product/1463)
- [Sharp IR Distance Sensor - GP2Y0A02YK](https://www.adafruit.com/product/1031)
