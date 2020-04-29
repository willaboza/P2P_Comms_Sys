/*
 * led.h
 *
 *  Created on: Apr 29, 2020
 *      Author: willi
 */

#ifndef LED_H_
#define LED_H_

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

#define MAX_PWM 1023

#define PWM0_RED_LED PORTB,5
#define PWM0_BLUE_LED PORTE,4
#define PWM0_GREEN_LED PORTE,5

void initPwm0();
void setRgbColor(uint16_t red, uint16_t green, uint16_t blue);

#endif /* LED_H_ */
