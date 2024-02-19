#include "display.h"
#include "audio.h"
#include "note.h"

#include <ssd1306.h>

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
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO6 | GPIO7);
    gpio_set_output_options(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO6 | GPIO7);
	gpio_set_af(GPIOB, GPIO_AF4, GPIO6 | GPIO7);

    i2c_peripheral_disable(I2C1); /* disable i2c during setup */

	i2c_set_speed(I2C1, i2c_speed_fm_400k, rcc_apb1_frequency/1000000);

	i2c_peripheral_enable(I2C1); /* finally enable i2c */
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

#define ABS(x)  ((x<0)?-x:x)
static void ledShowNote(Note *n) {
    if (n->valid) {
        if (ABS(n->cents) < 15) {
            led_write(0, 1, 0);
        } else {
            if (n->cents < 0) {
                led_write(1, 0, 0);
            } else {
                led_write(0, 0, 1);
            }
        }
    } else {
        led_write(0, 0, 0);
    }
}


static void printNote(Note *n) {
    static bool first = 1;

    const int NOTE_NAME_OFFSET_X = 52;
    const int NOTE_NAME_OFFSET_Y = 8;

    if (n->valid) {
        #if 1
        char *name = noteNameStrings[n->name];
        char noteLetter = name[0];
        char noteModifier = name[1] ? name[1] : ' ';
        char octStr[2];
        snprintf(octStr, 2, "%d", n->octave);

        ssd1306_SetCursor(NOTE_NAME_OFFSET_X, NOTE_NAME_OFFSET_Y);
        ssd1306_WriteChar(noteLetter, Font_16x26, White);
        ssd1306_SetCursor(NOTE_NAME_OFFSET_X+16, NOTE_NAME_OFFSET_Y);
        ssd1306_WriteChar(noteModifier, Font_7x10, White);
        ssd1306_SetCursor(NOTE_NAME_OFFSET_X+16, NOTE_NAME_OFFSET_Y+26-10);
        ssd1306_WriteChar(octStr[0], Font_7x10, White);
        #endif

        #if 0
        const uint8_t F_OFFSET_X = 0; //NOTE_NAME_OFFSET_X+16+7;
        const uint8_t F_OFFSET_Y = 24;
        static char freqStr[18];
        snprintf(freqStr, 18, "%f Hz", n->freq);
        ssd1306_FillRectangle(F_OFFSET_X, F_OFFSET_Y, F_OFFSET_X + 7*18, F_OFFSET_Y + 10, Black);
        ssd1306_SetCursor(F_OFFSET_X, F_OFFSET_Y);
        ssd1306_WriteString(freqStr, Font_7x10, White);
        #endif
    }

    if (n->valid || first) {
        first = 0;

        ssd1306_FillRectangle(0, 32, 127, 63, Black);
        for (uint8_t i=0; i<=10; i++) {
            // big lines for end and center
            uint8_t x = i*10+28/2;
            if (i == 5) {
                ssd1306_FillRectangle(x, 40, x, 56, White);
            } else {
                ssd1306_DrawPixel(x, 48, White);
            }
        }
        uint8_t x = n->cents+50+28/2;
        ssd1306_FillRectangle(x-1, 44, x+1, 52, White);

        ssd1306_UpdateScreen();
    }
}


static void ui_task(void *args __attribute((unused))) {
    ssd1306_Init();

    while (true) {
        Note note;
		xQueueReceive(tunerQueue, &note, portMAX_DELAY);
        ledShowNote(&note);
        printNote(&note);

	}
}


void ui_init() {
    i2c_setup();
    led_setup();
    tunerQueue = xQueueCreate(1, sizeof(Note));
    audio_set_tuner_queue(tunerQueue);
	xTaskCreate(ui_task, "UI", 256, NULL, configMAX_PRIORITIES-1, NULL);
}
