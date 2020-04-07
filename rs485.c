/*
 * rs485.c
 *
 *  Created on: Mar 26, 2020
 *      Author: willi
 */

#include "rs485.h"

bool carrierSenseFlag      = false;
bool ackFlagSet            = false;
bool randomFlag            = false;
uint8_t SOURCE_ADDRESS     = 1;
uint8_t BACKOFF_ATTEMPTS   = 4;
uint8_t lastSequenceId     = 0;
uint8_t currentTableIndex  = 0;
uint8_t sum                = 0;

packetFrame info = {0};
pendingTable table[MAX_TABLE_SIZE] = {0};

// Determines if packet received is unicast
bool packetIsUnicast(uint8_t packet[])
{
    packetFrame* info = (packetFrame*)packet;
    bool ok = false;
    if(!(info->dstAdd & 0xFF)) // Check if Message is NOT broadcast
    {
        ok = true;
    }
    if((UART1_LCRH_R & 0xFF) == 0x82 && info->dstAdd == SOURCE_ADDRESS)
    {
        ok = true; //
    }
    else
    {
        ok = false;
    }
    return ok;
}

// Determines if packet received requires an ACK to be sent back to sender
bool ackIsRequired(uint8_t packet[])
{
    packetFrame* info = (packetFrame*)packet;
    bool ok = false;
    if(((info->dstAdd >> 7) & 0xFF) == 0x01) // Shift bits right by 7 and then apply mask
        ok = true;
    return ok;
}

// Function to get Input from Terminal
void getPacket(uint8_t packet[], uint16_t maxSize)
{
    packetFrame* info = (packetFrame*)packet;
    uint16_t i;
    uint8_t trueSize;

    info->dstAdd   = getcUart1();
    info->srcAdd   = getcUart1();
    info->seqId    = getcUart1();
    info->ackCmd   = getcUart1();
    info->channel  = getcUart1();
    info->size     = getcUart1();

    if(info->size > maxSize)
    {
        trueSize = maxSize;
    }
    else
    {
        trueSize = info->size;
    }

    for(i = 0; i < trueSize; i++)
    {
        info->data[i] = getcUart1();
    }

    info->data[i] = '\0'; // add NULL terminating character to end of data

    info->checksum = getcUart1();
}

// When received from PC; where A is the address, C is the Channel, and V is
// the value, a set command shall be sent
void setACV(uint8_t address, uint8_t channel, char c[])
{
    int i;
    uint8_t index;
    char str[50];

    index = currentTableIndex = findEmptySlot();

    table[index].seqId = (uint8_t)random32(); // Set Sequence ID for Message

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    putsUart0(str);

    table[index].dstAdd = address;
    table[index].ackCmd = 0x00; // Set Command is 0x00h
    table[index].attempts = 0;
    if(ackFlagSet)
    {
        table[index].ackCmd = (table[index].ackCmd | 0x80); // Add Ack flag to (MSB) to the field
        table[index].retries = 4;
    }
    else
    {
        table[index].retries = 1; // Set retries to 1 when ACK flag NOT set
    }

    table[index].channel = channel;

    // Clear out data field with all zeros
    i = 0;
    while(i < DATA_MAX_SIZE)
    {
        table[index].data[i] = 0;
        i++;
    }
    // Add user data
    i = 0;
    while(c[i] != '\0')
    {
        table[index].data[i] = c[i];
        i++;
    }

    table[index].size     = i;
    table[index].phase    = 0;             // Byte to transmit is DST_ADD
    table[index].backoff  = 1;             // Initial backoff value is zero to Tx ASAP
    table[index].checksum = 0;
    setPacketFrame(index);                 // Set values in packetFrame for checksum calculations
    table[index].checksum = getChecksum(); // Perform checksum calculations

    // When Checksum set
    setPinValue(RED_LED, TX_FLASH_LED = 1);
    TX_FLASH_TIMEOUT = 1000;

    table[index].validBit = true; // Ready to Tx data packet
}

