#include <RASLib/inc/common.h>
#include <RASLib/inc/motor.h>
#include <RASLib/inc/gpio.h>
#include <RASLib/inc/spi.h>
#include <RASLib/inc/time.h>
#include <RASLib/inc/uart.h>
#include <stdint.h>

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
                int buff_size = send_packet_PS2(&poll[0],
                                                arraySizeof(poll), &data[0]);
                int i = 0;
                for (i = 0; i < buff_size; ++i)
                Printf(" %02x", data[i]);
                Printf("\n");
        }
}
