// rs485.c
// William Bozarth
// Created on: March 26, 2020

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "uart0.h"
#include "uart1.h"
#include "timers.h"
#include "eeprom.h"
#include "reboot.h"
#include "pwm0.h"

bool carrierSenseFlag      = false;
bool ackFlagSet            = false;
bool randomFlag            = false;
uint8_t SOURCE_ADDRESS     = 1;
uint8_t BACKOFF_ATTEMPTS   = 4;
uint8_t seqId              = 0;
uint8_t messageInProgress  = 0;
uint8_t sum                = 0;

packetFrame txInfo = {0};
packetFrame rxInfo = {0};
pendingTable table[MAX_TABLE_SIZE] = {0};

// Function to get Next Sequence ID
void getNextSeqID(void)
{
    seqId = (seqId + 1) % 256;
}

// Function determines whether to add ACK bit to ackCmd field of message
void setAckFlag(uint8_t index)
{
    if(ackFlagSet)
    {
        table[index].ackCmd  |= 0x80; // Add Ack flag to (MSB) to the field
        table[index].retries = 4;
    }
    else
    {
        table[index].retries = 1; // Set retries to 1 when ACK flag NOT set
    }
}

// When received from PC; where A is the address, C is the Channel, and V is
// the value, a set command shall be sent
void setACV(uint8_t address, uint8_t channel, uint8_t value)
{
    uint8_t index;
    char str[50];

    index = messageInProgress = findEmptySlot();

    // Set Sequence ID for Message
    table[index].seqId = (uint8_t)random32(); // Set Sequence ID for Message

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    sendUart0String(str);

    table[index].ackCmd   = 0x00; // Set Command is 0x00h
    setAckFlag(index);
    table[index].dstAdd   = address;
    table[index].attempts = 0;
    table[index].channel  = channel;
    table[index].data[0]  = value;
    table[index].size     = 1;
    table[index].phase    = 0;             // Byte to transmit is DST_ADD
    table[index].backoff  = 1;             // Initial backoff value is zero to Tx ASAP
    table[index].checksum = 0;
    table[index].checksum = getChecksum(index); // Perform checksum calculations
    table[index].validBit = true;          // Ready to Tx data packet
}

// Function to send a Data Request to a specified Address and Channel
void getAC(uint8_t address, uint8_t channel)
{
    uint8_t index;
    char str[50];

    index = messageInProgress = findEmptySlot();

    // Set Sequence ID for Message
    table[index].seqId = (uint8_t)random32();

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    sendUart0String(str);

    table[index].ackCmd   = 0x20; // Data Request Command is 0x20h
    setAckFlag(index);
    table[index].channel  = channel;
    table[index].data[0]  = 0;
    table[index].size     = 0;
    table[index].dstAdd   = address;
    table[index].attempts = 0;
    table[index].checksum = 0;
    table[index].checksum = getChecksum(index); // Perform checksum calculations
    table[index].phase    = 0;                  // Byte to transmit is DST_ADD
    table[index].backoff  = 1;                  // Backoff set
    table[index].validBit = true;
}

// Function to send a Poll Request
void poll(void)
{
    uint16_t index = 0;
    char str[50];

    index = messageInProgress = findEmptySlot();

    // Set Sequence ID for Message
    table[index].seqId = (uint8_t)random32();

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    sendUart0String(str);

    table[index].dstAdd   = 0xFF;
    table[index].attempts = 0;
    table[index].ackCmd   = 0x78; // Send a Poll Request
    table[index].retries  = 1;    // Set retries to 4 when ACK flag set
    table[index].channel  = 0;
    table[index].size     = 0;
    table[index].phase    = 0;                  // Byte to transmit is DST_ADD
    table[index].backoff  = 1;                  // Backoff set to 500 milliseconds
    table[index].checksum = getChecksum(index); // Perform checksum calculations
    table[index].validBit = true;
}

// Function to Set New Address of Node
void setNewAddress(uint8_t oldAddress, uint8_t newAddress)
{
    uint8_t index;
    char str[50];

    index = messageInProgress = findEmptySlot();

    // Set Sequence ID for Message
    table[index].seqId = (uint8_t)random32();

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    sendUart0String(str);

    table[index].ackCmd   = 0x7A; // Set Command is 0x00h
    setAckFlag(index);
    table[index].channel  = 0;
    table[index].dstAdd   = oldAddress;
    table[index].data[0]  = newAddress;
    table[index].size     = 1;
    table[index].attempts = 0;
    table[index].phase    = 0;             // Byte to transmit is DST_ADD
    table[index].backoff  = 1;             // Initial backoff value is zero to Tx ASAP
    table[index].checksum = getChecksum(index); // Perform checksum calculations
    table[index].validBit = true;          // Ready to Tx data packet
}