// Function to send a Data Request to a specified Address and Channel
void getAC(uint8_t address, uint8_t channel)
{
    int i;
    uint8_t index;
    char str[10];

    index = currentTableIndex = findEmptySlot();

    table[index].dstAdd = address;
    table[index].seqId  = (uint8_t)random32();
    table[index].ackCmd = 0x20; // Data Request Command is 0x20h
    table[index].attempts = 0;

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    putsUart0(str);

    if(ackFlagSet)
    {
        table[index].ackCmd = (table[index].ackCmd | 0x80); // Add Ack flag to (MSB) to the field
        table[index].retries = 4; // Set retries to 4 when ACK flag set
    }
    else
    {
        table[index].retries = 1; // Set retries to 1 when ACK flag NOT set
    }

    table[index].channel = channel;

    // Clear out data field with all zeros
    i = 0;
    while(i < DATA_MAX_SIZE)
    {
        table[index].data[i] = 0;
        i++;
    }

    table[index].size     = 0;
    table[index].checksum = 0;
    setPacketFrame(index);                 // Set values in packetFrame for checksum calculations
    table[index].checksum = getChecksum(); // Perform checksum calculations
    table[index].phase    = 0;             // Byte to transmit is DST_ADD
    table[index].backoff  = 1;             // Backoff set

    // When Checksum set
    TX_FLASH_TIMEOUT = 1000;
    setPinValue(RED_LED, TX_FLASH_LED = 1);

    table[index].validBit = true; //
}

// Function to send a Poll Request
void poll()
{
    int i;
    uint16_t index = 0;
    char str[50];

    index = currentTableIndex = findEmptySlot();

    table[index].dstAdd = 0xFF;
    table[index].seqId  = (uint8_t)random32();
    table[index].attempts = 0;

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    putsUart0(str);

    table[index].ackCmd = 0x78; // Send a Poll Request
    table[index].ackCmd = (table[index].ackCmd | 0x80); // Add Ack flag to (MSB) to the field
    table[index].retries = 1; // Set retries to 4 when ACK flag set
    table[index].channel = 0;

    // Clear out data field with all zeros
    i = 0;
    while(i < DATA_MAX_SIZE)
    {
        table[index].data[i] = 0;
        i++;
    }

    table[index].size     = 0;
    table[index].checksum = 0;
    setPacketFrame(index);                 // Set values in packetFrame for checksum calculations
    table[index].checksum = getChecksum(); // Perform checksum calculations
    table[index].phase    = 0;             // Byte to transmit is DST_ADD
    table[index].backoff  = 1;             // Backoff set to 500 milliseconds

    // When Checksum set
    TX_FLASH_TIMEOUT = 1000;
    setPinValue(RED_LED, TX_FLASH_LED = 1);

    table[index].validBit = true;
}

// Function to Set New Address of Node
void setNewAddress(uint8_t oldAddress, uint8_t newAddress)
{
    // UART1_9BITADDR_R
    int i;
    uint8_t index;
    char str[50];

    index = currentTableIndex = findEmptySlot();

    table[index].seqId = (uint8_t)random32(); // Set Sequence ID for Message

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    putsUart0(str);

    table[index].dstAdd = oldAddress;
    table[index].ackCmd = 0x7A; // Set Command is 0x00h
    table[index].attempts = 0;
    if(ackFlagSet)
    {
        table[index].ackCmd = (table[index].ackCmd | 0x80); // Add Ack flag to (MSB) to the field
        table[index].retries = 4;
    }
    else
    {
        table[index].retries = 1; // Set retries to 1 when ACK flag NOT set
    }

    table[index].channel = 0;

    // Clear out data field with all zeros
    i = 0;
    while(i < DATA_MAX_SIZE)
    {
        table[index].data[i] = 0;
        i++;
    }

    table[index].data[0] = newAddress;
    table[index].size     = 1;
    table[index].phase    = 0;             // Byte to transmit is DST_ADD
    table[index].backoff  = 1;             // Initial backoff value is zero to Tx ASAP
    table[index].checksum = 0;
    setPacketFrame(index);                 // Set values in packetFrame for checksum calculations
    table[index].checksum = getChecksum(); // Perform checksum calculations

    // When Checksum set
    TX_FLASH_TIMEOUT = 1000;
    setPinValue(RED_LED, TX_FLASH_LED = 1);

    table[index].validBit = true; // Ready to Tx data packet
}


