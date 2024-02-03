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


static void passthrough(const int32_t *restrict in, int32_t *restrict out) {
    for (int i=0; i<FRAME_SAMPLES; i++) {
        out[i] = in[i];
    }

    if (xSemaphoreTakeFromISR(tunerListenSem, NULL) == pdTRUE) {
        static uint16_t tbIndex = 0;
        for (int i=0; i<FRAME_SAMPLES/2; i++) {
            tunerBuffer[tbIndex+i] = in[i*2] >> 8; // only left channel
        }
        tbIndex += FRAME_SAMPLES/2;

        if (tbIndex < FRAME_SAMPLES/2) {
            xSemaphoreGiveFromISR(tunerListenSem, NULL);
        } else {
            xSemaphoreGiveFromISR(tunerCalcSem, NULL);
            tbIndex = 0;
        }
    }
}

static void tuner_task(void *args __attribute((unused))) {
    static Yin yin;
    Yin_init(&yin, YIN_BUFFER_SIZE, 0.05);

    while (1) {
        xSemaphoreGive(tunerListenSem);
        xSemaphoreTake(tunerCalcSem, portMAX_DELAY);
        float pitch = Yin_getPitch(&yin, tunerBuffer);
        printf("Pitch is found to be %f with probabiity %f\n", pitch, Yin_getProbability(&yin));
    }
}

int audio_init(void) {
    tunerListenSem = xSemaphoreCreateBinary();
    tunerCalcSem = xSemaphoreCreateBinary();

    init_codec();
    set_processing_function(passthrough);

    //xTaskCreate(audio_task, "AUDIO", 100, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(tuner_task, "TUNER", 100, NULL, configMAX_PRIORITIES-1, NULL);

    return 0;
}
