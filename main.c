/**
 * main.c
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "uart1.h"
#include "reboot.h"
#include "terminal.h"
#include "wait.h"
#include "gpio.h"
#include "rs485.h"
#include "timers.h"
#include "eeprom.h"

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
    initEeprom();
    initUart0();
    initUart1();
    initTimer();
//    initWatchdog();

    // Declare Variables
    USER_DATA userInput;

    // Setup UART0 Baud Rate
    setUart0BaudRate(115200, 40e6);

    // Setup UART1 Baud Rate
    setUart1BaudRate(38400, 40e6);

    // Flash LED
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(100000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(100000);

    // Get current value for SOURCE_ADDRESS
    readEepromAddress();

    // Seed random variable with current Source Address
    srand(SOURCE_ADDRESS);

    // Set Variables for User Input to Initial Condition
    resetUserInput(&userInput);

    // Display Main Menu
    putsUart0("\r\n");
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

        // Perform Command from User Input
        if(userInput.endOfString && isCommand(&userInput, "reset", 2))
        {
            char *token;

            token = getFieldInteger(&userInput, 1);

            sendReset(token);

            resetUserInput(&userInput); // Reset Input from User to Rx Next Command
        }
        else if(userInput.endOfString && isCommand(&userInput, "cs", 2))
        {
            char *token;

            token = getFieldString(&userInput, 1);

            if(strcmp(token, "on") == 0)
            {
                putsUart0("  Carrier Sense Detection Enabled\r\n");
            }
            else if(strcmp(token, "off") == 0)
            {
                putsUart0("  Carrier Sense Detection Disabled\r\n");
            }

            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "random", 2))
        {
            char *token;

            strcpy(token,getFieldString(&userInput, 1));

            if(strcmp(token, "on") == 0)
            {
                // Seed Random Function with Unique Address
                // srand(SOURCE_ADDRESS);
                randomFlag = true;
            }
            else if(strcmp(token, "off") == 0)
            {
                randomFlag = false;
            }
            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "set", 4))
        {
            char token[DATA_MAX_SIZE];
            uint8_t address, channel;

            // Get Address, Channel, and Data to be Transmitted
            address = getFieldInteger(&userInput, 1);
            channel = getFieldInteger(&userInput, 2);
            strcpy(token,getFieldString(&userInput, 3));
            setACV(address, channel, token);

            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "get", 3))
        {
            uint8_t address, channel;

            address = getFieldInteger(&userInput, 1);
            channel = getFieldInteger(&userInput, 2);
            getAC(address, channel);

            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "poll", 1))
        {
            poll();
            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "sa", 3))
        {
            uint8_t oldAddress, newAddress;

            oldAddress = getFieldInteger(&userInput, 1);
            newAddress = getFieldInteger(&userInput, 2);
            setNewAddress(oldAddress, newAddress);

            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "ack", 2))
        {
            char *token;

            token = getFieldString(&userInput, 1);

            if(strcmp(token, "on") == 0)
            {
                BACKOFF_ATTEMPTS = 4; // Set to 4 for ACK ON
                ackFlagSet = true;
            }
            else if(strcmp(token, "off") == 0)
            {
                BACKOFF_ATTEMPTS = 1; // Set to 1 for ACK OFF
                ackFlagSet = false;
            }

            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "print", 1))
        {
            displayTableContents();

            resetUserInput(&userInput);
        }
        else if(userInput.endOfString)
        {
            resetUserInput(&userInput);
        }
    }

    return 0;
}