// Used to send a Data Report to another node
void sendDataReport(uint8_t address, uint8_t channel, uint8_t value)
{
    int i;
    uint8_t index;
    char str[50];

    index = messageInProgress = findEmptySlot();

    // Set Sequence ID for Message
    table[index].seqId = (uint8_t)random32();

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    sendUart0String(str);

    table[index].dstAdd = address;
    table[index].ackCmd = 0x21; // Data Report is 0x21h
    setAckFlag(index);
    table[index].attempts = 0;
    table[index].channel = channel;

    // Clear out data field with all zeros
    i = 0;
    while(i < DATA_MAX_SIZE)
    {
        table[index].data[i] = 0;
        i++;
    }

    table[index].data[0]  = value;
    table[index].size     = (i - 1);
    table[index].phase    = 0;             // Byte to transmit is DST_ADD
    table[index].backoff  = 1;             // Initial backoff value is zero to Tx ASAP
    table[index].checksum = 0;
    table[index].checksum = getChecksum(index); // Perform checksum calculations
    table[index].validBit = true; // Ready to Tx data packet
}

// Send Request to Node to Reset Device
void sendReset(uint8_t address)
{
    uint8_t index;
    char str[50];

    // Find Spot in Pending Table to Queue Message
    index = messageInProgress = findEmptySlot();

    // Set Sequence ID for Message
    table[index].seqId = (uint8_t)random32();

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    sendUart0String(str);

    table[index].ackCmd = 0x7F; // Reset Command is 0x7Fh
    setAckFlag(index);
    table[index].channel  = 0;
    table[index].data[0]  = 0;
    table[index].dstAdd   = address;
    table[index].attempts = 0;
    table[index].data[0]  = 0;
    table[index].size     = 0;
    table[index].phase    = 0;             // Byte to transmit is DST_ADD
    table[index].backoff  = 1;             // Initial backoff value is zero to Tx ASAP
    table[index].checksum = 0;
    table[index].checksum = getChecksum(index); // Perform checksum calculations
    table[index].validBit = true; // Ready to Tx data packet
}

// Send Request to Node to Reset Device
void sendPulse(uint8_t address, uint8_t channel, uint8_t value, uint16_t duration)
{
    uint8_t index;
    char str[50];

    // Find Spot in Pending Table to Queue Message
    index = messageInProgress = findEmptySlot();

    // Set Sequence ID for Message
    table[index].seqId = (uint8_t)random32();

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    sendUart0String(str);

    table[index].ackCmd = 0x02;                 // Reset Command is 0x7Fh
    setAckFlag(index);                          //
    table[index].channel  = channel;            //
    table[index].dstAdd   = address;            //
    table[index].attempts = 0;                  //
    table[index].data[0]  = value;              //
    table[index].data[1]  = duration >> 8;      //
    table[index].data[2]  = duration;           //
    table[index].size     = 3;                  //
    table[index].phase    = 0;                  // Byte to transmit is DST_ADD
    table[index].backoff  = 1;                  // Initial backoff value is zero to Tx ASAP
    table[index].checksum = 0;                  //
    table[index].checksum = getChecksum(index); // Perform checksum calculations
    table[index].validBit = true;               // Ready to Tx data packet
}

// Creates packet for Tx Ack to destination address
void sendAcknowledge(uint8_t address, uint8_t id)
{
    uint8_t index;
    char str[50];

    index = messageInProgress = findEmptySlot();

    // Set Sequence ID for Message
    table[index].seqId = (uint8_t)random32();

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    sendUart0String(str);

    table[index].dstAdd   = address;
    table[index].ackCmd   = 0x70;               // Data Report is 0x21h
    table[index].attempts = 0;
    table[index].retries  = 1;                  // Set retries to 1 when ACK flag NOT set
    table[index].channel  = 0;
    table[index].data[0]  = id;
    table[index].size     = 1;
    table[index].phase    = 0;                  // Byte to transmit is DST_ADD
    table[index].backoff  = 1;                  // Initial backoff value is zero to Tx ASAP
    table[index].checksum = getChecksum(index); // Perform checksum calculations
    table[index].validBit = true;               // Ready to Tx data packet
}

// Function to Send Currently Assigned Address on Poll Request
void sendPollResponse(uint8_t address)
{
    uint8_t index;
    char str[50];

    index = messageInProgress = findEmptySlot();

    // Set Sequence ID for Message
    table[index].seqId = (uint8_t)random32();

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    sendUart0String(str);

    table[index].dstAdd   = address;
    table[index].ackCmd   = 0x79;               // Poll Response is 0x79h
    table[index].attempts = 0;
    table[index].retries  = 1;                  // Set retries to 1 when ACK flag NOT set
    table[index].channel  = 0;
    table[index].data[0]  = SOURCE_ADDRESS;
    table[index].size     = 1;
    table[index].phase    = 0;                  // Byte to transmit is DST_ADD
    table[index].backoff  = 1;                  // Initial backoff value is zero to Tx ASAP
    table[index].checksum = getChecksum(index); // Perform checksum calculations
    table[index].validBit = true;               // Ready to Tx data packet
}

