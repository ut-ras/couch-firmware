#include <RASLib/inc/common.h>
#include <RASLib/inc/motor.h>
#include <RASLib/inc/gpio.h>
#include <RASLib/inc/time.h>

static int ledState = 0;

void ToggleLED (void) {
        SetPin(PIN_F1, ledState & 1);
        SetPin(PIN_F2, ledState & 2);
        SetPin(PIN_F3, ledState & 4);
        ++ledState;
}

int main (void) {
        CallEvery(ToggleLED, 0, 0.25);
        while (1)
                asm volatile ("nop");
}
