// Memory manager functions
// Carson Fabbro

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef MM_H_
#define MM_H_

#define NUM_SRAM_REGIONS 5

#include <stdbool.h>

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void * mallocFromHeap(uint32_t size_in_bytes);
void initMpu(void);
void generateSramSrdMasks(uint8_t srdMask[NUM_SRAM_REGIONS], void *baseAdd, uint32_t size_in_bytes);
void applySramSrdMasks(uint8_t srdMask[NUM_SRAM_REGIONS]);
bool verifyAccess(uint8_t srdMaskRequired[NUM_SRAM_REGIONS], uint8_t srdMaskActive[NUM_SRAM_REGIONS]);

#endif
