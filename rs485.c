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
uint8_t seqId              = 0;
uint8_t messageInProgress  = 0;
uint8_t sum                = 0;

packetFrame txInfo = {0};
packetFrame rxInfo = {0};
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

// Function to get Next Sequence ID
void getNextSeqID()
{
    seqId = (seqId + 1) % 256;
}

// When received from PC; where A is the address, C is the Channel, and V is
// the value, a set command shall be sent
void setACV(uint8_t address, uint8_t channel, char c[])
{
    int i;
    uint8_t index;
    char str[50];

    index = messageInProgress = findEmptySlot();

    table[index].dstAdd = address;
    table[index].ackCmd = 0x00; // Set Command is 0x00h
    table[index].attempts = 0;

    // Set Sequence ID for Message
    table[index].seqId = (uint8_t)random32(); // Set Sequence ID for Message

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    sendUart0String(str);

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

    index = messageInProgress = findEmptySlot();

    // Set Sequence ID for Message
    table[index].seqId = (uint8_t)random32();

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    sendUart0String(str);

    table[index].dstAdd = address;
    table[index].ackCmd = 0x20; // Data Request Command is 0x20h
    table[index].attempts = 0;
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

    index = messageInProgress = findEmptySlot();

    // Set Sequence ID for Message
    table[index].seqId = (uint8_t)random32();

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    sendUart0String(str);

    table[index].dstAdd = 0xFF;
    table[index].attempts = 0;
    table[index].ackCmd = 0x78; // Send a Poll Request
    table[index].retries = 1;   // Set retries to 4 when ACK flag set
    table[index].channel = 0;

    // Clear out data field with all zeros
    /*
    i = 0;
    while(i < DATA_MAX_SIZE)
    {
        table[index].data[i] = 0;
        i++;
    }
    */
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
    int i;
    uint8_t index;
    char str[50];

    index = messageInProgress = findEmptySlot();

    // Set Sequence ID for Message
    table[index].seqId = (uint8_t)random32();

    sprintf(str, "  Queuing Msg %u\r\n", table[index].seqId);
    sendUart0String(str);

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

void sendDataRequest(uint8_t address, uint8_t channel, uint8_t value)
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

    table[index].data[0]  = value;
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

// Send Request to Node to Reset Device
void sendReset(uint8_t address)
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
    table[index].ackCmd = 0x7F; // Data Report is 0x21h
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

    table[index].data[0]  = 0;
    table[index].size     = 0;
    table[index].phase    = 0;             // Byte to transmit is DST_ADD
    table[index].backoff  = 1;             // Initial backoff value is zero to Tx ASAP
    table[index].checksum = 0;
    setPacketFrame(index);                 // Set values in packetFrame for checksum calculations
    table[index].checksum = getChecksum(); // Perform checksum calculations

    // When Checksum set flash RED_LED
    TX_FLASH_TIMEOUT = 1000;
    setPinValue(RED_LED, TX_FLASH_LED = 1);

    table[index].validBit = true; // Ready to Tx data packet
}

void sendAcknowledge(uint8_t address, uint8_t id)
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
    table[index].ackCmd = 0x70; // Data Report is 0x21h
    table[index].attempts = 0;
    table[index].retries = 1;   // Set retries to 1 when ACK flag NOT set
    table[index].channel = 0;

    // Clear out data field with all zeros
    i = 0;
    while(i < DATA_MAX_SIZE)
    {
        table[index].data[i] = 0;
        i++;
    }

    table[index].data[0]  = id;
    table[index].size     = 1;
    table[index].phase    = 0;             // Byte to transmit is DST_ADD
    table[index].backoff  = 1;             // Initial backoff value is zero to Tx ASAP
    table[index].checksum = 0;
    setPacketFrame(index);                 // Set values in packetFrame for checksum calculations
    table[index].checksum = getChecksum(); // Perform checksum calculations

    // When Checksum set flash RED_LED
    TX_FLASH_TIMEOUT = 1000;
    setPinValue(RED_LED, TX_FLASH_LED = 1);

    table[index].validBit = true; // Ready to Tx data packet
}

