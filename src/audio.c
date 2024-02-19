#include "audio.h"
#include "i2s.h"
#include "log.h"
#include "yin.h"
#include "note.h"

#include <libopencm3/stm32/gpio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <printf.h>


static int16_t tunerBuffer[2][YIN_BUFFER_SIZE];
static volatile bool frontBufferIndex;

static SemaphoreHandle_t tunerListenSem, tunerCalcSem;
static volatile uint16_t tbIndex = 0;


static void passthrough(const int32_t *restrict in, int32_t *restrict out) {
    for (int i=0; i<FRAME_SAMPLES; i++) {
        out[i] = (i>>5) << 26;// in[i];
    }

    BaseType_t hptw = pdFALSE; // Higher Priority Task Woken
    if (xSemaphoreTakeFromISR(tunerListenSem, &hptw) == pdTRUE) {
        for (int i=0; i<FRAME_SAMPLES/2; i++) {
            tunerBuffer[!frontBufferIndex][tbIndex+i] = in[i*2] >> 16; // only left channel
        }
        tbIndex += FRAME_SAMPLES/2;

        if (tbIndex < YIN_BUFFER_SIZE) {
            xSemaphoreGiveFromISR(tunerListenSem, &hptw);
        } else {
            xSemaphoreGiveFromISR(tunerCalcSem, &hptw);
            tbIndex = 0;
        }
    }
    portYIELD_FROM_ISR(hptw);
}


static QueueHandle_t tunerQueue;

void audio_set_tuner_queue(QueueHandle_t queue) {
    tunerQueue = queue;
}

static void tuner_task(void *args __attribute((unused))) {
    static Yin yin;
    static Note note;
    Yin_init(&yin, YIN_BUFFER_SIZE, 0.1);

    while (1) {
        while (xSemaphoreTake(tunerCalcSem, portMAX_DELAY) != pdTRUE);
        frontBufferIndex = !frontBufferIndex;
        xSemaphoreGive(tunerListenSem);

        float pitch = Yin_getPitch(&yin, tunerBuffer[frontBufferIndex]);
        freqToNote(pitch, &note);

        if (tunerQueue) {
            xQueueSendToBack(tunerQueue, &note, portMAX_DELAY);
        }
        gpio_toggle(GPIOC, GPIO13);
    }
}

int audio_init(void) {
    tunerListenSem = xSemaphoreCreateCounting(2, 1);
    tunerCalcSem = xSemaphoreCreateCounting(2, 0);

    init_codec();
    set_processing_function(passthrough);

    xTaskCreate(tuner_task, "TUNER", 256, NULL, configMAX_PRIORITIES-1, NULL);

    return 0;
}
