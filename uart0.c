// UART0 Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

// Hardware configuration:
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the micro-controller

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include "uart0.h"

char uart0String[QUEUE_BUFFER_LENGTH] = {0};
uint8_t writeIndex = 0;
uint8_t readIndex = 0;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize UART0
void initUart0()
{
    // Enable clocks
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R0;
    _delay_cycles(3);

    enablePort(PORTA);
    _delay_cycles(3);

    // Configure UART0 pins
    selectPinPushPullOutput(UART0_TX);
    selectPinDigitalInput(UART0_RX);
    setPinAuxFunction(UART0_TX, GPIO_PCTL_PA1_U0TX);
    setPinAuxFunction(UART0_RX, GPIO_PCTL_PA0_U0RX);

    // Configure UART0 with default baud rate
    UART0_CTL_R = 0;                                    // turn-off UART0 to allow safe programming
    UART0_CC_R  = UART_CC_CS_SYSCLK;                     // use system clock (usually 40 MHz)
}

// Set baud rate as function of instruction cycle frequency
void setUart0BaudRate(uint32_t baudRate, uint32_t fcyc)
{
    uint32_t divisorTimes128 = (fcyc * 8) / baudRate;   // calculate divisor (r) in units of 1/128,
                                                        // where r = fcyc / 16 * baudRate
    UART0_CTL_R  = 0;                                   // turn-off UART0 to allow safe programming
    UART0_IBRD_R = divisorTimes128 >> 7;                // set integer value to floor(r)
    UART0_FBRD_R = ((divisorTimes128 + 1) >> 1) & 63;   // set fractional value to round(fract(r)*64)
    UART0_LCRH_R = UART_LCRH_WLEN_8;                    // configure for 8N1 w/ 16-level FIFO
    UART0_CTL_R  = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN; // turn-on UART0
    UART0_IM_R = UART_IM_TXIM;                          // turn-on TX interrupt
    NVIC_EN0_R |= 1 << (INT_UART0-16);                  // turn-on interrupt 21 (UART0)
}

// Blocking function that writes a serial character when the UART buffer is not full
void putcUart0(char c)
{
    while (UART0_FR_R & UART_FR_TXFF); // wait if uart0 tx fifo full
    UART0_DR_R = c;                    // write character to fifo
}

// Blocking function that writes a string when the UART buffer is not full
void putsUart0(char* str)
{
    uint8_t i = 0;
    while (str[i] != '\0')
        putcUart0(str[i++]);
}

// Blocking function that returns with serial data once the buffer is not empty
char getcUart0()
{
    return UART0_DR_R & 0xFF;                        // get character from fifo
}

// Returns the status of the receive buffer
bool kbhitUart0()
{
    return !(UART0_FR_R & UART_FR_RXFE);
}

void sendUart0String(char str[])
{
    int i = 0;

    // Write string to Tx Ring Buffer
    while(str[i] != '\0')
    {
        // If NOT full write next Character
        if(!(fullRingBuffer()))
        {
            writeToQueue(str[i]);
            i++;
        }
        //i++;
    }

    // Check to see if UART Tx holding register is empty
    if(UART0_FR_R & UART_FR_TXFE)
    {
        UART0_DR_R = readFromQueue(); // "Prime Pump" by writing 1st char to Uart0
    }
}

// Update current position of writeIndex variable for Ring Buffer
void writeToQueue(char c)
{
    uart0String[writeIndex] = c;
    writeIndex = (writeIndex + 1) % QUEUE_BUFFER_LENGTH;
}

// Update current position of readIndex variable for Ring Buffer
char readFromQueue()
{
    char c;
    c = uart0String[readIndex];
    readIndex = (readIndex + 1) % QUEUE_BUFFER_LENGTH;
    return c;
}

// Returns true if ring buffer is EMPTY
bool emptyRingBuffer()
{
    bool ok = false;

    if(writeIndex == readIndex)
    {
        ok = true;
    }

    return ok;
}

// Returns true if ring buffer is FULL
bool fullRingBuffer()
{
    bool ok = false;

    if(((writeIndex + 1) % QUEUE_BUFFER_LENGTH) == readIndex)
    {
        ok = true;
    }

    return ok;
}

// Handle UART0 Interrupts
void uart0Isr()
{
    // Writing a 1 to the bits in this register clears the bits in the UARTRIS and UARTMIS registers
    UART0_ICR_R = 0xFF;
    // Check to see if UART Tx holding register is empty and send next byte of data
    if((UART0_FR_R & UART_FR_TXFE) && !(emptyRingBuffer()))
    {
        UART0_DR_R = readFromQueue();
    }
}