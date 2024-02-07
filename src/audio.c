#include "audio.h"
#include "i2s.h"
#include "log.h"
#include "yin.h"

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <printf.h>


static int16_t tunerBuffer[YIN_BUFFER_SIZE];

static SemaphoreHandle_t tunerListenSem, tunerCalcSem;
static volatile uint16_t tbIndex = 0;


static void passthrough(const int32_t *restrict in, int32_t *restrict out) {
    for (int i=0; i<FRAME_SAMPLES; i++) {
        out[i] = (i>>5) << 26;// in[i];
    }

    BaseType_t hptw = pdFALSE; // Higher Priority Task Woken
    if (xSemaphoreTakeFromISR(tunerListenSem, &hptw) == pdTRUE) {
        for (int i=0; i<FRAME_SAMPLES/2; i++) {
            tunerBuffer[tbIndex+i] = in[i*2] >> 16; // only left channel
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


static void tuner_task(void *args __attribute((unused))) {
    static Yin yin;
    Yin_init(&yin, YIN_BUFFER_SIZE, 0.5);

    while (1) {
        xSemaphoreGive(tunerListenSem);
        if (xSemaphoreTake(tunerCalcSem, portMAX_DELAY) == pdTRUE) {
            float pitch = Yin_getPitch(&yin, tunerBuffer);
            float prob = Yin_getProbability(&yin);
            printf("Pitch is found to be %f with probability %f\n", pitch, prob);
        }
    }
}

int audio_init(void) {
    tunerListenSem = xSemaphoreCreateBinary();
    tunerCalcSem = xSemaphoreCreateBinary();

    init_codec();
    set_processing_function(passthrough);

    //xTaskCreate(audio_task, "AUDIO", 100, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(tuner_task, "TUNER", 256, NULL, configMAX_PRIORITIES-1, NULL);

    return 0;
}
