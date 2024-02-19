# STM32 tuner

The aim of this project  was to create a tuner for musical instruments.
The project is built around [WeAct Studio Black Pill V3.0](https://docs.zephyrproject.org/latest/boards/arm/blackpill_f401ce/doc/index.html) development board, which is based on STM32F401CC microcontroller.
To capture the sound, INMP441 microphone is used.
It communicates with the microcontroller via I<sup>2</sup>S interface.
The target note is shown on SSD1306 OLED display, communicating over I<sup>2</sup>C bus.
The device also has three LEDs to guide the user to the target note.
The algorithms used allow the device to detect any frequency, meaning it can be used to tune any musical instrument.

## Firmware



## Connections

The microphone communicates over I2S interface.

| SPH0645 | STM32F401 |
|---------|-----------|
| SCK     | PB10      |
| SD      | PB14      |
| WS      | PB9       |
| L/R     | GND       |

The OLED display communicates over I2S interface and is connected to pins PB6 (SCL) and PB7 (SDA).
The indicator LEDs are connected to pins PB0, PB1, PB2.


## Libraries and projects used

[libopencm3](http://libopencm3.org/)
[FreeRTOS](https://www.freertos.org/index.html)
[A printf/sprintf Implementation for Embedded Systems](https://github.com/mpaland/printf)
[stm32-ssd1306](https://github.com/afiskon/stm32-ssd1306)

[Yin Pitch Tracking](https://github.com/ashokfernandez/Yin-Pitch-Tracking)
[STM32F4 Discovery FreeRTOS Makefile](https://github.com/AndColla/stm32f4discovery-freertos-makefile)
[owly](https://github.com/ildus/owly)
[libopencm3 examples](https://github.com/libopencm3/libopencm3-examples)

