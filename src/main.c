#include <RASLib/inc/common.h>
#include <RASLib/inc/motor.h>
#include <RASLib/inc/gpio.h>
#include <RASLib/inc/time.h>
#include <StellarisWare/driverlib/ssi.h>
#include <StellarisWare/driverlib/gpio.h>
#include <StellarisWare/driverlib/sysctl.h>
#include <StellarisWare/inc/hw_ssi.h>
#include <StellarisWare/inc/hw_types.h>
#include <StellarisWare/inc/hw_memmap.h>
#include <stdint.h>

static int ledState = 0;

void ToggleLED (void) {
        SetPin(PIN_F1, ledState & 1);
        SetPin(PIN_F2, ledState & 2);
        SetPin(PIN_F3, ledState & 4);
        ++ledState;
}

void initSPI(void) {
        uint32_t crap;
        SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
        /* in case this is not already done */
        SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
        /* set the pins for spi modes */
        GPIOPinConfigure(GPIO_PA2_SSI0CLK);
        GPIOPinConfigure(GPIO_PA3_SSI0FSS);
        GPIOPinConfigure(GPIO_PA4_SSI0RX);
        GPIOPinConfigure(GPIO_PA5_SSI0TX);
        GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 |
                       GPIO_PIN_2);
        SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), SSI_FRF_MOTO_MODE_3,
                           SSI_MODE_MASTER, 250000, 8);
        SSIEnable(SSI0_BASE);
        /* clean up the recieve fifo */
        while(SSIDataGetNonBlocking(SSI0_BASE, &crap));
}

uint8_t reverse_byte (uint8_t toRev) {
        uint32_t toRet;
        asm volatile ("rbit %1, %0;": "=r"(toRet): "r" (toRev));
        return (toRet >> 24) & 0xFF ;
}

#define mode_byte (1)
#define ack_byte (2)

int send_packet_PS2(uint32_t * packet, int size, uint32_t * response) {
        int i, buff_size = 3;
        SetPin(PIN_B2, 0);
        WaitUS(16);
        for (i = 0; i < size; ++i) {
                SSIDataPut(SSI0_BASE, reverse_byte(packet[i]));
                SSIDataGet(SSI0_BASE, &response[i]);
                response[i] = reverse_byte(response[i]);
                WaitUS(16);
        }
        if (response[ack_byte] == 0x5a)
                buff_size += 2 * (response[mode_byte] & 0x0F);
        for (; i < buff_size; ++i) {
                SSIDataPut(SSI0_BASE, 0);
                SSIDataGet(SSI0_BASE, &response[i]);
                response[i] = reverse_byte(response[i]);
                WaitUS(16);
        }
        while (SSIBusy(SSI0_BASE));
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
        initSPI();
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
        while (1) {
                int buff_size = send_packet_PS2(&poll[0], 3, &data[0]);
                for (i = 0; i < buff_size; ++i) {
                        Printf("0x%02x ", data[i]);
                }
                Printf("\r");
        }
}
