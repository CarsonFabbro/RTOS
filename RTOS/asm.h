// Assembly Library (RTOS)
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

#ifndef ASM_H_
#define ASM_H_

#include <stdint.h>

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

extern void setPSPAddress(uint32_t address);
extern void setThreadStackToPSP();
extern void setThreadModeUnprivileged();
extern uint32_t* getPSP();
extern uint32_t* getMSP();
extern void launchTaskUnprivileged(uint32_t address);
extern void pushHwHandledRegsToPsp(uint32_t pspAddress, uint32_t xPsr, uint32_t fnAddress);
extern uint32_t getPid(const char process[]);
extern uint8_t getMutexInfo(void *mutexStruct, uint8_t num);
extern uint8_t getSemaphoreInfo(void *semaphoreStruct, uint8_t num);
extern uint8_t getTaskInfo(void *taskStruct, uint8_t num);
extern void runThread(uint32_t fn);
extern void killThread(uint32_t fn);
extern void enablePreemption();
extern void disablePreemption();
extern void setSchedPriority();
extern void setSchedRoundRobin();
extern void changeThreadPriority(uint32_t fn, uint8_t prio);

#endif
