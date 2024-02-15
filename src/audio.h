#ifndef _AUDIO_H
#define _AUDIO_H

#include <FreeRTOS.h>
#include <queue.h>


int audio_init(void);
void audio_set_tuner_queue(QueueHandle_t queue);


#endif //_AUDIO_H
