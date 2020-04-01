/*
 * uart1.c
 *
 *  Created on: Mar 27, 2020
 *      Author: willi
 */

#include "uart1.h"

// Initialize UART1
void initUart1()
{
    // Enable clocks
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R1;
    _delay_cycles(3);

    enablePort(PORTB);
    _delay_cycles(3);

    // Configure UART1 pins
    selectPinPushPullOutput(UART1_TX);
    selectPinDigitalInput(UART1_RX);
    setPinAuxFunction(UART1_TX, GPIO_PCTL_PB1_U1TX);
    setPinAuxFunction(UART1_RX, GPIO_PCTL_PB0_U1RX);

    // Configure DE Pin for RS-485
    selectPinPushPullOutput(DRIVER_ENABLE);

    // Configure UART1 with default baud rate
    UART1_CTL_R = 0;                                     // turn-off UART1 to allow safe programming
    UART1_CC_R  = UART_CC_CS_SYSCLK;                     // use system clock (usually 40 MHz)
}

// Set baud rate as function of instruction cycle frequency
void setUart1BaudRate(uint32_t baudRate, uint32_t fcyc)
{
    uint32_t divisorTimes128 = (fcyc * 8) / baudRate;   // calculate divisor (r) in units of 1/128,
                                                        // where r = fcyc / 16 * baudRate
    UART1_CTL_R = 0;                                    // turn-off UART1 to allow safe programming
    UART1_IBRD_R = divisorTimes128 >> 7;                // set integer value to floor(r)
    UART1_FBRD_R = ((divisorTimes128 + 1) >> 1) & 63;   // set fractional value to round(fract(r)*64)
    UART1_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN | UART_LCRH_SPS | UART_LCRH_PEN;    // configure for 8N1 w/ 16-level FIFO
    UART1_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN; //on UART1
}

// Blocking function that writes a serial character when the UART buffer is not full
void putcUart1(char c)
{
    while (UART1_FR_R & UART_FR_TXFF);               // wait if uart1 tx fifo full
    UART1_DR_R = c;                                  // write character to fifo
}

// Blocking function that writes a string when the UART buffer is not full
void putsUart1(char* str)
{
    uint8_t i = 0;
    while (str[i] != '\0')
        putcUart1(str[i++]);
}

// Blocking function that returns with serial data once the buffer is not empty
char getcUart1()
{
    return UART1_DR_R & 0xFF;                        // get character from fifo
}

// Returns the status of the receive buffer
bool isDataAvailable()
{
    return !(UART1_FR_R & UART_FR_RXFE);
}

// Implement the ISR for UART1
void uart1Isr()
{
    uint16_t i;
    bool ok = true;

    // Check to see if Tx FIFO EMPTY
    if(!(UART1_FR_R & UART_FR_TXFE))
    {
        sendPacket(currentTableIndex); // Transmit next byte of packet
    }
    else // If packet not being transmitted, check if another is available in pending table to transmit
    {
        i = 0;
        while((i < MAX_TABLE_SIZE) && ok)
        {
            if(table[i].validBit && (table[i].backoff == 0)) // Find next packet to Tx
            {
                currentTableIndex = i; // store new table index value to process
                ok = false; // set to exit while loop
                sendPacket(currentTableIndex); // "prime the pump"
            }
            i++;
        }
    }

    UART1_ICR_R = 0xFF; // Writing a 1 to the bits in this register clears the bits in the UARTRIS and UARTMIS registers
}