// Check to see if Tx FIFO EMPTY.
// If UART Tx FIFO Empty write first
// byte of next message, "prime the pump".
void sendPacket(uint8_t index)
{
    uint16_t size, phase;

    size = table[index].size + 6; // Size of data packet
    phase = table[index].phase;   // Store current phase value

    if(phase == 0) // Tx DESTINATION ADDRESS
    {
        setPinValue(DRIVER_ENABLE, 1);    // Turn ON Driver Enable (DE) for RS-485 to Tx
        UART1_LCRH_R |= ~(UART_LCRH_EPS); // turn-off EPS before Tx dstAdd, sets parity bit = 1
        UART1_DR_R = table[index].dstAdd;
        table[index].phase ++;
    }
    else if(phase == 1) // Tx SOURCE_ADDRESS
    {
        UART1_LCRH_R |= UART_LCRH_EPS; // turn-on EPS before Tx all bytes except Destination Address
        UART1_DR_R = SOURCE_ADDRESS;
        table[index].phase ++;
    }
    else if(phase == 2) // Tx SEQUENCE ID
    {
        UART1_LCRH_R |= UART_LCRH_EPS; // turn-on EPS before Tx all bytes except Destination Address
        UART1_DR_R = table[index].seqId;
        table[index].phase ++;
    }
    else if(phase == 3) // Tx ACK & COMMAND
    {
        UART1_LCRH_R |= UART_LCRH_EPS; // turn-on EPS before Tx all bytes except Destination Address
        UART1_DR_R = table[index].ackCmd;
        table[index].phase ++;
    }
    else if(phase == 4) // Tx CHANNEL
    {
        UART1_LCRH_R |= UART_LCRH_EPS; // turn-on EPS before Tx all bytes except Destination Address
        UART1_DR_R = table[index].channel;
        table[index].phase ++;
    }
    else if(phase == 5) // Tx SIZE
    {
        UART1_LCRH_R |= UART_LCRH_EPS; // turn-on EPS before Tx all bytes except Destination Address
        UART1_DR_R = table[index].size;
        table[index].phase ++;
    }
    else if(phase > 5 && phase < size) // Tx DATA[]
    {
        UART1_LCRH_R |= UART_LCRH_EPS; // turn-on EPS before Tx all bytes except Destination Address
        UART1_DR_R = (table[index].dstAdd + phase);
        table[index].phase ++;
    }
    else if(phase == size) // Tx CHECKSUM
    {
        putcUart1(table[index].checksum);
        table[index].phase = 0; // Set phase = 0 after Tx checksum
        table[index].retries--; // Decrement Tx retries remaining
        table[index].backoff = calcNewBackoff(table[index].attempts);

        // If ACK Requested and MAX_ReTransmissions sent w/out ACK send error msg to PC
        if((table[index].retries == 0) && (table[index].ackCmd & 0x80)) //
        {
            char str[50];
            sprintf(str, "  Error Sending Msg %u\r\n", table[index].seqId);
            putsUart0(str);
        }

        if(table[index].retries == 0) // Delete message from Tx Queue
        {
            table[index].validBit = false; // Set valid bit to false as finished transmitting the packet
        }

        while(UART1_FR_R & UART_FR_BUSY);  // Wait until UART is not busy before proceeding

        setPinValue(DRIVER_ENABLE, 0);     // Turn OFF Driver Enable (DE) on RS-485
    }p
}

// Function finds 1st slot with validBit field set to false and returns the index value to it.
uint8_t findEmptySlot()
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

// Fill out Packet Frame for Transmitting
void setPacketFrame(uint8_t index)
{
    int i;

    txInfo.dstAdd  = table[index].dstAdd;
    txInfo.srcAdd  = SOURCE_ADDRESS;
    txInfo.seqId   = table[index].seqId;
    txInfo.ackCmd  = table[index].ackCmd;
    txInfo.channel = table[index].channel;
    txInfo.size    = table[index].size;

    for(i = 0; i < DATA_MAX_SIZE; i++)
    {
        txInfo.data[i] = 0;
    }

    for(i = 0; i < info.size; i++)
    {
        txInfo.data[i] = table[index].data[i];
    }

    txInfo.checksum = table[index].checksum;
}

