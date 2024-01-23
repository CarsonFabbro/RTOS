// Memory manager functions
// Carson Fabbro

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "mm.h"

typedef struct
{
    uint32_t size;
    bool free;
    void* address;
}_block;

#define NUM_BLOCKS 36

static _block heap[NUM_BLOCKS];

typedef struct
{
    uint16_t size;
    uint32_t* address;
    uint8_t region_number;
}_region;

static _region SRAM[NUM_SRAM_REGIONS];

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: add your malloc code here and update the SRD bits for the current thread
void * mallocFromHeap(uint32_t size_in_bytes)
{
   uint8_t i;
   if(size_in_bytes <= 512)
   {
       for(i = 8; i < NUM_BLOCKS; i++)
       {
           if(heap[i].size == 512 && heap[i].free)
           {
               heap[i].free = false;
               return heap[i].address;
           }
       }

   }
   else if(size_in_bytes <= 1024)
   {
       for(i = 0; i < NUM_BLOCKS; i++)
       {
           if(heap[i].size == 1024 && heap[i].free)
           {
               heap[i].free = false;
               return heap[i].address;
           }
       }
   }
   else if(size_in_bytes <= 1536)
   {
       if(heap[7].free)
       {
           heap[7].free = false;
           return heap[7].address;
       }
       else if(heap[22].free)
       {
           heap[22].free = false;
           return heap[22].address;
       }
       else if(heap[29].free)
       {
           heap[29].free = false;
           return heap[29].address;
       }
   }

   int8_t temp_index = -1;
   int8_t j;
   int16_t size_needed = size_in_bytes;
   int8_t num_blocks = 0;

   // If bigger than 1536
   for(i = 0; i < NUM_BLOCKS; i++)
   {
       if(heap[i].free && heap[i].size == 1024)
       {
           if(temp_index == -1)
           {
               temp_index = i;
               size_needed = size_in_bytes - 1024;
               num_blocks++;
           }
           else if(temp_index != -1 && size_needed > 0)
           {
               num_blocks++;
               size_needed -= 1024;
           }
       }
       else if(size_needed > 0)
       {
           num_blocks = 0;
           temp_index = -1;
           size_needed = size_in_bytes;
       }
       else if(size_needed <= 0)
       {
           heap[temp_index].free = false;
           for(j = 1; j < num_blocks; j++)
           {
               heap[temp_index + j].free = false;
           }
           return heap[temp_index + j - 1].address;
       }
   }
   if(size_needed <= 0)
   {
       heap[temp_index].free = false;
       for(j = 1; j < num_blocks; j++)
       {
           heap[temp_index + j].free = false;
       }
       return heap[temp_index + j - 1].address;
   }
   else
   {
      num_blocks = 0;
      temp_index = -1;
      size_needed = size_in_bytes;
   }

   for(i = 8; i < NUM_BLOCKS; i++)
   {
      if(heap[i].free && heap[i].size == 512)
      {
          if(temp_index == -1)
          {
              temp_index = i;
              size_needed = size_in_bytes - 512;
              num_blocks++;
          }
          else if(temp_index != -1 && size_needed > 0)
          {
              num_blocks++;
              size_needed -= 512;
          }
      }
      else if(size_needed > 0)
      {
          num_blocks = 0;
          temp_index = -1;
          size_needed = size_in_bytes;
      }
      else if(size_needed <= 0)
      {
          heap[temp_index].free = false;
          for(j = 1; j < num_blocks; j++)
          {
              heap[temp_index + j].free = false;
          }
          return heap[temp_index + j - 1].address;
      }
   }
   if(size_needed <= 0)
   {
       heap[temp_index].free = false;
       for(j = 1; j < num_blocks; j++)
       {
           heap[temp_index + j].free = false;
       }
       return heap[temp_index + j - 1].address;
   }
   else
   {
      num_blocks = 0;
      temp_index = -1;
      size_needed = size_in_bytes;
   }


   // Catch the rest
   for(i = 0; i < NUM_BLOCKS; i++)
   {
       if(heap[i].free)
       {
           if(temp_index == -1)
           {
               temp_index = i;
               size_needed = size_in_bytes - heap[i].size;
               num_blocks++;
           }
           else if(temp_index != -1 && size_needed > 0)
           {
               num_blocks++;
               size_needed -= heap[i].size;
           }
       }
       else if(size_needed > 0)
       {
           num_blocks = 0;
           temp_index = -1;
           size_needed = size_in_bytes;
       }
       else if(size_needed <= 0)
       {
           heap[temp_index].free = false;
           for(j = 1; j < num_blocks; j++)
           {
               heap[temp_index + j].free = false;
           }
           return heap[temp_index + j - 1].address;
       }
   }
   if(size_needed <= 0)
   {
       heap[temp_index].free = false;
       for(j = 1; j < num_blocks; j++)
       {
           heap[temp_index + j].free = false;
       }
       return heap[temp_index + j - 1].address;
   }

   return 0;
}