// Function to Send Currently Assigned Address on Poll Request
void sendPollResponse(uint8_t address)
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
    table[index].ackCmd = 0x79;  // Poll Response is 0x79h
    table[index].attempts = 0;
    table[index].retries  = 1;   // Set retries to 1 when ACK flag NOT set
    table[index].channel  = 0;

    // Clear out data field with all zeros
    i = 0;
    while(i < DATA_MAX_SIZE)
    {
        table[index].data[i] = 0;
        i++;
    }

    table[index].data[0]  = SOURCE_ADDRESS;
    table[index].size     = 1;
    table[index].phase    = 0;             // Byte to transmit is DST_ADD
    table[index].backoff  = 1;             // Initial backoff value is zero to Tx ASAP
    table[index].checksum = 0;
    setPacketFrame(index);                 // Set values in packetFrame for checksum calculations
    table[index].checksum = getChecksum(); // Perform checksum calculations

    // When Checksum set flash RED_LED
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
        UART1_LCRH_R &= ~(UART_LCRH_EPS); // Turn OFF EPS before Tx dstAdd, sets parity bit = 1
        UART1_DR_R = table[index].dstAdd; // Transmit destination address
        UART1_LCRH_R |= UART_LCRH_EPS;    // Turn ON EPS before Tx all bytes except Destination Address
        table[index].phase ++;
    }
    else if(phase == 1) // Tx SOURCE_ADDRESS
    {
        UART1_DR_R = SOURCE_ADDRESS;
        table[index].phase ++;
    }
    else if(phase == 2) // Tx SEQUENCE ID
    {
        UART1_DR_R = table[index].seqId;
        table[index].phase ++;
    }
    else if(phase == 3) // Tx ACK & COMMAND
    {
        UART1_DR_R = table[index].ackCmd;
        table[index].phase ++;
    }
    else if(phase == 4) // Tx CHANNEL
    {
        UART1_DR_R = table[index].channel;
        table[index].phase ++;
    }
    else if(phase == 5) // Tx SIZE
    {
        UART1_DR_R = table[index].size;
        table[index].phase ++;
    }
    else if(phase > 5 && phase < size) // Tx DATA[]
    {
        UART1_DR_R = (table[index].dstAdd + phase);
        table[index].phase ++;
    }
    else if(phase == size) // Tx CHECKSUM
    {
        UART1_DR_R = table[index].checksum;
        table[index].phase = 0;             // Set phase = 0 after Tx checksum
        table[index].retries--;             // Decrement Tx retries remaining
        table[index].backoff = calcNewBackoff(table[index].attempts);

        // If ACK Requested and MAX_ReTransmissions sent w/out ACK send error msg to PC
        if((table[index].retries == 0) && (table[index].ackCmd & 0x80) && table[index].validBit)
        {
            char str[50];
            sprintf(str, "  Error Sending Msg %u\r\n", table[index].seqId);
            sendUart0String(str);
        }

        if(table[index].retries == 0)      // Delete message from Tx Queue
        {
            table[index].validBit = false; // Set valid bit to false as finished transmitting the packet
        }

        while(UART1_FR_R & UART_FR_BUSY);  // Wait until UART is not busy before proceeding

        UART1_LCRH_R &= ~(UART_LCRH_EPS);  // turn-off EPS before Tx dstAdd, sets parity bit = 1

        setPinValue(DRIVER_ENABLE, 0);     // Turn OFF Driver Enable (DE) on RS-485
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

    txInfo.dstAdd  = table[index].dstAdd;
    txInfo.srcAdd  = SOURCE_ADDRESS;
    txInfo.seqId   = table[index].seqId;
    txInfo.ackCmd  = table[index].ackCmd;
    txInfo.channel = table[index].channel;
    txInfo.size    = table[index].size;

    for(i = 0; i < txInfo.size; i++)
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
    uint8_t tmp8;
    sum = 0;

    sumWords(&txInfo.dstAdd, 6);
    sumWords(&txInfo.data, txInfo.size);

    tmp8 = ~(sum);

    return tmp8;
}

// Get Current Channel Value
uint8_t getChannelValue()
{
    uint8_t tmp8 = 0;

    return tmp8;
}

// If Acknowledge Rx'd and SeqID matches then set valid bit for message to 0
void ackReceived()
{
    uint8_t i;
    bool ok = true;
    i = 0;
    while((i < MAX_TABLE_SIZE) && ok)
    {
        if(table[i].seqId == rxInfo.seqId)
        {
            table[i].validBit = 0;
            ok = false;
        }
        i++;
    }
}

void takeAction()
{
    uint8_t command, channel, value;
    char str[DATA_MAX_SIZE];

    command = (rxInfo.ackCmd & 0x7F); // Mask Bits to remove ACK if set

    switch(command)
    {
        // Set Command
        case 0x00:
            sendUart0String("  Set\r\n");
            if(ackFlagSet)
            {
                sendAcknowledge(rxInfo.srcAdd, rxInfo.seqId);
            }
            channel = rxInfo.channel;
            value   = rxInfo.data[0]; // DATA argument for set should be size 1
            break;

        // Pulse Command
        case 0x20:
            sendUart0String("  Data Request\r\n");
            if(ackFlagSet)
            {
                sendAcknowledge(rxInfo.srcAdd, rxInfo.seqId);
            }
            sendDataRequest(rxInfo.srcAdd, rxInfo.channel, getChannelValue());
            break;

        // Data Request
        case 0x21:
            sprintf(str, "  Status = %02u for Channel %02u at Add = %02u\r\n", rxInfo.data[0], rxInfo.channel, rxInfo.dstAdd);
            sendUart0String(str);
            break;

        // ACK Command
        case 0x70:
            sendUart0String("  Acknowledge\r\n");
            ackReceived(); // Check for seqId match
            break;

        // Poll Request
        case 0x78:
            sendUart0String("  Rx Poll Request\r\n");
            sendPollResponse(rxInfo.srcAdd);
            break;

        // Poll Response
        case 0x79:
            sprintf(str, "  Address %02u Responded\r\n", rxInfo.srcAdd);
            sendUart0String(str);
            break;

        // Set Address
        case 0x7A:
            if(ackFlagSet)
            {
                sendAcknowledge(rxInfo.srcAdd, rxInfo.seqId);
            }
            SOURCE_ADDRESS = rxInfo.data[0];
            writeEeprom(0x0000, SOURCE_ADDRESS); // Store New Address in EEPROM
            srand(SOURCE_ADDRESS);
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
