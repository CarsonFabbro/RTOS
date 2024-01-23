// Kernel functions
// Carson Fabbro

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef KERNEL_H_
#define KERNEL_H_

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// function pointer
typedef void (*_fn)();

// tasks
#define MAX_TASKS 12

// mutex
#define MAX_MUTEXES 1
#define MAX_MUTEX_QUEUE_SIZE 2
#define resource 0

typedef struct _MUTEX_INFO
{
    char name[16];
    bool lock;
    char lockedBy[16];
    char waiters[MAX_MUTEX_QUEUE_SIZE][16];
    uint8_t numWaiters;
} MUTEX_INFO;

// semaphore
#define MAX_SEMAPHORES 3
#define MAX_SEMAPHORE_QUEUE_SIZE 2
#define keyPressed 0
#define keyReleased 1
#define flashReq 2

typedef struct _SEMAPHORE_INFO
{
    char name[16];
    uint8_t count;
    char waiters[MAX_SEMAPHORE_QUEUE_SIZE][16];
    uint8_t numWaiters;
} SEMAPHORE_INFO;

typedef struct _TASK_INFO
{
    char name[16];
    uint32_t pid;
    uint8_t state;
    char lockedBy[16];
    char cpuUsage[16];
    uint32_t ticks;
} TASK_INFO;

// task states
#define STATE_INVALID           0 // no task
#define STATE_STOPPED           1 // stopped, can be resumed
#define STATE_UNRUN             2 // task has never been run
#define STATE_READY             3 // has run, can resume at any time
#define STATE_DELAYED           4 // has run, but now awaiting timer
#define STATE_BLOCKED_MUTEX     5 // has run, but now blocked by semaphore
#define STATE_BLOCKED_SEMAPHORE 6 // has run, but now blocked by semaphore

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex, const char name[]);
bool initSemaphore(uint8_t semaphore, uint8_t count, const char name[]);

void initRtos(void);
void startRtos(void);

bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes);
void restartThread(_fn fn);
void stopThread(_fn fn);
void setThreadPriority(_fn fn, uint8_t priority);

void yield(void);
void sleep(uint32_t tick);
void lock(int8_t mutex);
void unlock(int8_t mutex);
void wait(int8_t semaphore);
void post(int8_t semaphore);
uint32_t getCurrentPid();

void systickIsr(void);
void __attribute__((naked)) pendSvIsr(void);
void svCallIsr(void);

#endif