// REQUIRED: add your custom MPU functions here (eg to return the srd bits)
void generateSramSrdMasks(uint8_t srdMask[NUM_SRAM_REGIONS], void *baseAdd, uint32_t size_in_bytes)
{
    uint8_t i;
    uint8_t j;
    uint8_t srd_mask = 0xFF;
    bool reached = 0;

    for(i = 0; i < NUM_SRAM_REGIONS; i++)
    {
        NVIC_MPU_NUMBER_R |= NVIC_MPU_NUMBER_M; // Set region to 7 (to avoid accidental change)
        NVIC_MPU_NUMBER_R &= 0xFFFFFFF8 | SRAM[i].region_number; // Set to region

        if(((uint32_t)baseAdd < (uint32_t)SRAM[i].address + SRAM[i].size) && ((uint32_t)baseAdd + size_in_bytes > (uint32_t)SRAM[i].address))
        {
            for(j = 0; j < 8; j++)
            {
                if(!reached)
                {
                    if((uint32_t)baseAdd >= ((uint32_t)SRAM[i].address + (j*SRAM[i].size/8)) && (uint32_t)baseAdd < ((uint32_t)SRAM[i].address + ((j+1)*SRAM[i].size/8)))
                    {
                        reached = true;
                        srd_mask &= ~(0x01 << j);
                    }
                }
                else if(((uint32_t)baseAdd + size_in_bytes) > ((uint32_t)SRAM[i].address + (j*SRAM[i].size/8)) && reached)
                {
                    srd_mask &= ~(0x01 << j);
                }
            }
        }
        else
        {
            srd_mask = 0xFF;
        }

        srdMask[i] = srd_mask;

        srd_mask = 0xFF;
    }
}

void applySramSrdMasks(uint8_t srdMask[NUM_SRAM_REGIONS])
{
    // Function should take no longer than 70ish clks = 1.75us GOOD
    // Set for all 5, don't loop to avoid loss in time (10clks = 250ns = .25us)
    NVIC_MPU_NUMBER_R = SRAM[0].region_number; // Set to region
    NVIC_MPU_ATTR_R &= 0xFFFF00FF;
    NVIC_MPU_ATTR_R |= ((uint16_t)srdMask[0] << 8);

    NVIC_MPU_NUMBER_R = SRAM[1].region_number; // Set to region
    NVIC_MPU_ATTR_R &= 0xFFFF00FF;
    NVIC_MPU_ATTR_R |= ((uint16_t)srdMask[1] << 8);

    NVIC_MPU_NUMBER_R = SRAM[2].region_number; // Set to region
    NVIC_MPU_ATTR_R &= 0xFFFF00FF;
    NVIC_MPU_ATTR_R |= ((uint16_t)srdMask[2] << 8);

    NVIC_MPU_NUMBER_R = SRAM[3].region_number; // Set to region
    NVIC_MPU_ATTR_R &= 0xFFFF00FF;
    NVIC_MPU_ATTR_R |= ((uint16_t)srdMask[3] << 8);

    NVIC_MPU_NUMBER_R = SRAM[4].region_number; // Set to region
    NVIC_MPU_ATTR_R &= 0xFFFF00FF;
    NVIC_MPU_ATTR_R |= ((uint16_t)srdMask[4] << 8);
}


