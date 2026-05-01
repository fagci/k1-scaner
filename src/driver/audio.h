#ifndef AUDIO_H
#define AUDIO_H

#include "gpio.h"
#include <stdbool.h>
#include <stdint.h>

#define AUDIO_AudioPathOn() GPIO_EnableAudioPath()
#define AUDIO_AudioPathOff() GPIO_DisableAudioPath()
void AUDIO_ToggleSpeaker(bool on);

#endif
