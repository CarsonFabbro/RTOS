// Shell functions
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
#include "tm4c123gh6pm.h"
#include "faults.h"
#include "asm.h"
#include "string.h"
#include "kernel.h"
#include "uart0.h"

#define NUM_SRAM_REGIONS 5
#define BUF_SIZE 32

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// REQUIRED: If these were written in assembly
//           omit this file and add a faults.s file

// REQUIRED: code this function
void mpuFaultIsr(void)
{
    uint32_t pid = getCurrentPid();
    char str[BUF_SIZE] = {0};

    putsUart0("MPU fault in process ");
    putsUart0(itoa(pid, str));
    putcUart0('\n');

    uint32_t* psp = getPSP();
    uint32_t* msp = getMSP();

    putsUart0("PSP: ");
    putsUart0(itohex((uint32_t) psp, str));
    putcUart0('\n');

    putsUart0("MSP: ");
    putsUart0(itohex((uint32_t)msp, str));
    putcUart0('\n');

    putsUart0("Flags: ");
    // gets lower 8 bits of FAULTSTAT register, which contains mpu fault flags
    putsUart0(itohex(NVIC_FAULT_STAT_R & 0xFF, str));
    putcUart0('\n');

    putsUart0("At instruction: ");
    if(NVIC_FAULT_STAT_R & (NVIC_FAULT_STAT_DERR | NVIC_FAULT_STAT_IERR)) //if deer or ierr, pc has instr.
        putsUart0(itohex(*(psp + 6), str));
    else
        putsUart0("Unknown");

    putcUart0('\n');

    putsUart0("At address: ");
    // If mmarv bit is set, mmaddr reg contains valid address, else unknown
    if(NVIC_FAULT_STAT_R & NVIC_FAULT_STAT_MMARV)
        putsUart0(itohex(NVIC_MM_ADDR_R, str));
    else
        putsUart0("Unknown");
    putcUart0('\n');
    putcUart0('\n');

    putsUart0("---Process Stack Dump---\n\n");


    putsUart0("R0: ");
    putsUart0(itohex(*psp, str));
    putcUart0('\n');

    putsUart0("R1: ");
    putsUart0(itohex(*(psp + 1), str));
    putcUart0('\n');

    putsUart0("R2: ");
    putsUart0(itohex(*(psp + 2), str));
    putcUart0('\n');

    putsUart0("R3: ");
    putsUart0(itohex(*(psp + 3), str));
    putcUart0('\n');

    putsUart0("R12: ");
    putsUart0(itohex(*(psp + 4), str));
    putcUart0('\n');

    putsUart0("LR: ");
    putsUart0(itohex(*(psp + 5), str));
    putcUart0('\n');

    putsUart0("PC: ");
    putsUart0(itohex(*(psp + 6), str));
    putcUart0('\n');

    putsUart0("xPSR: ");
    putsUart0(itohex(*(psp + 7), str));
    putcUart0('\n');
    putcUart0('\n');

    // Clear memory management fault pending bit
    NVIC_SYS_HND_CTRL_R &= ~(NVIC_SYS_HND_CTRL_MEMP);

    // PendSV set to pending
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

// REQUIRED: code this function
void hardFaultIsr(void)
{
    uint32_t pid = getCurrentPid();
    char str[BUF_SIZE] = {0};

    putsUart0("Hard fault in process ");
    putsUart0(itoa(pid, str));
    putcUart0('\n');

    uint32_t psp = (uint32_t)getPSP();
    uint32_t msp = (uint32_t)getMSP();

    putsUart0("PSP: ");
    putsUart0(itohex(psp, str));
    putcUart0('\n');

    putsUart0("MSP: ");
    putsUart0(itohex(msp, str));
    putcUart0('\n');

    putsUart0("Flags: ");
    putsUart0(itohex(NVIC_HFAULT_STAT_R, str));
    putsUart0("\n\n");


    while(1)
    {
    }
}

// REQUIRED: code this function
void busFaultIsr(void)
{
    uint32_t pid = getCurrentPid();
    char str[BUF_SIZE] = {0};

    putsUart0("Bus fault in process ");
    putsUart0(itoa(pid, str));
    putcUart0('\n');
    putcUart0('\n');

    while(1)
    {
    }
}

// REQUIRED: code this function
void usageFaultIsr(void)
{
    uint32_t pid = getCurrentPid();
    char str[BUF_SIZE] = {0};

    putsUart0("Usage fault in process ");
    putsUart0(itoa(pid, str));
    putcUart0('\n');
    putcUart0('\n');

    while(1)
    {
    }
}