// Calculate sum of words
// Must use getChecksum to complete 1's compliment addition
void sumWords(void* data, uint16_t sizeInBytes)
{
    uint8_t* pData = (uint8_t*)data;
    uint16_t i;
    for (i = 0; i < sizeInBytes; i++)
    {
        sum += *pData++;
    }
}
// Completes 1's compliment addition by folding carries back into field
uint8_t getChecksum()
{
    uint16_t size;
    uint8_t tmp8;
    sum = 0;
    size = txInfo.size;

    sumWords(&txInfo.dstAdd, 6);
    sumWords(&txInfo.data, size);

    tmp8 = ~(sum);

    return tmp8;
}

void takeAction()
{
    uint8_t command, channel, value;

    command = (rxInfo.ackCmd & 0x7F); // Mask Bits to remove ACK if set

    switch(command)
    {
        // Set Command
        case 0x00:
            putsUart0("  Set\r\n");
            channel = rxInfo.channel;
            value = rxInfo.data[0]; // DATA argument for set should be size 1
            break;
        // Piecewise Command
        case 0x01:
            putsUart0("  Piecewise\r\n");
            break;
        // Pulse Command
        case 0x02:
            putsUart0("  Pulse\r\n");
            break;
        // Square Command
        case 0x03:
            putsUart0("  Square\r\n");
            break;
        // Sawtooth Command
        case 0x04:
            putsUart0("  Sawtooth\r\n");
            break;
        // Triangle Command
        case 0x05:
            putsUart0("  Triangle\r\n");
            break;
        // Pulse Command
        case 0x20:
            putsUart0("  Pulse\r\n");
            break;
        // Data Request
        case 0x21:
            putsUart0("  Data Request\r\n");
            break;
        // Report Control
        case 0x22:
            putsUart0("  Report Control\r\n");
            break;
        // LCD Display Text
        case 0x40:
            putsUart0("  LCD Display Text\r\n");
            break;
        // RGB Command
        case 0x48:
            putsUart0("  RGB\r\n");
            break;
        // RGB Piecewise Command
        case 0x49:
            putsUart0("  RGB Piecewise\r\n");
            break;
        // UART Data
        case 0x50:
            putsUart0("  UART Data\r\n");
            break;
        // UART Control
        case 0x51:
            putsUart0("  UART Control\r\n");
            break;
        // I2C Command
        case 0x54:
            putsUart0("  I2C Command\r\n");
            break;
        // ACK Command
        case 0x70:
            putsUart0("  Acknowledge\r\n");
            break;
        // Poll Request
        case 0x78:
            putsUart0("  Poll Request\r\n");
            break;
        // Poll Response
        case 0x79:
            putsUart0("  Poll Response\r\n");
            break;
        // Set Address
        case 0x7A:
            putsUart0("  Set Address\r\n");
            break;
        // Node Control
        case 0x7D:
            putsUart0("  Node Control\r\n");
            break;
        // Bootload
        case 0x7E:
            putsUart0("  Bootload\r\n");
            break;
        // Reset
        case 0x7F:
            putsUart0("  Reset\r\n");
            break;
        default:
            putsUart0("  Command Not Recognized\r\n");
            break;
    }
}

// Function to Display to Terminal Contents of the Pending Table
void displayTableContents()
{
    int i, j;
    char str[10];

    for (i = 0; i < 10; i++)
    {
        putsUart0("[");
        sprintf(str, "[%02u] ", table[i].validBit);
        putsUart0(str);
        sprintf(str, "[%02u] ", table[i].seqId);
        putsUart0(str);
        sprintf(str, "[%02u] ", table[i].dstAdd);
        putsUart0(str);
        sprintf(str, "[%02u] ", table[i].seqId);
        putsUart0(str);
        sprintf(str, "[%02u] ", table[i].ackCmd);
        putsUart0(str);
        sprintf(str, "[%02u] ", table[i].channel);
        putsUart0(str);
        sprintf(str, "[%02u] ", table[i].size);
        putsUart0(str);
        sprintf(str, "[%02u] ", table[i].ackCmd);
        putsUart0(str);
        putsUart0("[");
        for(j = 0; j < DATA_MAX_SIZE; j++)
        {
            sprintf(str, "[%02u] ", table[i].data[j]);
            putsUart0(str);
        }
        putsUart0("] ");
        sprintf(str, "[%02u] ", table[i].checksum);
        putsUart0(str);
        sprintf(str, "[%02u]", table[i].retries);
        putsUart0(str);
        putsUart0("]\r\n");
    }
}