void allowFlashAccess(void)
{
    // set up flash region (region 0)
    NVIC_MPU_NUMBER_R &= ~(NVIC_MPU_NUMBER_M); // set to region 0
    NVIC_MPU_BASE_R &= ~(0xFFFC0008); //sets base addr to 0 N = 18, bits (N-1):5 reserved, & zero valid bit to ensure correct region edited
    NVIC_MPU_ATTR_R &=  ~(NVIC_MPU_ATTR_SIZE_M); // clear size bits
    NVIC_MPU_ATTR_R |= 0x00000011 << 1; //size = 2^(N+1) N = 17 2^18 = 256k 0x11
    NVIC_MPU_ATTR_R &= ~NVIC_MPU_ATTR_SRD_M; // disable all SRD
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_BUFFRABLE);
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_CACHEABLE; // set according to table 3-6
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_TEX_M);  // in datasheet
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_AP_M); // set AP to 0 (clear)
    NVIC_MPU_ATTR_R |= 0x03000000; // set AP to 011, Full access
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_XN); // instructions are executable

    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE; //enable region
}

void allowPeripheralAccess(void)
{
    // set up peripheral region (region 1)
    NVIC_MPU_NUMBER_R |= NVIC_MPU_NUMBER_M; // Set region to 7 (to avoid accidental change)
    NVIC_MPU_NUMBER_R &= 0xFFFFFFF9; // Set to region 1
    NVIC_MPU_BASE_R &= ~(0xFC000008); // turn off valid bit & zero base reg
    NVIC_MPU_BASE_R |= 0x40000000; //sets base addr to 0x40000000 N = 26, bits (N-1):5 reserved
                                      //, & zero valid bit to ensure correct region edited.
                                      // 0x40000000 / 0x04000000 = 0x10 = 0b010000
    NVIC_MPU_ATTR_R &=  ~(NVIC_MPU_ATTR_SIZE_M); // clear size bits
    NVIC_MPU_ATTR_R |= 0x00000019 << 1; // size = 2^(N+1)  N = 25 2^26 = 67MB = 0x19
    NVIC_MPU_ATTR_R &= ~NVIC_MPU_ATTR_SRD_M; // disable all SRD
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_CACHEABLE); // set according to table 3-6
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_BUFFRABLE;
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_TEX_M);  // in datasheet
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_AP_M); // set AP to 0 (clear)
    NVIC_MPU_ATTR_R |= 0x03000000; // set AP to 011, Full access
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN; // instructions are not executable

    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE; //enable region
}

