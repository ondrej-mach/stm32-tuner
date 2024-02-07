/*
 * Code taken from https://github.com/1Bitsy/1bitsy-examples
 */

#ifndef _I2S_H
#define _I2S_H

#include <stdint.h>

#define AUDIO_FS 48000
#define FRAME_SAMPLES 64

typedef void AudioProcessor(const int32_t *restrict in, int32_t *restrict out);


extern void init_codec(void);
void set_processing_function(AudioProcessor *fn);

#endif // _I2S_H
