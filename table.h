/*
 * table.h
 *
 *  Created on: Mar 26, 2020
 *      Author: willi
 */

#ifndef TABLE_H_
#define TABLE_H_

#define MAX_SIZE 255

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "tm4c123gh6pm.h"
#include "uart.h"

typedef struct _packetFrame
{
    uint8_t   dstAdd;
    uint8_t   srcAdd;
    uint8_t   seqId;
    uint8_t   ackCmd;
    uint8_t   chan;
    uint8_t   size;
    uint8_t   data[MAX_SIZE];
    uint8_t   checksum;
} packetFrame;

typedef struct _pendingTable // 9 bytes + 255 bytes = 264 bytes
{
    bool      validBit[10];
    uint8_t   seqNum[10];
    uint8_t   dstAdd[10];
    uint8_t   seqId[10];
    uint8_t   ackCmd[10];
    uint8_t   chan[10];
    uint8_t   size[10];
    uint8_t   data[10][MAX_SIZE];
    uint8_t   checksum[10];
    uint8_t   retries[10];
} pendingTable;

extern bool ackFlagSet;

void setACV(uint8_t address, uint8_t channel, uint8_t value);
void getAC(uint8_t address, uint8_t channel);
void displayTableContents();

#endif /* TABLE_H_ */
