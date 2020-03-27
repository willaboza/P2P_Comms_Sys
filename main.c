

/**
 * main.c
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <reboot.h>
#include "tm4c123gh6pm.h"
#include "terminal.h"
#include "wait.h"
#include "uart.h"
#include "gpio.h"
#include "table.h"
#include "timers.h"
//#include "reboot.h"

// Pins
#define RED_LED PORTF,     1
#define BLUE_LED PORTF,    2
#define GREEN_LED PORTF,   3
#define PUSH_BUTTON PORTF, 4

// Function to Initialize Hardware
void initHw()
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, sysdivider of 5, creating system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S);

    // Enable clocks
    enablePort(PORTF);
    _delay_cycles(3);

    // Configure LED and pushbutton pins
    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(GREEN_LED);
    selectPinPushPullOutput(BLUE_LED);
    selectPinDigitalInput(PUSH_BUTTON);
}

int main(void)
{
    // Initialize Hardware
    initHw();
    initUart0();
    initUart1();
//    initTimers();
//    initWatchdog();

    // Declare Variables
    USER_DATA userInput;

    // Setup UART0 Baud Rate
    setUart0BaudRate(115200, 40e6);

    // Setup UART1 Baud Rate
    setUart1BaudRate(115200, 40e6);

    // Flash LED
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(100000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(100000);

    // Set Variables for User Input to Initial Condition
    resetUserInput(&userInput);

    // Display Main Menu
    printMainMenu();

    while(true)
    {
        // If User Input detected, then process input
        if(kbhitUart0())
        {
            // Get User Input
            getsUart0(&userInput);

            // Tokenize User Input
            parseFields(&userInput);
        }


        // Packet processing
        if(isDataAvailableUart1())
        {

            // Get packet
            uartGetPacket();
        }

        if(userInput.endOfString && isCommand(&userInput, "reset", 2))
        {
            char *token;

            token = getFieldString(&userInput, 1);

            if(strcmp(token, "A") == 0)
            {
                putsUart0("Reset sent to address A\r\n");
            }

            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "cs", 2))
        {
            char *token;

            token = getFieldString(&userInput, 1);

            if(strcmp(token, "on") == 0)
            {
                putsUart0("Carrier Sense Detection Enabled\r\n");
            }
            else if(strcmp(token, "off") == 0)
            {
                putsUart0("Carrier Sense Detection Disabled\r\n");
            }

            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "random", 2))
        {
            char *token;

            token = getFieldString(&userInput, 1);

            if(strcmp(token, "on") == 0)
            {
                putsUart0("Random Retransmission Enabled\r\n");
            }
            else if(strcmp(token, "off") == 0)
            {
                putsUart0("Random Retransmission Disabled\r\n");
            }
            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "set", 4))
        {
            char token[MAX_CHARS + 1];
            uint8_t address, channel, value;

            // Retrieve network configuration parameter
            strcpy(token,getFieldString(&userInput, 1));

            // Get Network Address
            address = getFieldInteger(&userInput, 2);
            channel = getFieldInteger(&userInput, 3);
            value   = getFieldInteger(&userInput, 4);

            setACV(address, channel, value);

            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "get", 3))
        {
            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "poll", 1))
        {
            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "sa", 3))
        {
            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "ack", 2))
        {
            char *token;

            token = getFieldString(&userInput, 1);

            if(strcmp(token, "on") == 0)
            {
                ackFlagSet = true;
                putsUart0("ACK Enabled\r\n");
            }
            else if(strcmp(token, "off") == 0)
            {
                ackFlagSet = false;
                putsUart0("ACK Disabled\r\n");
            }

            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "print", 1))
        {

            displayTableContents();
            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "reboot", 1))
        {
            putsUart0("Rebooting System...\r\n");
            //rebootFlag = true;
        }
        else if(userInput.endOfString)
        {
            resetUserInput(&userInput);
        }
    }

    return 0;
}
