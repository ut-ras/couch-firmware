#include <RASLib/inc/common.h>
#include <RASLib/inc/motor.h>
#include <RASLib/inc/gpio.h>
#include <RASLib/inc/spi.h>
#include <RASLib/inc/time.h>
#include <RASLib/inc/uart.h>
#include <stdint.h>
int runningSumLeft = 0;
int runningSumRight = 0;
int ind = 0;
static tMotor *leftMotor;
static tMotor *rightMotor;
static int ledState = 0;
static tSPI * spi;

void ToggleLED (void) {
        SetPin(PIN_F1, ledState & 1);
        ++ledState;
}

uint8_t reverse_byte (uint8_t toRev) {
        uint32_t toRet;
        asm volatile ("rbit %1, %0;": "=r"(toRet): "r" (toRev));
        return (toRet >> 24) & 0xFF ;
}

void reverse_array (uint32_t * array, int size){
        for (int i = 0; i < size; ++i)
                array[i] = reverse_byte(array[i]);
}

#define mode_byte (1)

int send_packet_PS2(uint32_t * packet, int size, uint32_t * response) {
        int  buff_size = 3;
        SetPin(PIN_B2, 0);
        SPIRequestUS(spi, -1, &packet[0], 3 , &response[0], 3, 16);
        reverse_array (response, 3);
        buff_size += 2 * (response[mode_byte] & 0x0F);
        SPIRequestUS(spi, -1, &packet[3], size - 3 , &response[3], buff_size - 3, 32);
        reverse_array (&response[3], buff_size - 3);
        SetPin(PIN_B2, 1);
        return buff_size;
}

#define d_buf_size (37)
uint32_t poll[3] = {0x01, 0x42, 0x00};
uint32_t poll_and_escape[5] = {0x01, 0x43, 0x00, 0x01, 0x00};
uint32_t set_dualshock_mode[9] = { 0x01, 0x44, 0x00, 0x01, 0x03
                                   , 0x00, 0x00, 0x00, 0x00};
uint32_t set_analog_button_mode[9] = { 0x01, 0x4f, 0x00, 0xFF, 0xFF
                                       , 0x03, 0x00, 0x00, 0x00};
uint32_t leave_escape[5] = {0x01, 0x43, 0x00, 0x00, 0x00};
#define arraySizeof(arry) (sizeof(arry) / sizeof(arry[0]))

