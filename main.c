

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
    initTimer();
//    initWatchdog();

    // Declare Variables
    USER_DATA userInput;
    uint8_t   data[MAX_PACKET_SIZE];

    // Setup UART0 Baud Rate
    setUart0BaudRate(115200, 40e6);

    // Setup UART1 Baud Rate
    setUart1BaudRate(38400, 40e6);

    // Flash LED
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(100000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(100000);

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
        /*
        // Packet processing
        if(isDataAvailable())
        {

            // Get packet
            getPacket(data, DATA_MAX_SIZE);

            if(ackIsRequired(data))
            {

            }

            if (packetIsUnicast(data)) // Process Unicast Packets
            {
                uint8_t command;

                command = (info.ackCmd & 0x7F); // Mask Bits

                switch(command)
                {
                    case 0x00: putsUart0("  Set\r\n");
                               break;
                    case 0x01: putsUart0("  Piecewise\r\n");
                               break;
                    case 0x02: putsUart0("  Pulse\r\n");
                               break;
                    case 0x03: putsUart0("  Square\r\n");
                               break;
                    case 0x04: putsUart0("  Sawtooth\r\n");
                               break;
                    case 0x05: putsUart0("  Triangle\r\n");
                               break;
                    case 0x20: putsUart0("  Pulse\r\n");
                               break;
                    case 0x21: putsUart0("  Data Request\r\n");
                               break;
                    case 0x22: putsUart0("  Report Control\r\n");
                               break;
                    case 0x40: putsUart0("  LCD Display Text\r\n");
                               break;
                    case 0x48: putsUart0("  RGB\r\n");
                               break;
                    case 0x49: putsUart0("  RGB Piecewise\r\n");
                               break;
                    case 0x50: putsUart0("  UART Data\r\n");
                               break;
                    case 0x51: putsUart0("  UART Data\r\n");
                               break;
                    case 0x52: putsUart0("  UART Control\r\n");
                               break;
                    case 0x54: putsUart0("  Acknowledge\r\n");
                               break;
                    case 0x70: putsUart0("  Poll Request\r\n");
                               break;
                    case 0x78: putsUart0("  Poll Response\r\n");
                               break;
                    case 0x79: putsUart0("  Set Address\r\n");
                               break;
                    case 0x7A: putsUart0("  Acknowledge\r\n");
                               break;
                    case 0x7D: putsUart0("  Poll Request\r\n");
                               break;
                    case 0x7E: putsUart0("  Poll Response\r\n");
                               break;
                    case 0x7F: putsUart0("  Set Address\r\n");
                               break;
                    default:   putsUart0("  Command Not Recognized\r\n");
                               break;
                }
            }
            else // Process Broadcast Packets
            {

            }
        }
        */
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

            putsUart0("  get A C\r\n");
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

            putsUart0("  sa A Anew\r\n");
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
                putsUart0("  ACK Enabled\r\n");
            }
            else if(strcmp(token, "off") == 0)
            {
                BACKOFF_ATTEMPTS = 1; // Set to 1 for ACK OFF
                ackFlagSet = false;
                putsUart0("  ACK Disabled\r\n");
            }

            resetUserInput(&userInput);
        }
        else if(userInput.endOfString && isCommand(&userInput, "print", 1))
        {
            displayTableContents();

            putsUart0("  pending table\r\n");

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
