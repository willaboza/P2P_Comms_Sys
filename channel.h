/*
 * channel.h
 *
 *  Created on: Apr 29, 2020
 *      Author: willi
 */

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

typedef struct _activityFrame // 7 bytes + 256 bytes = 263 bytes
{
    uint8_t   dstAdd;
    uint8_t   srcAdd;
    uint8_t   seqId;
    uint8_t   ackCmd;
    uint8_t   channel;
    uint8_t   size;
    uint8_t   checksum;
} activityFrame;

#endif /* CHANNEL_H_ */
