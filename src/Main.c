// note on use of serial port:
// to quit screen, send CTRL+a followed by \

#include <raslib/inc/common.h>
#include <raslib/inc/gpio.h>
#include <raslib/inc/time.h>
#include <raslib/inc/pwm.h>
#include <raslib/inc/uart.h>
#include <raslib/inc/servo.h>

// declare components
tPWM * right_motor;
tPWM * left_motor;

// define pins
#define right_motor_pin PIN_E1
#define left_motor_pin PIN_E2
//#define red_led_pin PIN_B5
//#define green PIN_F3
//#define blue_led_pin PIN_F2

// magic numbers
#define motor_pwm_freq 50.0f //50000.0f // 20 us signal period
#define pwm_min .0335f // percentage of 20 us signal for minimum pulse
#define pwm_max .1165f // percentage of 20 us signal for maximum pulse
float pwm_span = 0;
float pwm_half_span = 0;
float pwm_neutral = 0;

// serial control
char left_up = 'f';
char left_down = 'r';
char right_up = 'j';
char right_down = 'u';
char kill = 'k';
float speed_increment = .5;

// variables
float right_speed = 0; // power from -1 to 1
float left_speed = 0; // power from -1 to 1
char ch = 0; // input from serial port

// forward declarations
void initialize();
void set_motor_speeds();

// do stuff
int main(void) {


	initialize();

	//leftservo = InitializeServo(PIN_F1);
	//SetServo(leftservo, 0.5);

	while (1) {
	// print commands
	Printf("\nRASCOUCHATOR: ");

	// get character
	char ch = Getc();

	// move motors if need be
	if(ch == kill){
		right_speed = 0;
		left_speed = 0;
		Printf("To die, to sleep...");
	} if(ch == left_up){
		left_speed += speed_increment;
		Printf("Left is going faster.");
		if(left_speed > 1){
			left_speed = 1;
		}
	} else if(ch == left_down){
		left_speed -= speed_increment;
		Printf("Left is going slower.");
		if(left_speed < -1){
			left_speed = -1;
		}
	} else if(ch == right_up){
		right_speed += speed_increment;
		Printf("Right is going faster.");
		if(right_speed > 1){
			right_speed = 1;
		}
	} else if(ch == right_down){
		right_speed -= speed_increment;
		Printf("Right is going slower.");
		if(right_speed < -1){
			right_speed = -1;
		}
	}

	set_motor_speeds();
	}
}

// initializes pins and stuff
void initialize(){
	pwm_span = pwm_max - pwm_min;
	pwm_half_span = pwm_span / 2;
	pwm_neutral = pwm_min + pwm_half_span;

	right_motor = InitializePWM(right_motor_pin, motor_pwm_freq);
	left_motor = InitializePWM(left_motor_pin, motor_pwm_freq);
	set_motor_speeds();
}

// sets the motor speeds based on right/left speed variables
void set_motor_speeds(){
	float right = right_speed*pwm_half_span+pwm_half_span+pwm_min;
	float left = left_speed*pwm_half_span+pwm_half_span+pwm_min;

	//SetServo(leftservo, (left_speed + 1)*0.5);
	SetPWM(right_motor, right, 0);
	SetPWM(left_motor, left, 0);
	Printf(" (");
	Printf("%.2f", left_speed);
	Printf(" / ");
	Printf("%.2f", right_speed);
	Printf(") ");
}

/*
 * Jaguar PWM Specifications
 *
 * min pulse width : .67 ms
 * neutral width : 1.5 ms
 * max width : 2.33 ms
 * servo signal period: 5.0125 < x < 29.985 ms
 * valid pulse width range: 0.5 < x < 2.50625 ms
 * duty cycle range : 50%
 *
 * note: pulse widths follow linear distribution
 */

