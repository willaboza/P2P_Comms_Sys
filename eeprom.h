// eeprom.h
// William Bozarth
// Created on: April 7, 2020

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef EEPROM_H_
#define EEPROM_H_

#include "eeprom.h"

void initEeprom(void);
void writeEeprom(uint16_t add, uint32_t data);
uint32_t readEeprom(uint16_t add);
void readEepromAddress(void);


#endif /* EEPROM_H_ */
