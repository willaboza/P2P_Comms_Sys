/*
 * eeprom.h
 *
 *  Created on: Apr 7, 2020
 *      Author: William Bozarth
 */

#ifndef EEPROM_H_
#define EEPROM_H_


#include <stdint.h>
#include "tm4c123gh6pm.h"

void initEeprom();
void writeEeprom(uint16_t add, uint32_t data);
uint32_t readEeprom(uint16_t add);


#endif /* EEPROM_H_ */
