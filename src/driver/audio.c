#include "st7565.h"
#include <stdbool.h>
#ifdef ENABLE_FMRADIO
#include "app/fm.h"
#endif
#include "audio.h"
#ifdef ENABLE_FMRADIO
#include "bk1080.h"
#endif
#include "bk4829.h"
#include "gpio.h"
#include "py25q16.h"
#include "systick.h"

static bool lastState;

void AUDIO_ToggleSpeaker(bool on) {
  if (on == lastState) {
    return;
  }

  if (on) {
    AUDIO_AudioPathOn();
  } else {
    AUDIO_AudioPathOff();
  }
  lastState = on;
  gRedrawScreen = true;
}
