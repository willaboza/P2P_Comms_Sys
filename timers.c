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

bool reload[NUM_TIMERS]     = {0};
uint32_t period[NUM_TIMERS] = {0};
uint32_t backoff[NUM_TIMERS]  = {0};
_callback fn[NUM_TIMERS]    = {0};
bool TX_FLASH_LED = 0;
uint32_t TX_FLASH_TIMEOUT = 0;

// Function To Initialize Timers
void initTimer()
{
    uint8_t i;

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

    for(i = 0; i < NUM_TIMERS; i++)
    {
        period[i] = 0;
        backoff[i]  = 0;
        fn[i]     = 0;
        reload[i] = false;
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
            backoff[i] = seconds;
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
            backoff[i] = seconds;
            fn[i] = callback;
            reload[i] = true;
        }
        i++;
    }
    return found;
}

// Function stops a timer currently in use
bool stopTimer(_callback callback)
{
    uint8_t i = 0;
    bool found = false;
    while(i < NUM_TIMERS && !found)
    {
        found = fn[i] == callback;
        if(found)
        {
            period[i]  = 0;
            backoff[i] = 0;
            fn[i]      = 0;
            reload[i]  = false;
        }
        i++;
    }
    return found;
}

// Restart Timer Previously Initialized
bool restartTimer(_callback callback)
{
    uint8_t i = 0;
    bool found = false;
    while(i < NUM_TIMERS && !found)
    {
        found = fn[i] == callback;
        if(found)
        {
            backoff[i] = period[i];
        }
        i++;
    }
    return found;
}

// Reset all timers
void resetAllTimers()
{
    uint8_t i;
    for(i = 0; i < NUM_TIMERS; i++)
    {
        period[i]  = 0;
        backoff[i] = 0;
        fn[i]      = 0;
        reload[i]  = false;
    }
}

// Reset all timers
void resetTimer(uint8_t index)
{
    period[index]  = 0;
    backoff[index] = 0;
    fn[index]      = 0;
    reload[index]  = false;
}

// Function to handle Timer Interrupts
void tickIsr()
{
    uint8_t i;
    for(i = 0; i < NUM_TIMERS; i++)
    {
        if(backoff[i] != 0)
        {
            backoff[i]--;
            if((backoff[i] == 0))
            {
                currentTableIndex = i; // "Prime the Pump" here
                (*fn[i])();
            }
        }
    }
    if(TX_FLASH_TIMEOUT > 0)
     {
         TX_FLASH_TIMEOUT--;
         if(TX_FLASH_TIMEOUT)
         {
             setPinValue(RED_LED, TX_FLASH_LED = 0); // Turn LED OFF
         }
     }
    TIMER4_ICR_R = TIMER_ICR_TATOCINT;
}

void backoffTimer()
{
    char str[10];

    sprintf(str, "  Transmitting Msg %u, Attempt %u\r\n", table[currentTableIndex].seqId, ++table[currentTableIndex].attempts);
    putsUart0(str);

    sendPacket(currentTableIndex); // "Prime the Pump" here
}

// Calculate New BACKOFF_MS for pending table
uint32_t calcNewBackoff(uint8_t exponent)
{
    uint32_t num, tmp32;

    tmp32 = (calcPower(exponent - 1) * BACKOFF_BASE_NUMBER); // Calculates the value for 2^(n-1)

    if(randomFlag) // Returns new backoff value when random flag is set
    {
        num = BACKOFF_BASE_NUMBER + (rand() * tmp32);
    }
    else // Returns new backoff value when random flag is NOT set
    {
        num = BACKOFF_BASE_NUMBER + tmp32;
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

// Placeholder random number function
uint32_t random32()
{
    return TIMER4_TAV_R;
}
