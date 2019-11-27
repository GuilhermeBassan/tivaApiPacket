#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "inc/tm4c123gh6pm.h"

#include "driverlib/rom.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/timer.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/interrupt.h"

#define SW2         GPIO_PIN_0
#define RED_LED     GPIO_PIN_1
#define BLUE_LED    GPIO_PIN_2
#define GREEN_LED   GPIO_PIN_3
#define SW1         GPIO_PIN_4

#define RX          GPIO_PIN_0
#define TX          GPIO_PIN_1

#define     API_STR_DE  0x7E // API packet start delimiter

// API identifiers
#define     API_TRA_RE  0x10 // Transmit request
#define     API_TRA_ID  0x00 // Frame ID (0x00 - no response)
#define     API_COM_AT  0x10 // AT command
#define     API_COM_PV  0x09 // AT command parameter value
#define     API_COM_RE  0x88 // AT command response
#define     API_REM_CR  0x17 // Remote command request
#define     API_REM_RE  0x97 // Remote command response
#define     API_ZIG_TS  0x8B // Zigbee transmit status
#define     API_ZIG_RP  0x90 // Zigbee receive packet
#define     API_NOD_ID  0x95 // Node identification indicator
#define     API_OTA_ST  0xAD // Over the air update status
#define     API_MOD_ST  0x8A // Modem status

#define     API_BRO_RA  0x00 // API broadcast radius
#define     API_OPT_NO  0x00 // API options none
#define     API_OPT_DI  0x01 // API options disable retry
#define     API_OPT_EN  0x20 // API options enable encryption
#define     API_OPT_TI  0x40 // API options use extended timeout

#define     STR_MAX_SZ  1000 // Max string size

//void broadcastMessage(char message[STR_MAX_SZ]);
void broadcastMessage(uint32_t uartBase, char *message);

uint32_t counter0 = 0, counter1 = 0;
uint32_t ctrl0 = 0, ctrl1 = 0, ctrl2 = 0;

char time1[STR_MAX_SZ];
char send[STR_MAX_SZ];

unsigned char teste[] = { 0x7E, // Start delimiter
        0x00, 0x11, // Length MSB and LSB
        0x10, // Frame type
        0x01, // Frame ID
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 64 add
        0xFF, 0xFE, // 16 add
        0x00, // Broadcast radius
        0x00, // Options
        0x41, 0x42, 0x43, // Data (ABC)
        0x2B }; // Checksum

unsigned char apiPacket[STR_MAX_SZ];
unsigned char dataFrame[STR_MAX_SZ];
unsigned char coordBroadcast64[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00 };
unsigned char coordBroadcast16[] = { 0xFF, 0xFE };
char timeout[] = "Timeout!";

int main(void)
{
    int i = 0;
//    float time0;
    uint32_t period;
//    unsigned char buffer0;

    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL |
    SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // Set input and output
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED | BLUE_LED | GREEN_LED);
    HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTF_BASE + GPIO_O_CR) = 0x1;
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, SW1 | SW2);
    GPIOPadConfigSet(GPIO_PORTF_BASE, SW1 | SW2, GPIO_STRENGTH_2MA,
    GPIO_PIN_TYPE_STD_WPU);
    SysCtlDelay(SysCtlClockGet() / 10);

    // Set UART0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, RX | TX);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE));
    SysCtlDelay(SysCtlClockGet() / 10);

    // Set UART1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);
    GPIOPinTypeUART(GPIO_PORTB_BASE, RX | TX);
    UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 9600,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE));
    SysCtlDelay(SysCtlClockGet() / 10);

    // Set UART3
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    GPIOPinConfigure(GPIO_PC6_U3RX);
    GPIOPinConfigure(GPIO_PC7_U3TX);
    GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    UARTConfigSetExpClk(UART3_BASE, SysCtlClockGet(), 9600,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE));
    SysCtlDelay(SysCtlClockGet() / 10);

    // Set UART4
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART4);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    GPIOPinConfigure(GPIO_PC4_U4RX);
    GPIOPinConfigure(GPIO_PC5_U4TX);
    GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    UARTConfigSetExpClk(UART4_BASE, SysCtlClockGet(), 9600,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE));
    SysCtlDelay(SysCtlClockGet() / 10);

    // Set timer int
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    period = (SysCtlClockGet() / 1000);
    TimerLoadSet(TIMER0_BASE, TIMER_A, period - 1);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
    TimerEnable(TIMER0_BASE, TIMER_A);
    SysCtlDelay(SysCtlClockGet() / 10);

    while (1)
    {
//        if (!GPIOPinRead(GPIO_PORTF_BASE, SW1))
//        {
//
//            while (i <= 20)
//            {
//                buffer0 = teste[i];
//                UARTCharPut(UART0_BASE, buffer0);
//                i++;
//            }
//            i = 0;
//
//            while (i <= 20)
//            {
//                buffer0 = teste[i];
//                UARTCharPut(UART1_BASE, buffer0);
//                i++;
//            }
//            i = 0;
//
//            SysCtlDelay(SysCtlClockGet() / 10);
//        }
//
//        if (!GPIOPinRead(GPIO_PORTF_BASE, SW2))
//        {
//            time0 = (float) counter0 / 1000.0;
//            counter0 = 0;
//            sprintf(time1, "t = %.2f", time0);
//
//            broadcastMessage(UART0_BASE, time1);
//
//            SysCtlDelay(SysCtlClockGet() / 10);
//        }

        if (i >= 0)
        {
            if (counter0 == 200)
            {
                ctrl0++;
                sprintf(send, "R01P%d", ctrl0);
                broadcastMessage(UART1_BASE, send);
            }

            if (counter0 == 400)
            {
                ctrl1++;
                sprintf(send, "R02P%d", ctrl1);
                broadcastMessage(UART3_BASE, send);
            }

            if (counter0 == 600)
            {
                ctrl2++;
                sprintf(send, "R03P%d", ctrl2);
                broadcastMessage(UART4_BASE, send);
                counter0 = 0;
                i++;
            }
        }

        if(i == 5000)
        {
            i = -1;
            GPIOPinWrite(GPIO_PORTF_BASE, RED_LED, RED_LED);
        }

    }
}

