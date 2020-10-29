// shell.c
// William Bozarth
// Created on: October 7, 2020

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "shell.h"

// Function to get Input from Terminal
void getsUart0(USER_DATA* data)
{
    char c;

    c = UART0_DR_R & 0xFF; // Get character

    UART0_ICR_R = 0xFFF; // Clear any UART0 interrupts

    // Determine if user input is complete
    if((c == 13) || (data->characterCount == QUEUE_BUFFER_LENGTH))
    {
        data->buffer[data->characterCount++] = '\0';
        data->endOfString = true;
        sendUart0String("\r\n");
    }
    else if (data->characterCount > 0 && (c == 8 || c == 127)) // Decrement count if invalid character entered
    {
        data->characterCount--; // Decrement character count
        sendUart0String(" \b"); // Removes character from terminal display
    }
    else if (c >= ' ' && c < 127) // Converts capital letter to lower case (if necessary)
    {
        if('A' <= c && c <= 'Z')
        {
            data->buffer[data->characterCount++] = c + 32;
        }
        else
        {
            data->buffer[data->characterCount++] = c;
        }
    }
}

// Function to Tokenize Strings
void parseFields(USER_DATA* data)
{
    char    c;
    uint8_t count, fieldIndex;

    count = data->characterCount - 1; // Subtract by one to get correct character count

    c = data->buffer[count];

    // Check if at end of user input and exit function if so,
    // or if backspace or delete was entered by user.
    if(c == '\0' || count > MAX_CHARS || c == 8 || c == 127)
        return;

    // Get current Index for field arrays
    fieldIndex = data->fieldCount;

    if('a' <= c && c <= 'z') // Verify is character is an alpha (case sensitive)
    {
        if(data->delimeter)
        {
            data->fieldPosition[fieldIndex] = count;
            data->fieldType[fieldIndex] = 'A';
            data->fieldCount = ++fieldIndex;
            data->delimeter = false;
        }
    }
    else if(('0' <= c && c <= '9') || ',' == c ||  c == '.') //Code executes for numerics same as alpha
    {
        if(data->delimeter)
        {
            data->fieldPosition[fieldIndex] = count;
            data->fieldType[fieldIndex] = 'N';
            data->fieldCount = ++fieldIndex;
            data->delimeter = false;
        }
    }
    else // Insert NULL('\0') into character array if NON-alphanumeric character detected
    {
        data->buffer[count] = '\0';
        data->delimeter = true;
    }
}

// Function to Return a Token as a String
void getFieldString(USER_DATA* data, char fieldString[], uint8_t fieldNumber)
{
    uint8_t offset, index = 0;

    // Get position of first character in string of interest
    offset = data->fieldPosition[fieldNumber];

    // Copy characters for return
    while(data->buffer[offset] != '\0')
    {
        fieldString[index++] = data->buffer[offset++];
    }

    // Add NULL to terminate string
    fieldString[index] = '\0';
}

// Function to Return a Token as an Integer
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    int32_t num;
    uint8_t offset, index = 0;
    char copy[MAX_CHARS + 1];

    offset = data->fieldPosition[fieldNumber];  // Get position of first character in string of interest

    while(data->buffer[offset] != '\0')
    {
        copy[index++] = data->buffer[offset++];
    }

    copy[index] = '\0';

    num = atoi(copy);

    return num;
}

// Function Used to Determine if Correct Command Entered
bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments)
{
    bool command = false;
    char copy[MAX_CHARS + 1];
    uint8_t offset = 0, index = 0;

    while(data->buffer[offset] != '\0')
    {
        copy[index++] = data->buffer[offset++];
    }

    copy[index] = '\0';

    if((strcmp(strCommand, copy) == 0) && (data->fieldCount >= minArguments))
    {
        command = true;
    }

    return command;
}

// Function to reset User Input for next command
void resetUserInput(USER_DATA* data)
{
    data->characterCount = 0;
    data->fieldCount = 0;
    data->delimeter = true;
    data->endOfString = false;
}
