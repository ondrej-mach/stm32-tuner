#include "audio.h"
#include "i2s.h"
#include "log.h"

#include <FreeRTOS.h>
#include <task.h>

static int16_t val=0;

static int16_t callback(i2s_channel channel) {
    if (channel == I2SC_LEFT) {
        val = (val+1) % 64;
    }
    return (val-32) * 1024;
}


int audio_init(void) {
    LOG("Initializing audio\n");

    i2s_config cfg = {
        .i2sc_sample_frequency = 44100,
        .i2sc_mode = I2SM_MASTER_TX,
        .i2sc_standard = I2SS_MSB,
        .i2sc_full_duplex = false,
    };

    i2s_instance inst = {
        .i2si_base_address = SPI2_BASE
    };

    init_i2s(&cfg, &inst, callback);

    //xTaskCreate(audio_task, "AUDIO", 100, NULL, configMAX_PRIORITIES-1, NULL);

    return 0;
}
