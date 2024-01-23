// String Library
// Carson Fabbro

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL
// Target uC:       TM4C123GH6PM
// System Clock:    -

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include "string.h"
#include <stdlib.h>

// Max digits in 32 bit int
#define MAX_DIGITS_32 10
#define MAX_DIGITS_64 20

// Max hex digits in 32 bit int
#define MAX_HEX 8

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

char* toLower(char* str)
{
    uint16_t i = 0;
    while(str[i])
    {
        if(str[i] >= 65 && str[i] <= 90)
        {
            str[i] += 32;
        }
    }
    return str;
}

bool strcmp(const char str1[], const char str2[])
{

    uint16_t i = 0;

    while(str1[i] != NULL || str2[i] != NULL)
    {
        if(str1[i] == str2[i])
        {
            i++;
        }
        else
        {
            return false;
        }
    }

    return true;
}

void strcpy(char* destination, const char* str_to_cpy)
{
    uint16_t i = 0;
    while((str_to_cpy + i) != NULL && *(str_to_cpy + i) != '\0')
    {
        *(destination + i) = *(str_to_cpy + i);
        i++;
    }
    *(destination + i) = '\0';
}

uint64_t pow(uint32_t num, uint8_t exp)
{
    uint64_t res = 1;
    for(exp; exp > 0; exp--)
    {
        res *= num;
    }
    return res;
}

char* itoa(uint32_t num, char* buffer)
{
    uint32_t temp;
    uint8_t index = 0;
    int8_t i;
    bool hasDigit = false;
    char str[MAX_DIGITS_32 + 1] = {0};
    for(i = MAX_DIGITS_32 - 1; i >= 0; i--)
    {
        temp = num / pow(10, i);
        if(temp > 0)
        {
            hasDigit = true;
            str[index] = temp + '0';
            index++;
            num -= temp*pow(10, i);
        }
        else if(hasDigit)
        {
            str[index] = '0';
            index++;
        }

    }

    if(!hasDigit)
    {
        str[index] = 48;
        str[index + 1] = '\0';
    }
    else
        str[index] = '\0';

    strcpy(buffer, str);

    return buffer;
}

// int-float to ascii. Takes integer represented float, and shifts decimalOffsets starting from right
char* iftoa(uint64_t num, uint8_t decimalOffset, uint8_t truncation, char* buffer)
{
    uint64_t temp = num / pow(10, decimalOffset);
    uint8_t index = 0;
    int8_t i;
    bool hasDigit = false;
    char str[MAX_DIGITS_64 + 2] = {0};

    // If num is below 1 percent, shift to make space for data
    if(temp == 0)
    {
        i = decimalOffset;
        while(temp == 0)
        {
            temp = num / pow(10, i);
            str[decimalOffset - i] = '0';
            i--;
        }
        index = decimalOffset - i; // Offset
    }

    for(i = MAX_DIGITS_64 - 1; i >= 0; i--)
    {
        temp = num / pow(10, i);
        if(temp > 0)
        {
            hasDigit = true;
            str[index] = temp + '0';
            index++;
            num -= temp*pow(10, i);
        }
        else if(hasDigit)
        {
            str[index] = '0';
            index++;
        }

    }

    if(!hasDigit)
    {
        str[index] = 48;
        str[index + 1] = '\0';
    }
    else
    {// in here i think limiting to anything over zero
        if(decimalOffset <= index)
        {
            for(i = index; i > index - decimalOffset; i--)
            {
                str[i] = str[i - 1];
            }
            str[i] = '.';
            str[i + truncation + 1] = '\0';

        }
    }

    strcpy(buffer, str);
    return buffer;
}

char* itohex(uint32_t num, char* buffer)
{
    char reverse_hex[MAX_HEX + 3];
    uint8_t i = 0;
    uint32_t quotient = num;
    uint8_t remainder = 0;
    reverse_hex[0] = '\0';
    reverse_hex[MAX_HEX + 2] = '0';
    reverse_hex[MAX_HEX + 1] = 'x';

    for(i = 1; i < MAX_HEX + 1; i++)
    {
       reverse_hex[i] = '0';
    }

    i = 1;

    while(quotient != 0)
    {
        remainder = quotient % 16;

        if(remainder >= 10)
        {
            reverse_hex[i] = remainder + 55;
        }
        else
        {
            reverse_hex[i] = remainder + 48;
        }

        i++;
        quotient /= 16;
    }

    char hex[MAX_HEX + 3];

    for(i = 0; i < MAX_HEX + 3; i++)
    {
        hex[i] = reverse_hex[MAX_HEX + 2 - i];
    }

    strcpy(buffer, hex);
    return buffer;
}
