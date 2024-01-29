# STM32F4 Discovery FreeRTOS Makfile

This is a simple template project to build a firmware for the ---STM32F4 Discovery--- WeAct Black Pill 3.0 board with [FreeRTOS](https://www.freertos.org/) and [libopencm3](https://libopencm3.org/).


The main file is provided as an example that implements a simple blink program.

All the dependencies (FreeRTOS and libopencm3) are provided as submodules.

## Requirements

GNU ARM GCC (https://launchpad.net/gcc-arm-embedded)

ST-Link open source utility (https://github.com/stlink-org/stlink)

## Usage

### build project

```
$ make
```

### clean project

```
$ make clean
```

### download to mcu by stlink
```
$ make flash
```
