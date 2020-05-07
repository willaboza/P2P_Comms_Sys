/*
 * led.h
 *
 *  Created on: Apr 29, 2020
 *      Author: willi
 */

#ifndef PWM0_H_
#define PWM0_H_

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "uart0.h"

#define MAX_PWM 1023

#define PWM0_RED_LED PORTB,5
#define PWM0_BLUE_LED PORTE,4
#define PWM0_GREEN_LED PORTE,5

extern uint8_t redLedValue;
extern uint8_t greenLedValue;
extern uint8_t blueLedValue;

void initPwm0();
void setRgbColor(uint16_t red, uint16_t green, uint16_t blue);
void setRedLed(uint16_t red);
void setGreenLed(uint16_t green);
void setBlueLed(uint16_t blue);

#endif /* PWM0_H_ */
