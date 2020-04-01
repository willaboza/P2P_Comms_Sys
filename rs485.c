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
uint8_t sum               = 0;

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
    char str[10];

    index = currentTableIndex = findEmptySlot();

    table[index].seqId  = info.seqId = (uint8_t)random32(); // Set Sequence ID for Message

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    putsUart0(str);

    table[index].dstAdd = info.dstAdd = address;
    table[index].ackCmd = info.ackCmd = 0x00; // Set Command is 0x00h
    table[index].attempts = 0;
    if(ackFlagSet)
    {
        table[index].ackCmd = info.ackCmd = (table[index].ackCmd | 0x80); // Add Ack flag to (MSB) to the field
        table[index].retries = 4;
    }
    else
    {
        table[index].retries = 1; // Set retries to 1 when ACK flag NOT set
    }

    table[index].channel = info.channel = channel;

    // Clear out data field with all zeros
    i = 0;
    while(i < DATA_MAX_SIZE)
    {
        table[index].data[i] = info.data[i] = 0;
        i++;
    }
    // Add user data
    i = 0;
    while(c[i] != '\0')
    {
        table[index].data[i] = info.data[i] = c[i];
        i++;
    }

    table[index].size     = info.size = i;
    table[index].phase    = 0;             // Byte to transmit is DST_ADD
    table[index].backoff  = 1;             // Initial backoff value is zero to Tx ASAP
    table[index].checksum = info.checksum = 0;
    table[index].checksum = getChecksum(&info.dstAdd);

    // When Checksum set
    setPinValue(RED_LED, TX_FLASH_LED = 1);
    TX_FLASH_TIMEOUT = 1000;

    table[index].validBit = true; // Ready to Tx data packet

    startPeriodicTimer(backoffTimer, table[index].backoff);
}

// Function to send a Data Request to a specified Address and Channel
void getAC(uint8_t address, uint8_t channel)
{
    int i;
    uint8_t index;
    char str[10];

    index = currentTableIndex = findEmptySlot();

    table[index].dstAdd = info.dstAdd = address;
    table[index].seqId  = info.seqId = (uint8_t)random32();
    table[index].ackCmd = info.ackCmd = 0x20; // Data Request Command is 0x20h
    table[index].attempts = 0;
    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    putsUart0(str);

    if(ackFlagSet)
    {
        table[index].ackCmd = info.ackCmd = (table[index].ackCmd | 0x80); // Add Ack flag to (MSB) to the field
        table[index].retries = 4; // Set retries to 4 when ACK flag set
    }
    else
    {
        table[index].retries = 1; // Set retries to 1 when ACK flag NOT set
    }

    table[index].channel = info.channel = channel;

    // Clear out data field with all zeros
    i = 0;
    while(i < DATA_MAX_SIZE)
    {
        table[index].data[i] = info.data[i] = 0;
        i++;
    }

    table[index].size     = info.size = 0;
    table[index].checksum = info.checksum = 0;
    table[index].checksum = info.checksum = getChecksum(&info.dstAdd);

    table[index].phase    = 0;    // Byte to transmit is DST_ADD
    table[index].backoff  = 1;  // Backoff set to 500 milliseconds

    // When Checksum set
    setPinValue(RED_LED, TX_FLASH_LED = 1);
    TX_FLASH_TIMEOUT = 1000;

    table[index].validBit = true; //

    startPeriodicTimer(backoffTimer, table[index].backoff);
}

// Function to send a Poll Request
void poll()
{
    int i;
    uint16_t index = 0;
    char str[10];

    index = currentTableIndex = findEmptySlot();

    table[index].dstAdd = info.dstAdd = 0xFF;
    table[index].seqId  = info.seqId = (uint8_t)random32();
    table[index].attempts = 0;
    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    putsUart0(str);

    table[index].ackCmd = info.ackCmd = 0x78; // Send a Poll Request
    table[index].ackCmd = info.ackCmd = (table[index].ackCmd | 0x80); // Add Ack flag to (MSB) to the field
    table[index].retries = 1; // Set retries to 4 when ACK flag set
    table[index].channel = info.channel = 0;

    // Clear out data field with all zeros
    i = 0;
    while(i < DATA_MAX_SIZE)
    {
        table[index].data[i] = info.data[i] = 0;
        i++;
    }

    table[index].size     = info.size = 0;
    table[index].checksum = info.checksum = 0;
    table[index].checksum = getChecksum();
    info.checksum = table[index].checksum;
    table[index].phase    = 0;    // Byte to transmit is DST_ADD
    table[index].backoff  = 1;  // Backoff set to 500 milliseconds

    // When Checksum set
    setPinValue(RED_LED, TX_FLASH_LED = 1);
    TX_FLASH_TIMEOUT = 1000;

    table[index].validBit = true;

    startPeriodicTimer(backoffTimer, table[index].backoff);
}

