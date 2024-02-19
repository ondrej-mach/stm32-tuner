# STM32 tuner

The aim of this project  was to create a tuner for musical instruments.
The project is built around [WeAct Studio Black Pill V3.0](https://docs.zephyrproject.org/latest/boards/arm/blackpill_f401ce/doc/index.html) development board, which is based on STM32F401CC microcontroller.
To capture the sound, INMP441 microphone is used.
It communicates with the microcontroller via I<sup>2</sup>S interface.
The target note is shown on SSD1306 OLED display, communicating over I<sup>2</sup>C bus.
The device also has three LEDs to guide the user to the target note.
The algorithms used allow the device to detect any frequency, meaning it can be used to tune any musical instrument.

## Connections

The microphone communicates over I2S interface.

| SPH0645 | STM32F401 |
|---------|-----------|
| SCK     | PB10      |
| SD      | PB14      |
| WS      | PB9       |
| L/R     | GND       |

The OLED display communicates over I2C interface and is connected to pins PB6 (SCL) and PB7 (SDA).
The indicator LEDs are connected to pins PB0, PB1, PB2.

## Firmware

All the libraries and tools used in this project are free and open source.
Libopencm3 was chosen as the low level abstraction library for the microcontroler.
FreeRTOS was used to make the application developement easier.

### FreeRTOS tasks

The project uses tasks to separate the user interface from the audio backend.
The audio backend is responsible for receiving the audio data and running the Yin algorithm (file audio.c).
After the frequency is detected, it is converted to a musical note (structure Note) and it is passed to the UI task through FreeRTOS queue.

The UI task awaits the note from the audio backend.
After receiving the structure, it shows it on the OLED display and lights up corresponding LEDs. Then it goes back to waiting for the next note.

### I<sup>2</sup>S interface

The SPH0645 microphone communicates with the microcontroller via I<sup>2</sup>S interface.
The SPH0645 was chosen because it does not require I<sup>2</sup>S master clock, which is not supported by the microcontroller.
It can generate its own master clock using PLL from the supplied bus clock.

On the STM32 side, I<sup>2</sup>S uses the SPI peripheral.
The interface runs in standard Philips mode, stereo with 32-bit words.
To take the load off the processor, DMA is used to copy the data directly into a buffer.
Unfortunately, after the data are transferred to the memory, they are in a wrong format.
Because the SPI peripheral data register is only 16-bit wide, the peripheral cannot swap endianness of 32 bit words from microphone.
Hence, 16 bit halves of the 32 bit words have to be swapped with the main processor.
Nevertheless, the effect on application performance is minimal.

### Yin algorithm

The pitch detection algorithm is the most computationally difficult part of the project.
Since modern pitch detection algorithms tend to be quite complicated, open source implementation of [Yin Pitch Tracking](https://github.com/ashokfernandez/Yin-Pitch-Tracking) was used.
Yin algorithm has some variables that affect its accuracy and performance.
These variables are buffer size and sampling frequency.
Sampling frequency was kept at 48 kHz, since anything else would require changing I2S configuration or software undersampling.
The buffer size was incresed until the accuracy was satisfactory.
That ended up being at buffer size of 2048 elements
However, the speed was rather unimpressive with one computation taking about 280 ms.

To address this issue, the code of Yin algorithm were inspected to find the most comuputationally expensive function.
The function `Yin_difference` is a variation on autocorrelation function and takes most of the time to compute.
The long computation is explained by two nested for loops that both run for whole buffer size.
That means that the time complexity is quadratic respective to the buffer size.

First step of optimization was using the attribute to optimize for speed (-O3), since the rest of the code is optimized for size.
This made the computation time drop from 280 ms to about 170ms.
Since the difference is computed with only 16 bit integers, the Cortex M4 ALU is not fully utilized.
SIMD instructions were used to use the whole 32 bit width.
Since the compiler did not use these instructions automatically, they were put into the code as assembly.
This optimalization effectively cuts the time in half.
One iteration of the algorithm now takes less than 100 ms.

### SSD1306 OLED

To make the usage of the display easier [stm32-ssd1306](https://github.com/afiskon/stm32-ssd1306) library was used.
The library was designed to be used with STM HAL (Hardware Abstraction Library).
To make it work with libopencm3, functions `ssd1306_WriteCommand` and `ssd1306_WriteData` were re-implemented.


## Possible improvements

The most meaningful improvement for this project would be further optimization of the pitch detection algorithm.
As of now, the tuner is very much usable.
Yet, it lacks some accurary and speed compared to commercial counterparts.

The easiest improvement would be using a more modern and powerful microcontroller.
Currently used STM32F401 has Cortex M4 core running at 84MHz, making it the least powerful microcontroller from STM32F4 series.
Upgrading to STM32F415, which has the same core running at 168MHz, would double the speed.

On the software side, there are also some improvements to be made.
The Yin algorithm may not be the optimal algorithm for this usage.
It detects detect many frequencies that are out of range of any musical instrument.
Downsampling the input signal could lead to better accuracy at lower frequencies, which are more important in this use case.

It is also possible that using more advanced Yin implementation would increase the performance.
These implementations are using FFT to compute the Yin differece with better time complexity.
Since the microcontroller has a floating point unit, this approach could come out on top.


## Libraries and projects used

- [libopencm3](http://libopencm3.org/)
- [FreeRTOS](https://www.freertos.org/index.html)
- [A printf/sprintf Implementation for Embedded Systems](https://github.com/mpaland/printf)
- [stm32-ssd1306](https://github.com/afiskon/stm32-ssd1306)

- [Yin Pitch Tracking](https://github.com/ashokfernandez/Yin-Pitch-Tracking)
- [STM32F4 Discovery FreeRTOS Makefile](https://github.com/AndColla/stm32f4discovery-freertos-makefile)
- [owly](https://github.com/ildus/owly)
- [libopencm3 examples](https://github.com/libopencm3/libopencm3-examples)

