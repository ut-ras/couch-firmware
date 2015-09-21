#include <RASLib/inc/common.h>
#include <RASLib/inc/motor.h>
#include <RASLib/inc/gpio.h>
#include <RASLib/inc/spi.h>
#include <RASLib/inc/time.h>
#include <StellarisWare/driverlib/ssi.h>
#include <StellarisWare/driverlib/gpio.h>
#include <StellarisWare/driverlib/sysctl.h>
#include <StellarisWare/inc/hw_ssi.h>
#include <StellarisWare/inc/hw_types.h>
#include <StellarisWare/inc/hw_memmap.h>
#include <stdint.h>

static int ledState = 0;
static tSPI * spi;

void ToggleLED (void) {
        SetPin(PIN_F1, ledState & 1);
        SetPin(PIN_F2, ledState & 2);
        SetPin(PIN_F3, ledState & 4);
        ++ledState;
}

uint8_t reverse_byte (uint8_t toRev) {
        uint32_t toRet;
        asm volatile ("rbit %1, %0;": "=r"(toRet): "r" (toRev));
        return (toRet >> 24) & 0xFF ;
}

void reverse_array (uint32_t * array, int size){
        int i = 0;
        for (; i < size; ++i)
                array[i] = reverse_byte(array[i]);
}

#define mode_byte (1)

int send_packet_PS2(uint32_t * packet, int size, uint32_t * response) {
        int  buff_size = 3;
        SetPin(PIN_B2, 0);
        SPIRequest(spi, -1, &packet[0], 3 , &response[0], 3, 0.000016f);
        reverse_array (response, 3);
        buff_size += 2 * (response[mode_byte] & 0x0F);
        SPIRequest(spi, -1, &packet[3], size - 3 , &response[3], buff_size - 3, 0.000016f);
        reverse_array (&response[3], buff_size - 3);
        SetPin(PIN_B2, 1);
        return buff_size;
}

#define d_buf_size (100)
uint32_t poll[3] = {0x01, 0x42, 0x00};
uint32_t poll_and_escape[5] = {0x01, 0x43, 0x00, 0x01, 0x00};
uint32_t set_dualshock_mode[9] = { 0x01, 0x44, 0x00, 0x01, 0x03
                                   , 0x00, 0x00, 0x00, 0x00};
uint32_t set_analog_button_mode[9] = { 0x01, 0x4f, 0x00, 0xFF, 0xFF
                                       , 0x03, 0x00, 0x00, 0x00};
uint32_t leave_escape[5] = {0x01, 0x43, 0x00, 0x00, 0x00};

int main (void) {
        int i, buff_size;
        uint32_t data[d_buf_size] = {0x00,};
        reverse_array(poll,3);
        reverse_array(poll_and_escape,5);
        reverse_array(set_dualshock_mode,9);
        reverse_array(set_analog_button_mode,9);
        reverse_array(leave_escape,5);
        CallEvery(ToggleLED, 0, 0.25);
        spi = InitializeSPI(PIN_A2, PIN_A5, PIN_A4, 250000, 8, true, true);
        do {
                buff_size = send_packet_PS2(&poll_and_escape[0], 5, &data[0]);
                for (i = 0; i < buff_size; ++i) {
                        Printf("0x%02x ", data[i]);
                }
                Printf("\n");
                buff_size = send_packet_PS2(&set_dualshock_mode[0], 9, &data[0]);
                for (i = 0; i < buff_size; ++i) {
                        Printf("0x%02x ", data[i]);
                }
                Printf("\n");
                buff_size = send_packet_PS2(&set_analog_button_mode[0], 9, &data[0]);
                for (i = 0; i < buff_size; ++i) {
                        Printf("0x%02x ", data[i]);
                }
                Printf("\n");
                buff_size = send_packet_PS2(&leave_escape[0], 5, &data[0]);
                for (i = 0; i < buff_size; ++i) {
                        Printf("0x%02x ", data[i]);
                }
                Printf("\n");
                buff_size = send_packet_PS2(&poll[0], 3, &data[0]);
                for (i = 0; i < buff_size; ++i) {
                        Printf("0x%02x ", data[i]);
                }
                Printf("\n");
        } while (data[1] != 0x79);
        while (1) {
                buff_size = send_packet_PS2(&poll[0], 3, &data[0]);
                for (i = 0; i < buff_size; ++i) {
                        Printf("0x%02x ", data[i]);
                }
                Printf("\r");
        }
}
