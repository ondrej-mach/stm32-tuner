#include "display.h"
#include "audio.h"
#include "note.h"

#include <ssd1306.h>
#include <ssd1306_tests.h>

#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <printf.h>

static QueueHandle_t tunerQueue;


static void i2c_setup(void) {
	rcc_periph_clock_enable(RCC_I2C1);
	rcc_periph_clock_enable(RCC_GPIOB);

	rcc_periph_reset_pulse(RST_I2C1);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6 | GPIO7);
	gpio_set_af(GPIOB, GPIO_AF4, GPIO6 | GPIO7);
	i2c_peripheral_disable(I2C1);

	i2c_set_speed(I2C1, i2c_speed_sm_100k, 8);

	i2c_peripheral_enable(I2C1);
}

static void led_setup(void) {
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0 | GPIO1 | GPIO2);
    gpio_clear(GPIOB, GPIO0 | GPIO1 | GPIO2);
}

static void led_write(bool l1, bool l2, bool l3) {
    gpio_clear(GPIOB, GPIO0 | GPIO1 | GPIO2);
    uint16_t on = GPIO0 * l1
        | GPIO1 * l2
        | GPIO2 * l3;
    gpio_set(GPIOB, on);
}


void testFonts() {
    uint8_t y = 0;
    ssd1306_Fill(Black);

    #ifdef SSD1306_INCLUDE_FONT_16x26
    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("Font 16x26", Font_16x26, White);
    y += 26;
    #endif

    #ifdef SSD1306_INCLUDE_FONT_11x18
    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("Font 11x18", Font_11x18, White);
    y += 18;
    #endif

    #ifdef SSD1306_INCLUDE_FONT_7x10
    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("Font 7x10", Font_7x10, White);
    y += 10;
    #endif

    #ifdef SSD1306_INCLUDE_FONT_6x8
    ssd1306_SetCursor(2, y);
    ssd1306_WriteString("Font 6x8", Font_6x8, White);
    #endif

    ssd1306_UpdateScreen();
}


#define ABS(x)  ((x<0)?-x:x)

static void ui_task(void *args __attribute((unused))) {
    //ssd1306_Init();

#if 0

	while (true) {
		ssd1306_TestFonts1();
        vTaskDelay(pdMS_TO_TICKS(5000));
		ssd1306_Fill(White);
        vTaskDelay(pdMS_TO_TICKS(1000));
	}

#else

    while (true) {
        Note note;
		xQueueReceive(tunerQueue, &note, portMAX_DELAY);
        if (note.valid) {
            if (ABS(note.cents) < 15) {
                led_write(0, 1, 0);
            } else {
                if (note.cents < 0) {
                    led_write(1, 0, 0);
                } else {
                    led_write(0, 0, 1);
                }
            }
        } else {
            led_write(0, 0, 0);
        }
	}

#endif


}


void ui_init() {
    i2c_setup();
    led_setup();
    tunerQueue = xQueueCreate(1, sizeof(Note));
    audio_set_tuner_queue(tunerQueue);
	xTaskCreate(ui_task, "UI", 256, NULL, configMAX_PRIORITIES-1, NULL);
}
