/*
 * timers.h
 *
 *  Created on: Feb 20, 2020
 *      Author: William Bozarth
 */

#ifndef TIMERS_H_
#define TIMERS_H_

//List of Libraries to Include
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "gpio.h"
#include "rs485.h"

#define NUM_TIMERS 10
#define NUM_CHANNELS 5
#define BACKOFF_BASE_NUMBER 500

extern uint32_t TX_FLASH_TIMEOUT;
extern uint32_t RX_FLASH_TIMEOUT;

typedef void(*_callback)(void);
extern _callback fn[NUM_CHANNELS];
extern uint32_t period[NUM_CHANNELS];
extern uint32_t ticks[NUM_CHANNELS];
extern bool reload[NUM_CHANNELS];
extern bool invert[NUM_CHANNELS];

void initTimer();
void tickIsr();
uint32_t calcNewBackoff(uint8_t exponent);
uint32_t calcPower(uint8_t exponent);
uint32_t random32();
bool startOneShotTimer(_callback callback, uint32_t seconds);
bool startPeriodicTimer(_callback callback, uint32_t seconds);
bool stopTimer(_callback callback);
void redLedPulse();
void blueLedPulse();
void greenLedPulse();

#endif /* TIMERS_H_ */
