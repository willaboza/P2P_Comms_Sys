// Timer Service Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Timer 4

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include "timers.h"

bool invert[NUM_CHANNELS]   = {0};
bool reload[NUM_CHANNELS]     = {0};
uint32_t period[NUM_CHANNELS] = {0};
uint32_t ticks[NUM_CHANNELS]  = {0};
_callback fn[NUM_CHANNELS]    = {0};

bool TX_FLASH_LED = 0;
bool RX_FLASH_LED = 0;
uint32_t TX_FLASH_TIMEOUT = 0;
uint32_t RX_FLASH_TIMEOUT = 0;

// Function To Initialize Timers
void initTimer()
{
    int i;

    // Enable clocks
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R4;
    _delay_cycles(3);

    TIMER4_CTL_R   &= ~TIMER_CTL_TAEN;                     // turn-off counter before reconfiguring
    TIMER4_CFG_R   = TIMER_CFG_32_BIT_TIMER;               // configure as 32-bit counter
    TIMER4_TAMR_R  = TIMER_TAMR_TAMR_PERIOD;               // configure for one-shot mode, count down
    TIMER4_TAILR_R = 40000;                                // Need to have a 1 kHz clock
    TIMER4_CTL_R   |= TIMER_CTL_TAEN;
    TIMER4_IMR_R   |= TIMER_IMR_TATOIM;                    // enable interrupts
    NVIC_EN2_R     |= 1 << (INT_TIMER4A-80);               // turn-on interrupt 86 (TIMER4A)

    for(i = 0; i < NUM_CHANNELS; i++)
    {
        period[i] = 0;
        ticks[i]  = 0;
        fn[i]     = 0;
        reload[i] = false;
    }
}

// Function to handle Timer Interrupts
void tickIsr()
{
    uint8_t i;
    for(i = 0; i < NUM_TIMERS; i++)
    {
        if((table[i].backoff != 0) && table[i].validBit)
        {
            table[i].backoff--;
            // If valid bit is 1 and backoff is 0 can start to send message
            if(table[i].backoff == 0)
            {
                char str[80];
                messageInProgress = i;

                if((table[i].retries > 0) && (table[i].validBit == true) && (UART1_FR_R & UART_FR_TXFE)) // "Prime the Pump" here
                {
                    sprintf(str, "  Transmitting Msg %u, Attempt %u\r\n", table[i].seqId, ++table[i].attempts);
                    sendUart0String(str);
                    sendPacket(messageInProgress);
                }

                // If ACK Flag set and Retries = 0 then output error message
                if((table[i].retries == 0) && ((table[i].ackCmd & 0x80) == 0x80))
                {
                    // When Checksum set flash RED_LED
                    TX_FLASH_TIMEOUT = 10000;
                    setPinValue(RED_LED, 1);

                    sprintf(str, "  Error Sending Msg %u\r\n", table[i].seqId);
                    sendUart0String(str);

                    // Set Msg for Removal
                    table[i].validBit = false;
                }
                else if(table[i].retries == 0 && ((table[i].ackCmd & 0x80) != 0x80)) // If ACK command not set then set valid bit to false
                {
                    table[i].validBit = false; // Set valid bit to false as finished transmitting the packet
                }
            }
        }
    }

    for(i = 0; i < NUM_CHANNELS; i++)
    {
        if(ticks[i] != 0)
        {
            ticks[i]--;
            if(ticks[i] == 0)
            {
                if(reload[i])
                    ticks[i] = period[i];
                (*fn[i])();
            }
        }
    }

    if(TX_FLASH_TIMEOUT > 0)
    {
        TX_FLASH_TIMEOUT--;
        if(TX_FLASH_TIMEOUT == 0)
        {
            setPinValue(RED_LED, 0); // Turn LED OFF
        }
    }
    if(RX_FLASH_TIMEOUT > 0)
    {
        RX_FLASH_TIMEOUT--;
        if(RX_FLASH_TIMEOUT == 0)
        {
            setPinValue(GREEN_LED, 0); // Turn LED OFF
        }
     }
    TIMER4_ICR_R = TIMER_ICR_TATOCINT;
}

// Calculate New BACKOFF_MS for pending table
uint32_t calcNewBackoff(uint8_t exponent)
{
    uint32_t num, offset;
    uint8_t random;
    offset = (calcPower(exponent - 1) * BACKOFF_BASE_NUMBER); // Calculates the value for 2^(n-1)

    if(randomFlag) // Returns new backoff value when random flag is set
    {
        random = rand();
        num = BACKOFF_BASE_NUMBER + (uint32_t)(((float)random/255) * (float)offset);
    }
    else // Returns new backoff value when random flag is NOT set
    {
        num = BACKOFF_BASE_NUMBER + offset;
    }
    return num;
}

// Function calculates the power of a number and returns its solution
uint32_t calcPower(uint8_t exponent)
{
    // uint32_t solution;
    if(exponent != 0)
    {
        return (2 * calcPower(exponent - 1));
    }
    else
    {
        return 1;
    }
}

// Function to Start One Shot Timer
bool startOneShotTimer(_callback callback, uint32_t seconds)
{
    uint8_t i = 0;
    bool found = false;
    while(i < NUM_TIMERS && !found)
    {
        found = fn[i] == NULL;
        if (found)
        {
            period[i] = seconds;
            ticks[i] = seconds;
            fn[i] = callback;
            reload[i] = false;
        }
        i++;
    }
    return found;
}

// Function to Start Periodic Timer
bool startPeriodicTimer(_callback callback, uint32_t seconds)
{
    uint8_t i = 0;
    bool found = false;
    while(i < NUM_TIMERS && !found)
    {
        found = fn[i] == NULL;
        if (found)
        {
            period[i] = seconds;
            ticks[i] = seconds;
            fn[i] = callback;
            reload[i] = true;
        }
        i++;
    }
    return found;
}

// Channel 1 Pulse callback
void redLedPulse()
{
    PWM0_1_CMPB_R = 0;
    redLedValue = 0;
    stopTimer(redLedPulse);
}

// Channel 2 Pulse callback
void blueLedPulse()
{
    PWM0_2_CMPA_R = 0;
    blueLedValue  = 0;
    stopTimer(blueLedPulse);
}

// Channel 3 Pulse callback
void greenLedPulse()
{
    PWM0_2_CMPB_R = 0;
    greenLedValue = 0;
    stopTimer(greenLedPulse);
}

//
bool stopTimer(_callback callback)
{
    uint8_t i = 0;
    bool found = false;
    for(i = 0; i < NUM_CHANNELS; i++)
    {
        found = fn[i] == callback;
        if(found)
        {
            period[i] = 0;
            ticks[i]  = 0;
            fn[i]     = 0;
            reload[i] = false;
        }
    }
    return found;
}


// Placeholder random number function
uint32_t random32()
{
    return TIMER4_TAV_R;
}