void Timer0IntHandler(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    counter1++;
    counter0++;

//    if(counter0 == 1000)
//    {
//        counter0 = 0;
//    }

    if (counter1 == 1990)
    {
        GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, BLUE_LED);
    }

    if (counter1 == 2000)
    {
        GPIOPinWrite(GPIO_PORTF_BASE, BLUE_LED, 0);
        counter1 = 0;
    }

}

//void broadcastMessage(char message[STR_MAX_SZ])
void broadcastMessage(uint32_t uartBase, char *message)
{
    int length = 0;
    int MSB, LSB, i = 0, j = 0, k = 0;
    int size = 0, checksum, checksumLSB;

    // 64bit coordinator broadcast address
    unsigned char coordAdd64[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00 };
    // 16bit coordinator broadcast address
    unsigned char coordAdd16[] = { 0xFF, 0xFE };

    // Begin dataframe construction
    dataFrame[k] = API_TRA_RE; // Transmission request byte
    k++;
//    dataFrame[k] = API_TRA_ID; // Transmission ID byte
    dataFrame[k] = 0x01; // Transmission ID byte
    k++;

    // Concat 64bit address
    while (j <= 7)
    {
        dataFrame[k] = coordAdd64[j];
        k++;
        j++;
    }
    j = 0;

    // Concat 16bit address
    while (j <= 1)
    {
        dataFrame[k] = coordAdd16[j];
        k++;
        j++;
    }
    j = 0;

    dataFrame[k] = API_BRO_RA; // Broadcast radius
    k++;
    dataFrame[k] = API_OPT_NO; // Broadcast options
    k++;

    // Concat message
    while (message[j] != '\0')
    {
        dataFrame[k] = message[j];
        k++;
        j++;
    }
    j = 0;

    // Find length and size
    while (j < k)
    {
        length++; // The length is the n of chars
        size += dataFrame[j]; // the size is the sum of the chars
        j++;
    }
    j = 0;

    // Find MSB and LSB
    MSB = (length & 0xFF00) >> 8; // 2 greater bytes
    LSB = (length & 0x00FF); // 2 lesser bytes

    // Find checksum
    checksumLSB = (size & 0xFF); // Lesser bytes of the size
    checksum = 0x00FF - checksumLSB; // subtracted of FF

    // Begin building the packet
    apiPacket[i] = API_STR_DE; // Start delimiter byte
    i++;
    apiPacket[i] = MSB; // Length MSB
    i++;
    apiPacket[i] = LSB; // Length LSB
    i++;

    // Concat the dataFrame
    while (j < k)
    {
        apiPacket[i] = dataFrame[j];
        i++;
        j++;
    }
    j = 0;

    // Include checksum
    apiPacket[i] = checksum;

    while (j <= i)
    {
        UARTCharPut(uartBase, apiPacket[j]);
        j++;
    }
    j = 0;

}