// Function to Set New Address of Node
void setNewAddress(uint8_t oldAddress, uint8_t newAddress)
{
    // UART1_9BITADDR_R
    int i;
    uint8_t index;
    uint32_t tmp32;
    char str[10];

    index = currentTableIndex = findEmptySlot();

    table[index].seqId  = info.seqId = (uint8_t)random32(); // Set Sequence ID for Message

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    putsUart0(str);

    table[index].dstAdd = info.dstAdd = oldAddress;
    table[index].ackCmd = info.ackCmd = 0x7A; // Set Command is 0x00h
    table[index].attempts = 0;
    if(ackFlagSet)
    {
        table[index].ackCmd = info.ackCmd = (table[index].ackCmd | 0x80); // Add Ack flag to (MSB) to the field
        table[index].retries = 4;
    }
    else
    {
        table[index].retries = 1; // Set retries to 1 when ACK flag NOT set
    }

    table[index].channel = info.channel = 0;

    // Clear out data field with all zeros
    i = 0;
    while(i < DATA_MAX_SIZE)
    {
        table[index].data[i] = info.data[i] = 0;
        i++;
    }
    // Add user data

    table[index].data[0] = info.data[0] = newAddress;

    table[index].size     = info.size = 1;
    table[index].phase    = 0;             // Byte to transmit is DST_ADD
    table[index].backoff  = 1;             // Initial backoff value is zero to Tx ASAP
    table[index].checksum = info.checksum = 0;
    table[index].checksum = getChecksum(&info.dstAdd);

    // When Checksum set
    setPinValue(RED_LED, TX_FLASH_LED = 1);
    TX_FLASH_TIMEOUT = 1000;

    table[index].validBit = true; // Ready to Tx data packet
    tmp32 = table[index].backoff;
    startPeriodicTimer(backoffTimer, tmp32);
}


// Check to see if Tx FIFO EMPTY.
// If UART Tx FIFO Empty write first
// byte of next message, "prime the pump".
void sendPacket(uint8_t index)
{
    uint8_t tmp8;
    uint16_t size, phase;

    size = table[index].size + 6; // Size of data packet
    phase = table[index].phase;   // Store current phase value

    if(phase == 0) // Tx Destination Address
    {
        setPinValue(DRIVER_ENABLE, 1);  // Set value to 1 before Tx packet
        UART1_LCRH_R &= ~UART_LCRH_EPS; // turn-off EPS before Tx Destination Address byte to enable parity bit as 1
        tmp8 = table[index].dstAdd;
        putcUart1(tmp8);
        table[index].phase ++;
    }
    else if(phase == 1) // Tx Source Address
    {
        UART1_LCRH_R |= UART_LCRH_EPS; // turn-on EPS before Tx all bytes except Destination Address
        putcUart1(SOURCE_ADDRESS);
        table[index].phase ++;
    }
    else if(phase < size) // Tx rest of packet besides checksum and addresses
    {
        putcUart1(table[index].dstAdd + phase);
        table[index].phase ++;
    }
    else if(phase == size) // Tx checksum
    {
        putcUart1(table[index].checksum);
        table[index].phase = 0; // Set phase = 0 after Tx checksum
        table[index].retries--; // Decrement Tx retries remaining
        table[index].backoff = calcNewBackoff(table[index].attempts);

        if((table[index].retries == 0) && (table[index].ackCmd & 0x80))
        {
            // Insert code here to send message to user of failure
        }

        if(table[index].retries == 0) // Delete message from Tx Queue
        {
            resetTimer(index);
            table[index].validBit = false; // Set valid bit to false as finished transmitting the packet
        }

        while(UART1_FR_R & UART_FR_BUSY); // Wait until UART is not busy before proceeding

        setPinValue(DRIVER_ENABLE, 0); // Turn OFF Driver Enable
    }
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

    info.dstAdd  = table[index].dstAdd;
    info.srcAdd  = SOURCE_ADDRESS;
    info.seqId   = table[index].seqId;
    info.ackCmd  = table[index].ackCmd;
    info.channel = table[index].channel;
    info.size    = table[index].size;
    for(i = 0; i < DATA_MAX_SIZE; i++)
    {
        info.data[i] = table[index].data[i];
    }
    info.checksum = table[index].checksum;
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
    size = info.size;

    sumWords(&info.dstAdd, 6);
    sumWords(&info.data, size);

    tmp8 = ~(sum);

    return tmp8;
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
