/*
 * rs485.h
 *
 *  Created on: Mar 26, 2020
 *      Author: willi
 */

#ifndef RS485_H_
#define RS485_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <uart0.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "uart0.h"
#include "uart1.h"
#include "timers.h"
#include "eeprom.h"
#include "reboot.h"

// Pins
#define RED_LED PORTF,     1
#define BLUE_LED PORTF,    2
#define GREEN_LED PORTF,   3
#define PUSH_BUTTON PORTF, 4

#define MAX_PACKET_SIZE 263
#define MAX_TABLE_SIZE  10
#define DATA_MAX_SIZE   50

typedef struct _packetFrame // 7 bytes + 256 bytes = 263 bytes
{
    uint8_t   dstAdd;
    uint8_t   srcAdd;
    uint8_t   seqId;
    uint8_t   ackCmd;
    uint8_t   channel;
    uint8_t   size;
    uint8_t   data[DATA_MAX_SIZE];
    uint8_t   checksum;
} packetFrame;

typedef struct _pendingTable
{
    bool     validBit;
    uint16_t phase;
    uint8_t  retries;
    uint8_t  attempts;
    uint32_t backoff;
    uint8_t  dstAdd;
    uint8_t  seqId;
    uint8_t  ackCmd;
    uint8_t  channel;
    uint8_t  size;
    uint8_t  data[DATA_MAX_SIZE];
    uint8_t  checksum;
} pendingTable;

extern packetFrame txInfo;
extern packetFrame rxInfo;
extern pendingTable table[MAX_TABLE_SIZE];
extern bool carrierSenseFlag;
extern bool ackFlagSet;
extern bool randomFlag;
extern uint8_t SOURCE_ADDRESS;
extern uint8_t BACKOFF_ATTEMPTS;
extern uint8_t seqId;
extern uint8_t messageInProgress;
extern uint8_t sum;

bool packetIsUnicast(uint8_t packet[]);
bool ackIsRequired(uint8_t packet[]);
void getNextSeqID();
void setACV(uint8_t address, uint8_t channel, char c[]);
void getAC(uint8_t address, uint8_t channel);
void poll();
void setNewAddress(uint8_t oldAddress, uint8_t newAddress);
void sendDataRequest(uint8_t address, uint8_t channel, uint8_t value);
void sendReset(uint8_t address);
void sendAcknowledge(uint8_t address, uint8_t id);
void sendPollResponse(uint8_t address);
void sumWords(void* data, uint16_t sizeInBytes);
void sendPacket(uint8_t index);
void displayTableContents();
void setPacketFrame(uint8_t index);
uint8_t getChecksum();
uint8_t findEmptySlot();
void takeAction();

#endif /* RS485_H_ */
