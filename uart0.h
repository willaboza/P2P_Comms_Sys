/// UART Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#ifndef UART0_H_
#define UART0_H_

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "timers.h"

// Pins
#define UART0_TX PORTA,1
#define UART0_RX PORTA,0
#define QUEUE_BUFFER_LENGTH 80

extern char uart0String[QUEUE_BUFFER_LENGTH];
extern uint8_t writeIndex;
extern uint8_t readIndex;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initUart0();
void setUart0BaudRate(uint32_t baudRate, uint32_t fcyc);
void putcUart0(char c);
void putsUart0(char* str);
char getcUart0();
bool kbhitUart0();
void uart0Isr();
bool emptyRingBuffer();
bool fullRingBuffer();
void writeToQueue(char c);
char readFromQueue();
void sendUart0String(char str[]);

#endif
