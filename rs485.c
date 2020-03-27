/*
 * rs485.c
 *
 *  Created on: Mar 26, 2020
 *      Author: willi
 */

#include "rs485.h"

bool carrierSenseFlag = false;

// Determines if packet received is unicast
bool packetIsUnicast(uint8_t packet[])
{
    packetFrame* info = (packetFrame*)packet;
    bool ok = false;
    if(!(info->dstAdd & 0xFF))
        ok = true;
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
        trueSize = maxSize
    }
    else
    {
        trueSize = info->size
    }

    for(i = 0; i < trueSize; i++)
    {
        info->data[i] = getcUart1();
    }

    info->data[i] = '\0'; // add NULL terminating character to end of data

    info->checksum = getcUart1();
}
