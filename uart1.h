/*
 * uart1.h
 *
 *  Created on: Mar 27, 2020
 *      Author: willi
 */

#ifndef UART1_H_
#define UART1_H_

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "uart0.h"
#include "rs485.h"

// Pins
#define DRIVER_ENABLE PORTB,2
#define UART1_TX PORTB,1
#define UART1_RX PORTB,0

extern uint16_t rxPhase;

void initUart1();
void setUart1BaudRate(uint32_t baudRate, uint32_t fcyc);
void uart1Isr();

#endif /* UART1_H_ */
