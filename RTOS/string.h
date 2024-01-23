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

#ifndef STRING_H_
#define STRING_H_

#include <stdbool.h>
#include <stdint.h>

// Max digits in 32 bit int
#define MAX_DIGITS 10

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

char* toLower(char* str);
bool strcmp(const char str1[], const char str2[]);
void strcpy(char* destination, const char* str_to_cpy);
uint64_t pow(uint32_t num, uint8_t exp);
char* itoa(uint32_t num, char* buffer);
char* itohex(uint32_t num, char* buffer);
char* iftoa(uint64_t num, uint8_t decimalOffset, uint8_t truncation, char* buffer);

#endif
