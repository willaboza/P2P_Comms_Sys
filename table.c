/*
 * table.c
 *
 *  Created on: Mar 26, 2020
 *      Author: willi
 */

#include "table.h"

#packetFrame info = {0};
pendingTable table = {0};
bool ackFlagSet = false;

// When received from PC; where A is the address, C is the Channel, and V is
// the value, a set command shall be sent
void setACV(uint8_t address, uint8_t channel, uint8_t value)
{



}

//
void getAC(uint8_t address, uint8_t channel)
{

}

// Function to Display to Terminal Contents of the Pending Table
void displayTableContents()
{
    int i, j;
    char str[10];

    for (i = 0; i < 10; i++)
    {
        putsUart0("[");
        sprintf(str, "[%02u] ", table.validBit[i]);
        putsUart0(str);
        sprintf(str, "[%02u] ", table.seqNum[i]);
        putsUart0(str);
        sprintf(str, "[%02u] ", table.dstAdd[i]);
        putsUart0(str);
        sprintf(str, "[%02u] ", table.seqId[i]);
        putsUart0(str);
        sprintf(str, "[%02u] ", table.ackCmd[i]);
        putsUart0(str);
        sprintf(str, "[%02u] ", table.chan[i]);
        putsUart0(str);
        sprintf(str, "[%02u] ", table.size[i]);
        putsUart0(str);
        sprintf(str, "[%02u] ", table.ackCmd[i]);
        putsUart0(str);
        putsUart0("[");
        for(j = 0; j < MAX_SIZE; j++)
        {
            sprintf(str, "[%02u] ", table.data[i][j]);
            putsUart0(str);
        }
        putsUart0("] ");
        sprintf(str, "[%02u] ", table.checksum[i]);
        putsUart0(str);
        sprintf(str, "[%02u]", table.retries[i]);
        putsUart0(str);
        putsUart0("]\r\n");
    }
}
