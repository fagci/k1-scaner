#include "system.h"

void SYS_Main(void) {
    for (;;) { __WFI(); }
}

void SYS_MsgKey(KEY_Code_t key, Key_State_t state) { (void)key; (void)state; }
