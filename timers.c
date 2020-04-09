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

bool TX_FLASH_LED = 0;
bool RX_FLASH_LED = 0;
uint32_t TX_FLASH_TIMEOUT = 0;
uint32_t RX_FLASH_TIMEOUT = 0;

// Function To Initialize Timers
void initTimer()
{
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
}

// Function to handle Timer Interrupts
void tickIsr()
{
    uint8_t i;
    for(i = 0; i < NUM_TIMERS; i++)
    {
        if(table[i].backoff != 0 && table[i].validBit)
        {
            table[i].backoff--;
            // If validBit = 1 && backoff = 0 can start to send message
            if(table[i].backoff == 0)
            {
                char str[50];
                messageInProgress = i;
                // "Prime the Pump" here
                if(UART1_FR_R & UART_FR_TXFE)
                {
                    sprintf(str, "  Transmitting Msg %u, Attempt %u\r\n", table[i].seqId, ++table[i].attempts);
                    sendUart0String(str);
                    UART1_LCRH_R &= ~UART_LCRH_EPS;   // turn-off EPS before Tx dstAdd, sets parity bit = 1
                    sendPacket(messageInProgress);
                }
            }
        }
    }
    if(TX_FLASH_TIMEOUT > 0)
     {
         TX_FLASH_TIMEOUT--;
         if(TX_FLASH_TIMEOUT == 0)
         {
             setPinValue(RED_LED, TX_FLASH_LED = 0); // Turn LED OFF
         }
     }
    if(RX_FLASH_TIMEOUT > 0)
     {
         RX_FLASH_TIMEOUT--;
         if(RX_FLASH_TIMEOUT == 0)
         {
             setPinValue(GREEN_LED, RX_FLASH_LED = 0); // Turn LED OFF
         }
     }
    TIMER4_ICR_R = TIMER_ICR_TATOCINT;
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