void setupSramAccess(void)
{

    // set up SRAM region (region 2 - 4K block) (KERNEL HEAP)
    NVIC_MPU_NUMBER_R |= NVIC_MPU_NUMBER_M; // Set region to 7 (to avoid accidental change)
    NVIC_MPU_NUMBER_R &= 0xFFFFFFFA; // Set to region 2
    NVIC_MPU_BASE_R &= ~(0xFFFFF0008); // turn off valid bit and zero base reg
    NVIC_MPU_BASE_R |= 0x20000000;     //sets base addr to 0x20000000 N = 12, bits (N-1):5 reserved
                                      //, & zero valid bit to ensure correct region edited.
                                      // 0x20000000 / 0x00001000 = 0x20000 = 0b0010.0000.0000.0000.0000
    NVIC_MPU_ATTR_R &=  ~(NVIC_MPU_ATTR_SIZE_M); // clear size bits
    NVIC_MPU_ATTR_R |= 0x0000000B << 1; // size = 2^(N+1) N = 11 2^12 = 4KiB = 0xB
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SRD_M; // enable all SRD
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_BUFFRABLE); // set according to table 3-6
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE;
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_TEX_M);  // in datasheet
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_AP_M); // set AP to 0 (clear)
    NVIC_MPU_ATTR_R |= 0x01000000; // set AP to 001, RW for priv
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN; // instructions are not executable

    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE; //enable region

    // set up SRAM region (region 3 - 4K block)
    NVIC_MPU_NUMBER_R |= NVIC_MPU_NUMBER_M; // Set region to 7 (to avoid accidental change)
    NVIC_MPU_NUMBER_R &= 0xFFFFFFFB; // Set to region 3
    NVIC_MPU_BASE_R &= ~(0xFFFFF0008); // turn off valid bit and zero base reg
    NVIC_MPU_BASE_R |= 0x20001000;     //sets base addr to 0x20001000 N = 12, bits (N-1):5 reserved
                                      //, & zero valid bit to ensure correct region edited.
                                      // 0x20001000 / 0x00001000 = 0x20001 = 0b0010.0000.0000.0000.0001
    NVIC_MPU_ATTR_R &=  ~(NVIC_MPU_ATTR_SIZE_M); // clear size bits
    NVIC_MPU_ATTR_R |= 0x0000000B << 1; //size = 2^(N+1) N = 11 2^12 = 4KiB = 0xB
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SRD_M; // enable all SRD
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_BUFFRABLE); // set according to table 3-6
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE;
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_TEX_M);  // in datasheet
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_AP_M); // set AP to 0 (clear)
    NVIC_MPU_ATTR_R |= 0x03000000; // set AP to 011, RW for unpriv and priv
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN; // instructions are not executable

    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE; //enable region

    SRAM[0].address = (uint32_t *) 0x20001000;
    SRAM[0].region_number = 3;
    SRAM[0].size = 4096;

    // set up SRAM region (region 4 - 8K block)
    NVIC_MPU_NUMBER_R |= NVIC_MPU_NUMBER_M; // Set region to 7 (to avoid accidental change)
    NVIC_MPU_NUMBER_R &= 0xFFFFFFFC; // Set to region 4
    NVIC_MPU_BASE_R &= ~(0xFFFFE0008); // turn off valid bit and zero base reg
    NVIC_MPU_BASE_R |= 0x20002000;     //sets base addr to 0x20002000 N = 13, bits (N-1):5 reserved
                                      //, & zero valid bit to ensure correct region edited.
                                      // 0x20002000 / 0x00002000 = 0x10001 = 0b0010000000000000010
    NVIC_MPU_ATTR_R &=  ~(NVIC_MPU_ATTR_SIZE_M); // clear size bits
    NVIC_MPU_ATTR_R |= 0x0000000C << 1; //size = 2^(N+1) N = 12 2^13 = 8KiB = 0xC
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SRD_M; // enable all SRD
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_BUFFRABLE); // set according to table 3-6
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE;
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_TEX_M);  // in datasheet
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_AP_M); // set AP to 0 (clear)
    NVIC_MPU_ATTR_R |= 0x03000000; // set AP to 011, RW for unpriv and priv
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN; // instructions are not executable

    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE; //enable region

    SRAM[1].address = (uint32_t *) 0x20002000;
    SRAM[1].region_number = 4;
    SRAM[1].size = 8192;

    // set up SRAM region (region 5 - 4K block)
    NVIC_MPU_NUMBER_R |= NVIC_MPU_NUMBER_M; // Set region to 7 (to avoid accidental change)
    NVIC_MPU_NUMBER_R &= 0xFFFFFFFD; // Set to region 5
    NVIC_MPU_BASE_R &= ~(0xFFFFF0008); // turn off valid bit and zero base reg
    NVIC_MPU_BASE_R |= 0x20004000;     //sets base addr to 0x20004000 N = 12, bits (N-1):5 reserved
                                      //, & zero valid bit to ensure correct region edited.
                                      // 0x20004000 / 0x00001000 = 0x20004 = 0b0010.0000.0000.0000.0100
    NVIC_MPU_ATTR_R &=  ~(NVIC_MPU_ATTR_SIZE_M); // clear size bits
    NVIC_MPU_ATTR_R |= 0x0000000B << 1; //size = 2^(N+1) N = 11 2^12 = 4KiB = 0xB
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SRD_M; // enable all SRD
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_BUFFRABLE); // set according to table 3-6
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE;
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_TEX_M);  // in datasheet
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_AP_M); // set AP to 0 (clear)
    NVIC_MPU_ATTR_R |= 0x03000000; // set AP to 011, RW for unpriv and priv
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN; // instructions are not executable

    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE; //enable region

    SRAM[2].address = (uint32_t *) 0x20004000;
    SRAM[2].region_number = 5;
    SRAM[2].size = 4096;

    // set up SRAM region (region 6 - 4K block)
    NVIC_MPU_NUMBER_R |= NVIC_MPU_NUMBER_M; // Set region to 7 (to avoid accidental change)
    NVIC_MPU_NUMBER_R &= 0xFFFFFFFE; // Set to region 6
    NVIC_MPU_BASE_R &= ~(0xFFFFF0008); // turn off valid bit and zero base reg
    NVIC_MPU_BASE_R |= 0x20005000;     //sets base addr to 0x20005000 N = 12, bits (N-1):5 reserved
                                      //, & zero valid bit to ensure correct region edited.
                                      // 0x20005000 / 0x00001000 = 0x20005 = 0b0010.0000.0000.0000.0101
    NVIC_MPU_ATTR_R &=  ~(NVIC_MPU_ATTR_SIZE_M); // clear size bits
    NVIC_MPU_ATTR_R |= 0x0000000B << 1; //size = 2^(N+1) N = 11 2^12 = 4KiB = 0xB
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SRD_M; // enable all SRD
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_BUFFRABLE); // set according to table 3-6
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE;
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_TEX_M);  // in datasheet
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_AP_M); // set AP to 0 (clear)
    NVIC_MPU_ATTR_R |= 0x03000000; // set AP to 011, RW for unpriv and priv
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN; // instructions are not executable

    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE; //enable region

    SRAM[3].address = (uint32_t *) 0x20005000;
    SRAM[3].region_number = 6;
    SRAM[3].size = 4096;

    // set up SRAM region (region 7 - 8K block)
    NVIC_MPU_NUMBER_R |= NVIC_MPU_NUMBER_M; // Set region to 7
    NVIC_MPU_BASE_R &= ~(0xFFFFE0008); // turn off valid bit and zero base reg
    NVIC_MPU_BASE_R |= 0x20006000;     //sets base addr to 0x20006000 N = 13, bits (N-1):5 reserved
                                      //, & zero valid bit to ensure correct region edited.
                                      // 0x20006000 / 0x00002000 = 0x10003 = 0b0001000000000000110
    NVIC_MPU_ATTR_R &=  ~(NVIC_MPU_ATTR_SIZE_M); // clear size bits
    NVIC_MPU_ATTR_R |= 0x0000000C << 1; //size = 2^(N+1) N = 12 2^13 = 8KiB = 0xC
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SRD_M; // enable all SRD
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_BUFFRABLE); // set according to table 3-6
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE;
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_TEX_M);  // in datasheet
    NVIC_MPU_ATTR_R &= ~(NVIC_MPU_ATTR_AP_M); // set AP to 0 (clear)
    NVIC_MPU_ATTR_R |= 0x03000000; // set AP to 011, RW for unpriv and priv
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN; // instructions are not executable

    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_ENABLE; //enable region

    SRAM[4].address = (uint32_t *) 0x20006000;
    SRAM[4].region_number = 7;
    SRAM[4].size = 8192;
}