// Check to see if Tx FIFO EMPTY.
// If UART Tx FIFO Empty write first
// byte of next message, "prime the pump".
void sendPacket(uint8_t index)
{
    uint16_t size, phase;

    // Flash RED_LED when TX Byte
    TX_FLASH_TIMEOUT = 5;
    setPinValue(RED_LED, 1);

    size = table[index].size + 6; // Size of data packet
    phase = table[index].phase;   // Store current phase value

    setPinValue(RED_LED, 0);

    switch(phase)
    {
        // Tx DESTINATION ADDRESS
        case 0:
            UART1_LCRH_R &= ~(UART_LCRH_EPS);
            setPinValue(DRIVER_ENABLE, 1);    // Turn ON Driver Enable (DE) for RS-485 to Tx
            UART1_DR_R = table[index].dstAdd; // Transmit destination address
            table[index].phase = 1;
            UART1_LCRH_R |= UART_LCRH_EPS;
            break;
        // Tx SOURCE_ADDRESS
        case 1:
            // UART1_LCRH_R |= UART_LCRH_EPS;
            UART1_DR_R = SOURCE_ADDRESS;
            table[index].phase = 2;
            break;
        // Tx SEQUENCE ID
        case 2:
            UART1_DR_R = table[index].seqId;
            table[index].phase = 3;
            break;
        // Tx ACK & COMMAND
        case 3:
            UART1_DR_R = table[index].ackCmd;
            table[index].phase = 4;
            break;
        // Tx CHANNEL
        case 4:
            UART1_DR_R = table[index].channel;
            table[index].phase = 5;
            break;
        // Tx SIZE
        case 5:
            UART1_DR_R = table[index].size;
            table[index].phase = 6;
            break;
        // Tx DATA[] and CHECKSUM of packet
        default:
            if(phase < size) // Tx Data
            {
                UART1_DR_R = (table[index].data[phase - 6]); // Subtract by 6 to set initial data stored in array at index = 0
                table[index].phase++; // increment phase
            }
            else if(phase == size) // Tx Checksum
            {
                UART1_DR_R = table[index].checksum;

                while(UART1_FR_R & UART_FR_BUSY);  // Wait until UART is not busy before proceeding

                UART1_LCRH_R &= ~(UART_LCRH_EPS);

                setPinValue(DRIVER_ENABLE, 0);     // Turn OFF Driver Enable (DE) on RS-485

                table[index].retries--;            // Decrement Tx retries remaining
                table[index].phase++;              // Set phase = 0 after Tx checksum

                if(((table[index].ackCmd & 0x80) == 0x80)) // Delete message from Tx Queue
                {
                    table[index].phase = 0;   // Set phase = 0 after Tx checksum
                    table[index].backoff = calcNewBackoff(table[index].attempts);
                }
                else if(((table[index].ackCmd & 0x80) != 0x80))
                {
                    table[index].backoff = BACKOFF_BASE_NUMBER;
                }
            }
    }
}

// Function finds 1st slot with validBit field set to false and returns the index value to it.
uint8_t findEmptySlot(void)
{
    uint8_t i, tmp8;
    bool ok = true;
    i = 0;
    while((i < MAX_TABLE_SIZE) && ok)
    {
        if(!(table[i].validBit))
        {
            tmp8 = i;
            ok = false;
        }
        i++;
    }
    return tmp8;
}

// Calculate sum of words
// Must use getChecksum to complete 1's compliment addition
void sumWords(void)
{
    int i;
    sum = 0;

    sum = rxInfo.dstAdd;
    sum += rxInfo.srcAdd;
    sum += rxInfo.seqId;
    sum += rxInfo.ackCmd;
    sum += rxInfo.channel;
    sum += rxInfo.size;

    for(i = 0; i < rxInfo.size; i++)
    {
        sum += rxInfo.data[i];
    }
}
// Completes 1's compliment addition by folding carries back into field
uint8_t getChecksum(uint8_t index)
{
    int i;
    uint8_t tmp8;
    sum = 0;

    sum = table[index].dstAdd;
    sum += SOURCE_ADDRESS;
    sum += table[index].seqId;
    sum += table[index].ackCmd;
    sum += table[index].channel;
    sum += table[index].size;

    for(i = 0; i < table[index].size; i++)
    {
        sum += table[index].data[i];
    }

    tmp8 = ~(sum);

    return tmp8;
}

