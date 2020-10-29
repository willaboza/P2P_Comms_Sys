// main.c
// William Bozarth
// Created on: October 7, 2020

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

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
#include "shell.h"
#include "gpio.h"
#include "rs485.h"
#include "timers.h"
#include "eeprom.h"
#include "pwm0.h"
#include "wait.h"

// Function to Initialize Hardware
void initHw(void)
{
    // Configure HW to work with 16 MHz XTAL, PLL enabled, sysdivider of 5, creating system clock of 40 MHz
    SYSCTL_RCC_R = SYSCTL_RCC_XTAL_16MHZ | SYSCTL_RCC_OSCSRC_MAIN | SYSCTL_RCC_USESYSDIV | (4 << SYSCTL_RCC_SYSDIV_S) | SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_2;

    // Enable clocks
    enablePort(PORTF);
    _delay_cycles(3);


    // Configure LED and pushbutton pins
    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(GREEN_LED);
    selectPinPushPullOutput(BLUE_LED);
    selectPinDigitalInput(PUSH_BUTTON);
}

void main(void)
{
    // Initialize Hardware
    initHw();
    initEeprom();
    initUart0();
    initUart1();
    initTimer();
    initPwm0();
    initWatchdog();

    // Declare Variables
    USER_DATA userInput;

    // Setup UART0 Baud Rate
    setUart0BaudRate(115200, 40e6);

    // Setup UART1 Baud Rate
    setUart1BaudRate(38400, 40e6);

    // Get current value for SOURCE_ADDRESS
    readEepromAddress();

    // Seed random variable with current Source Address
    // srand(SOURCE_ADDRESS);
    srand(SOURCE_ADDRESS);
    setPinValue(GREEN_LED, 1);


    //test green LED is functioning correctly (turn on and then off)
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(100000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(100000);

    //Print Main Menu
    printMainMenu();

    while(true)
    {
        // Re-set user input
        resetUserInput(&userInput);

        // Spin until user input if complete
        while(!userInput.endOfString)
        {
            // If User Input detected, then process input
            if(kbhitUart0())
            {
                // Get User Input
                getsUart0(&userInput);

                // Tokenize User Input
                parseFields(&userInput);
            }
        }

        /*
        // If PB pressed broadcast change to other devices
        if((getPortValue(PORTF) & getPinValue(PUSH_BUTTON)))
        {
            sendDataReport(255, 0, 0); // Send value of 0 for PB pressed
        }
        */

        // Perform Command from User Input
        if(isCommand(&userInput, "reset", 2))
        {
            uint8_t destAddress;

            destAddress = getFieldInteger(&userInput, 1);

            sendReset(destAddress);
        }
        else if(isCommand(&userInput, "cs", 2))
        {
            char token[10];

            getFieldString(&userInput, token, 1);

            if(strcmp(token, "on") == 0)
            {
                sendUart0String("  Carrier Sense Detection Enabled\r\n");
            }
            else if(strcmp(token, "off") == 0)
            {
                sendUart0String("  Carrier Sense Detection Disabled\r\n");
            }
        }
        else if(isCommand(&userInput, "random", 2))
        {
            char token[80];

            getFieldString(&userInput, token, 1);

            if(strcmp(token, "on") == 0)
            {
                // Seed Random Function with Unique Address
                srand(SOURCE_ADDRESS);
                randomFlag = true;
            }
            else if(strcmp(token, "off") == 0)
            {
                randomFlag = false;
            }
        }
        else if(isCommand(&userInput, "set", 4))
        {
            uint8_t address, channel, value;

            // Get Address, Channel, and Data to be Transmitted
            address = getFieldInteger(&userInput, 1);
            channel = getFieldInteger(&userInput, 2);
            value   = getFieldInteger(&userInput, 3);

            setACV(address, channel, value);
        }
        else if(isCommand(&userInput, "get", 3))
        {
            uint8_t address, channel;

            address = getFieldInteger(&userInput, 1);
            channel = getFieldInteger(&userInput, 2);
            getAC(address, channel);
        }
        else if(isCommand(&userInput, "poll", 1))
        {
            poll();
        }
        else if(isCommand(&userInput, "sa", 3))
        {
            uint8_t oldAddress, newAddress;

            oldAddress = getFieldInteger(&userInput, 1);
            newAddress = getFieldInteger(&userInput, 2);
            setNewAddress(oldAddress, newAddress);
        }
        else if(isCommand(&userInput, "ack", 2))
        {
            char token[10];

            getFieldString(&userInput, token, 1);

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
        }
        else if(isCommand(&userInput, "pulse", 4))
        {
            uint8_t address, channel, value;
            uint16_t duration;

            address  = getFieldInteger(&userInput, 1);
            channel  = getFieldInteger(&userInput, 2);
            value    = getFieldInteger(&userInput, 3);
            duration = getFieldInteger(&userInput, 4);

            // sendPulse(address, channel, value, duration);
        }
        else if(isCommand(&userInput, "print", 1))
        {
            displayTableContents();
        }
    }
}
