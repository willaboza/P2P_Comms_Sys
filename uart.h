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

#ifndef UART_H_
#define UART_H_

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initUart0();
void initUart1();
void setUart0BaudRate(uint32_t baudRate, uint32_t fcyc);
void setUart1BaudRate(uint32_t baudRate, uint32_t fcyc);
void putcUart0(char c);
void putcUart1(char c);
void putsUart0(char* str);
void putsUart1(char* str);
char getcUart0();
char getcUart1();
bool kbhitUart0();
bool isDataAvailableUart1();

#endif
