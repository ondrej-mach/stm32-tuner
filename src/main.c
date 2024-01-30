#include "log.h"
#include "usb.h"
#include "audio.h"

#include <FreeRTOS.h>
#include <task.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include <stdint.h>
#include <string.h>
#include <printf.h>


static void task1(void *args __attribute((unused))) {
	while (true) {
		gpio_toggle(GPIOC, GPIO13);
		vTaskDelay(500);
	}
}

static void task2(void *args __attribute((unused))) {
	while (true) {
		vTaskDelay(pdMS_TO_TICKS(10000));
		printf("Hello There\n");
	}
}


int main(void) {
	rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
    rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOA);

    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13 | GPIO14);
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO0);

    xTaskCreate(task1, "LED1", 100, NULL, configMAX_PRIORITIES-1, NULL);

	usb_init();
	xTaskCreate(task2, "PRINTER", 100, NULL, configMAX_PRIORITIES-1, NULL);

	audio_init();

	vTaskStartScheduler();
	while (true) {}
}
