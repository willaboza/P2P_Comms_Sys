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
#include "tm4c123gh6pm.h"
#include "table.h"
#include "uart.h"

#define MAX_PACKET_SIZE 263

extern bool carrierSenseFlag;
bool packetIsUnicast(uint8_t packet[]);
bool ackIsRequired(uint8_t packet[]);
void getPacket(uint8_t packet[], uint16_t maxSize);

#endif /* RS485_H_ */