int main (void) {
        leftMotor = InitializeServoMotor(PIN_C5, true);
        rightMotor = InitializeServoMotor(PIN_C7, false);
        SetMotor(leftMotor, 0.0);
        SetMotor(rightMotor, 0.0);
        signed char last10left[10];
        signed char last10right[10];
        float left10[10];
        float right10[10];
        for (int p = 0; p < 10; p++)
        {
            last10left[p] = 0x00;
            last10right[p] = 0x00;
            left10[p] = 0.0;
            right10[p] = 0.0;
        }
        uint32_t data[d_buf_size] = {0x00,};
        spi = InitializeSPI(PIN_A2, PIN_A5, PIN_A4, 125000, 8, true, true);
        //int i = 0,j;
        reverse_array(&poll[0],
                      arraySizeof(poll));
        reverse_array(&poll_and_escape[0],
                      arraySizeof(poll_and_escape));
        reverse_array(&set_dualshock_mode[0],
                      arraySizeof(set_dualshock_mode));
        reverse_array(&set_analog_button_mode[0],
                      arraySizeof(set_analog_button_mode));
        reverse_array(&leave_escape[0],
                      arraySizeof(leave_escape));
        CallEvery(ToggleLED, 0, 0.25);
        do {
                Printf("Trying to turn on the Controller\n");
                send_packet_PS2(&poll_and_escape[0],
                                arraySizeof(poll_and_escape), &data[0]);
                Wait(0.02);
                send_packet_PS2(&set_dualshock_mode[0],
                                arraySizeof(set_dualshock_mode), &data[0]);
                Wait(0.05);
                send_packet_PS2(&set_analog_button_mode[0],
                                arraySizeof(set_analog_button_mode), &data[0]);
                Wait(0.02);
                send_packet_PS2(&leave_escape[0],
                                arraySizeof(leave_escape), &data[0]);
                Wait(0.02);
                send_packet_PS2(&poll[0],
                                arraySizeof(poll), &data[0]);
                Printf("Data [1] = %x\n", data[1]);
        } while ((data[1] & 0xF0) != 0x70);
        Wait(0.2);
        while(1) {
               // int buff_size = send_packet_PS2(&poll[0],
                 //                               arraySizeof(poll), &data[0]);
               // int i = 0;
               // for (i = 0; i < buff_size; ++i)
               // Printf(" %02x", data[i]);
               // Printf("\n");
                //at this point, set a motor based on controller input.
                //see jimmy's psx firmare demo for analog example.
                //left analog stick controls direction, x is forward, square is reverse, R2 is deadman switch. use running avergae so x->square doesn't launch you.
                //continue running average after R2 released so it doesn't brake. assume 0 inputs.
                //set emergency brake on O?
                //if value squared is under 400, ignore
                //
                //data packet mappings:
                //left analog dx: data[7], 0x00 = left, 0xff = right
                //left analog dy: data[8], 0x00 = up, 0xff = down
                //right analog dx: data[5], 0x00 = left, 0xff = right
                //right analog dy: data[6], 0x00 = up, oxff = down
                //x: data[4] & 0x10 = b
                //x pressure: data[15]
                //square: data[4] & 0x10 = 7
                //square pressure: data[16]
                //O: data[4] = data[4] & 0x10 = d
                //O pressure: data[14]
             //    if (data[20] > 0x00)
             //    {
                    if (ind < 9)
                    {
                        last10left[ind] = data[8] - 0x80;
                        last10right[ind] = data[6] - 0x80;
                        ind++;
                    }
                    else if (ind >= 9)
                    {
                        for (int n = 0; n < 9; n++)
                        {
                           last10left[n] = last10left[n+1];
                           last10right[n] = last10right[n+1];
                        }
                        last10left[9] = data[8] - 0x80;
                        last10right[9] = data[6] - 0x80;
//                        Printf("%.2f %.2f\n", last10left[ind%10], last10right[ind%10]);
                    }
                    for (int k = 0; k < 10; k++)
                    {
                        left10[k] = ((float)last10left[k]) / (float)0x80;
                        right10[k] =((float)last10right[k]) / (float)0x80;
                    }
             //    }
             //   else if (data[20] <= 0x00)
             //   {
                    if (ind < 9)
                    {
                        last10left[ind] = 0x00;
                        last10right[ind] = 0x00;
                        ind++;
                    }
                    else if (ind >= 9)
                    {
                        for (int n = 0; n < 9; n++)
                        {
                           last10left[n] = last10left[n+1];
                           last10right[n] = last10right[n+1];
                        }
                        last10left[9] = 0x00;
                        last10left[9] = 0x00;

                    }
                    for (int k = 0; k < 10; k++)
                    {
                        left10[k] = ((float)last10left[k]) / (float)0x80;
                        right10[k] =((float)last10right[k]) / (float)0x80;
                    }
             //   }
                float temp1 = 0;
                float temp2 = 0;
                for (int m = 0; m < 10; m++)
                {
                    temp1 += left10[m];
                    temp2 += right10[m];
                }
                runningSumLeft = temp1 / 10.0f;
                runningSumRight = temp2 / 10.0f;

                SetMotor(leftMotor, runningSumLeft);
                SetMotor(rightMotor, runningSumRight);
                Printf("%.2f %.2f\n", runningSumLeft, runningSumRight);
               
//something in my code is causing the controller to turn off.
        }
}
