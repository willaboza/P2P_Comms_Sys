/*
 * uart1.c
 *
 *  Created on: Mar 27, 2020
 *      Author: willi
 */

#include "uart1.h"

uint16_t rxPhase = 0;

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
    //selectPinDigitalOutput(DRIVER_ENABLE);
    selectPinPushPullOutput(DRIVER_ENABLE);

    // Configure UART1 with default baud rate
    UART1_CTL_R = 0;                             // Turn-off UART1 to allow safe programming
    UART1_CC_R  = UART_CC_CS_SYSCLK;             // Use system clock (usually 40 MHz)
}

// Set baud rate as function of instruction cycle frequency
void setUart1BaudRate(uint32_t baudRate, uint32_t fcyc)
{
    uint32_t divisorTimes128 = (fcyc * 8) / baudRate;   // calculate divisor (r) in units of 1/128,
                                                        // where r = fcyc / 16 * baudRate
    UART1_CTL_R  = 0;                                   // turn-off UART1 to allow safe programming
    UART1_IBRD_R = divisorTimes128 >> 7;                // set integer value to floor(r)
    UART1_FBRD_R = ((divisorTimes128 + 1) >> 1) & 63;   // set fractional value to round(fract(r)*64)
    UART1_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_SPS | UART_LCRH_PEN;    // configure for 8N1 w/ 16-level FIFO
    UART1_CTL_R  = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;       //on UART1
    UART1_IM_R   = UART_IM_TXIM | UART_IM_RXIM;
    NVIC_EN0_R |= 1 << (INT_UART1-16);                  // turn-on interrupt 22 (UART1)
}

// Implement the ISR for UART1
void uart1Isr()
{
    uint8_t tmp8;
    bool ok = true;

    // Check to see if UART Tx holding register is empty
    if(UART1_FR_R & UART_FR_TXFE)
    {
        sendPacket(currentTableIndex); // Transmit next byte of packet
    }

    // Check to see if Rx is NOT empty
    if(UART1_FR_R & UART_FR_RXFE)
    {
        // Check if Parity = 1 for received data by using XOR of EPS and PE bits
        if((UART1_LCRH_R & UART_LCRH_EPS) ^ (UART1_DR_R & UART_DR_PE))
        {
            rxPhase = 0; // Destination Address Rx'd so set phase = 0
        }

        switch(rxPhase)
        {
            case 0: // Check if Destination Address is Broadcast or the Address Assigned to the device
                tmp8 = UART1_DR_R; // Read data received on UART1
                if(tmp8 == SOURCE_ADDRESS || tmp8 == 0xFF)
                {
                    rxInfo.dstAdd = tmp8; // Store dstAdd for use in checksum verification
                    rxPhase = 1;            // Set phase = 1
                }
                break;
            // Store Source Address & increment phase
            case 1:
                tmp8 = UART1_DR_R; // Read data received on UART1
                rxInfo.srcAdd = tmp8;
                rxPhase = 2;
                break;
            // Store Sequence ID & increment phase
            case 2:
                tmp8 = UART1_DR_R; // Read data received on UART1
                rxInfo.seqId = tmp8;
                rxPhase = 3;
                break;
            // Store ACK & COMMAND, increment phase
            case 3:
                tmp8 = UART1_DR_R; // Read data received on UART1
                rxInfo.ackCmd = tmp8;
                rxPhase = 4;
                break;
            // Store ACK & COMMAND, increment phase
            case 4:
                tmp8 = UART1_DR_R; // Read data received on UART1
                rxInfo.ackCmd = tmp8;
                rxPhase = 5;
                break;
            // Store CHANNEL, increment phase
            case 5:
                tmp8 = UART1_DR_R; // Read data received on UART1
                rxInfo.channel = tmp8;
                rxPhase = 6;
                break;
            // Store SIZE, increment phase
            case 6:
                tmp8 = UART1_DR_R; // Read data received on UART1
                rxInfo.size = tmp8;
                rxPhase = 7;
                break;
            // Handle DATA[] and CHECKSUM
            default:
                if(rxPhase < (6 + rxInfo.size)) // Process Data
                {
                    tmp8 = UART1_DR_R; // Read data received on UART1
                    rxInfo.data[rxPhase-7] = tmp8; // Subtract by 7 to set initial data stored in array at index = 0
                    rxPhase++; // increment phase
                }
                else if(rxPhase == (6 + rxInfo.size)) // Store Checksum
                {
                    tmp8 = UART1_DR_R; // Read data received on UART1
                    rxInfo.checksum = tmp8;
                    rxPhase++;
                }
                else if(rxPhase == (6 + 1 + rxInfo.size)) // Verify if Checksum is good
                {
                    sum = 0;
                    size = rxInfo.size;
                    // Get sum of bytes for data Rx'd
                    sumWords(&rxInfo.dstAdd, 6);
                    sumWords(&rxInfo.data, size);

                    tmp8 = (sum + rxInfo.checksum); // Sum all fields for Rx'd packet and add with sender's checksum
                    // If tmp8 = 0xFF then packet Rx'd is valid
                    if(tmp8 == 0xFF)
                    {
                        takeAction(); // Perform action for Command Rx'd
                    }
                    else
                    {
                        rxPhase = 0; // Discard packet if checksum invalid and look for new packet to process
                    }
                }
        }
    }
    UART1_ICR_R = 0xFF; // Writing a 1 to the bits in this register clears the bits in the UARTRIS and UARTMIS registers
}