// Get Current Channel Value
uint8_t getChannelValue(uint8_t channel)
{
    uint8_t value;

    if(channel == 0)
    {
        value = (GPIO_PORTF_DATA_R & PUSH_BUTTON);
    }
    else if(channel == 1) // Get Red LED value
    {
        value = redLedValue;
    }
    else if(channel == 2) // Get Blue LED value
    {
        value = blueLedValue;
    }
    else if(channel == 3) // Get Green LED value
    {
        value = greenLedValue;
    }

    return value;
}

// Function sets channel values
void setChannelValue(uint8_t value)
{
    uint16_t scaledValue;

    scaledValue = (uint16_t)(((float)value/255)*1023);

    if(rxInfo.channel == 1) // Set Red LED value
    {
        PWM0_1_CMPB_R = scaledValue;
        redLedValue = value;
    }
    else if(rxInfo.channel == 2) //Set Blue LED value
    {
        PWM0_2_CMPA_R = scaledValue;
        blueLedValue = value;
    }
    else if(rxInfo.channel == 3) // Set Green LED value
    {
        PWM0_2_CMPB_R = scaledValue;
        greenLedValue = value;
    }
}

// If Acknowledge Rx'd and SeqID matches then set valid bit for message to 0
void ackReceived(void)
{
    uint8_t i;
    bool ok = true;
    i = 0;
    while((i < MAX_TABLE_SIZE) && ok)
    {
        if(table[i].seqId == rxInfo.data[0])
        {
            table[i].validBit = 0;
            table[i].backoff  = 0;
            table[i].retries  = 0;
            ok = false;
        }
        i++;
    }
}

// Function for handling actions of packets received
void takeAction(void)
{
    uint8_t command, value;
    uint16_t duration, scaledValue;
    char str[80];

    command = (rxInfo.ackCmd & 0x7F); // Mask Bits to remove ACK if set

    switch(command)
    {
        // Set Command
        case 0x00:
            value = rxInfo.data[0];
            // DATA argument for set should be size 1
            setChannelValue(value);
            break;

        // Pulse
        case 0x02:
            // value = rxInfo.data[0];
            scaledValue = (uint16_t)(((float)rxInfo.data[0] / 255) * 1023);
            duration = rxInfo.data[1] << 8;
            duration = rxInfo.data[2];
            if(rxInfo.channel == 1)
            {
                PWM0_1_CMPB_R = scaledValue;
                startOneShotTimer(redLedPulse, duration);
            }
            else if(rxInfo.channel == 2)
            {
                PWM0_2_CMPA_R = scaledValue;
                startOneShotTimer(blueLedPulse, duration);
            }
            else if(rxInfo.channel == 3)
            {
                PWM0_2_CMPB_R = scaledValue;
                startOneShotTimer(greenLedPulse, duration);
            }
            break;

        // Square
        case 0x03:
            break;

        // Data Request
        case 0x20:
            value = getChannelValue(rxInfo.channel);
            sendDataReport(rxInfo.srcAdd, rxInfo.channel, value);
            break;

        // Data Response
        case 0x21:
            sprintf(str, "  Status = %u for Channel %u at Address %u\r\n", rxInfo.data[0], rxInfo.channel, rxInfo.srcAdd);
            sendUart0String(str);
            break;

        // R, G, and B PWM values (brightness)
        case 0x48:
            sprintf(str, "  ACK Received for Msg %u\r\n", rxInfo.data[0]);
            sendUart0String(str);
            break;

        // ACK Command
        case 0x70:
            sprintf(str, "  ACK Received for Msg %u\r\n", rxInfo.data[0]);
            sendUart0String(str);
            break;

        // Poll Request
        case 0x78:
            sendPollResponse(rxInfo.srcAdd);
            break;

        // Poll Response
        case 0x79:
            sprintf(str, "  Address %u Responded\r\n", rxInfo.data[0]);
            sendUart0String(str);
            break;

        // Set Address
        case 0x7A:
            SOURCE_ADDRESS = rxInfo.data[0];
            writeEeprom(0x0000, SOURCE_ADDRESS); // Store New Address in EEPROM
            srand(SOURCE_ADDRESS);
            sprintf(str, "  New Address is %u\r\n", SOURCE_ADDRESS);
            sendUart0String(str);
            break;

        // Node Control
        case 0x7D:
            sendUart0String("  Node Control\r\n");
            break;

        // Bootload
        case 0x7E:
            sendUart0String("  Bootload\r\n");
            break;

        // Reset
        case 0x7F:
            sendUart0String("  Reset\r\n");
            rebootFlag = true; // Setting flag to TRUE will cause controller to restart
            break;

        default:
            sendUart0String("  Command Not Recognized\r\n");
            break;
    }
}

// Function to Display to Terminal Contents of the Pending Table
void displayTableContents(void)
{
    char str[50];

    sprintf(str, "  Current Address is %02u\r\n", SOURCE_ADDRESS);
    sendUart0String(str);
}
