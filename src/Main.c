#include <RASLib/inc/common.h>
#include <RASLib/inc/gpio.h>
#include <RASLib/inc/time.h>

// Blink the LED to show we're on
tBoolean blink_on = true;

void blink(void) {
    SetPin(PIN_F3, blink_on);
    blink_on = !blink_on;
}


// The 'main' function is the entry point of the program
int main(void) {
    // Initialization code can go here
    CallEvery(blink, 0, 0.5);
    
    while (1) {
        // Runtime code can go here
        Printf("Hello World!\n");
        
    }
}

/*
 * Jaguar PWM Specifications
 *
 * min pulse width : .67 us
 * neutral width : 1.5 us
 * max width : 2.33 us
 * servo signal period: 5.0125 < x < 29.985 us
 * valid pulse width range: 0.5 < x < 2.50625 us
 * duty cycle range : 50%
 */
