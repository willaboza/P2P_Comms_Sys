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
#include <stdbool.h>
#include <uart0.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "gpio.h"
#include "rs485.h"

#define NUM_TIMERS 10
#define BACKOFF_BASE_NUMBER 250

typedef void(*_callback)(void);
extern _callback fn[NUM_TIMERS];
extern uint32_t period[NUM_TIMERS];
extern uint32_t ticks[NUM_TIMERS];
extern bool reload[NUM_TIMERS];
extern bool TX_FLASH_LED;
extern uint32_t TX_FLASH_TIMEOUT;

void initTimer();
bool startOneShotTimer(_callback callback, uint32_t seconds);
bool startPeriodicTimer(_callback callback, uint32_t seconds);
bool stopTimer(_callback callback);
bool restartTimer(_callback callback);
void resetAllTimers();
void resetTimer(uint8_t index);
void tickIsr();
void backoffTimer();
uint32_t calcNewBackoff(uint8_t exponent);
uint32_t calcPower(uint8_t exponent);
uint32_t random32();

#endif /* TIMERS_H_ */