bool verifyAccess(uint8_t srdMaskRequired[NUM_SRAM_REGIONS], uint8_t srdMaskActive[NUM_SRAM_REGIONS])
{
    uint8_t i;
    uint8_t tempSrd;
    bool ok = true;

    // Loop through each mask, and verify that sub regions are enabled where they should be
    // If orring them results in a higher value than required mask, then there is an illegal region
    for(i = 0; i < NUM_SRAM_REGIONS; i++)
    {
        tempSrd = srdMaskRequired[i] | srdMaskActive[i];

        if(tempSrd > srdMaskRequired[i])
            ok = false;
    }

    return ok;
}

// REQUIRED: initialize MPU here
void initMpu(void)
{
    // REQUIRED: call your MPU functions here

    // -1 rule to ensure no issues (no background)
    NVIC_MPU_CTRL_R |= NVIC_MPU_CTRL_PRIVDEFEN;

    // Initialize heap
    uint8_t i;
    uint32_t temp_address = 0x20008000;
    for(i = 0; i < NUM_BLOCKS; i++)
    {
        if(i < 7 || (i > 22 && i < 29))
        {
            heap[i].size = 1024;
        }
        else if(i == 7 || i == 22 || i == 29)
        {
            heap[i].size = 1536;
        }
        else
        {
            heap[i].size = 512;
        }

        temp_address -= heap[i].size; //to get to base of block
        heap[i].address = (void*) temp_address;
        heap[i].free = true;
    }

    // Set region permissions
    allowFlashAccess();
    allowPeripheralAccess();
    setupSramAccess();

    // Init Interrupts
    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_USAGE;
    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_BUS;
    NVIC_SYS_HND_CTRL_R |= NVIC_SYS_HND_CTRL_MEM;

    // Enable MPU
    NVIC_MPU_CTRL_R |= NVIC_MPU_CTRL_ENABLE;
}

