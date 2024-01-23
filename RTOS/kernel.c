// Kernel functions
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
#include <stdio.h>
#include "tm4c123gh6pm.h"
#include "mm.h"
#include "kernel.h"
#include "string.h"
#include "uart0.h"
#include "asm.h"

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// mutex
typedef struct _mutex
{
    char name[16];
    bool lock;
    uint8_t queueSize;
    uint8_t processQueue[MAX_MUTEX_QUEUE_SIZE];
    uint8_t lockedBy;
} mutex;
mutex mutexes[MAX_MUTEXES];

// semaphore
typedef struct _semaphore
{
    char name[16];
    uint8_t count;
    uint8_t queueSize;
    uint8_t processQueue[MAX_SEMAPHORE_QUEUE_SIZE];
} semaphore;
semaphore semaphores[MAX_SEMAPHORES];

// task
uint8_t taskCount = 0;            // total number of valid tasks
uint8_t taskCurrent = 0;          // index of last dispatched task

// cpu usage
uint8_t clockCurrent = 0;         // current clock counting
uint16_t intCount = 0;            // num systick ints
uint32_t startClocks = 39999;         // clock num saved when leaving pendsv
uint32_t clkSum = 0;

#define PERIOD_MS 1000            // 10000000 clks
#define PERIOD_CLKS 40000000
#define BUF_SIZE 32

// control
bool priorityScheduler = false;   // priority (true) or round-robin (false)
bool priorityInheritance = false; // priority inheritance for mutexes
bool preemption = false;          // preemption (true) or cooperative (false)

// tcb
#define NUM_PRIORITIES   8

struct _tcb
{
    uint8_t state;                 // see STATE_ values above
    void *pid;                     // used to uniquely identify thread (add of task fn)
    void *spInit;                  // original top of stack
    void *sp;                      // current stack pointer
    uint8_t priority;              // 0=highest
    uint8_t currentPriority;       // 0=highest (needed for pi)
    uint32_t ticks;                // ticks until sleep complete
    uint8_t srd[NUM_SRAM_REGIONS]; // MPU subregion disable bits
    char name[16];                 // name of task used in ps command
    uint8_t mutex;                 // index of the mutex in use or blocking the thread
    uint8_t semaphore;             // index of the semaphore that is blocking the thread
    uint32_t clocks[2];             // clocks for keeping cpu usage (one sampling, one stable)
} tcb[MAX_TASKS];

uint8_t taskCurrentForPriority[NUM_PRIORITIES]; // Tracks last task run on each prio level

// SVC number defines
#define YIELD       0
#define SLEEP       1
#define LOCK        2
#define UNLOCK      3
#define WAIT        4
#define POST        5
#define PIDOF       6
#define MUT         7
#define SEM         8
#define PS          9
#define RUN         10
#define KILL        11
#define PREEMPT_EN  12
#define PREEMPT_DIS 13
#define SCHED_PRIO  14
#define SCHED_RR    15
#define SET_PRIO    16

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex, const char name[])
{
    bool ok = (mutex < MAX_MUTEXES);
    if (ok)
    {
        mutexes[mutex].lock = false;
        mutexes[mutex].lockedBy = 0;
        strcpy(mutexes[mutex].name, name);
    }
    return ok;
}

bool initSemaphore(uint8_t semaphore, uint8_t count, const char name[])
{
    bool ok = (semaphore < MAX_SEMAPHORES);
    {
        semaphores[semaphore].count = count;
        strcpy(semaphores[semaphore].name, name);
    }
    return ok;
}

// REQUIRED: initialize systick for 1ms system timer
void initRtos(void)
{
    uint8_t i;
    // no tasks running
    taskCount = 0;
    // clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;
        tcb[i].pid = 0;
    }

    NVIC_ST_RELOAD_R = 39999; // Sets system to interrupt at 1kHz rate
    NVIC_ST_CTRL_R |= NVIC_ST_CTRL_INTEN | NVIC_ST_CTRL_ENABLE | NVIC_ST_CTRL_CLK_SRC; // Enable interrupts and systick
}

// REQUIRED: Implement prioritization to NUM_PRIORITIES
uint8_t rtosScheduler(void)
{
    bool ok;
    static uint8_t task = 0xFF;
    ok = false;

    if(priorityScheduler)
    {
        uint8_t i;
        uint8_t highest_priority = NUM_PRIORITIES;
        // Find highest priority with a task ready to run (lower prio wins)
        for(i = 0; i < taskCount; i++)
        {
            if((tcb[i].state == STATE_READY || tcb[i].state == STATE_UNRUN) && tcb[i].priority < highest_priority)
                highest_priority = tcb[i].priority;
        }

        task = taskCurrentForPriority[highest_priority];

        while(!ok)
        {
            task++;
            if(task >= MAX_TASKS)
                task = 0;
            ok = (tcb[task].state == STATE_READY || tcb[task].state == STATE_UNRUN) && tcb[task].priority == highest_priority;
        }

        taskCurrentForPriority[highest_priority] = task;
    }
    else
    {
        while (!ok)
        {
            task++;
            if (task >= MAX_TASKS)
                task = 0;
            ok = (tcb[task].state == STATE_READY || tcb[task].state == STATE_UNRUN);
        }
    }

    return task;
}

// REQUIRED: modify this function to start the operating system
// by calling scheduler, set srd bits, setting PSP, ASP bit, TMPL bit, and PC
void startRtos(void)
{
    // Call scheduler and apply first tasks srd regions
    taskCurrent = rtosScheduler();
    applySramSrdMasks(tcb[taskCurrent].srd);

    // Set task as ready
    tcb[taskCurrent].state = STATE_READY;

    // Set PSP address and set it as active SP (set ASP)
    setPSPAddress((uint32_t) tcb[taskCurrent].sp);
    setThreadStackToPSP();


    // Call fn to set TMPL and launch first task
    // (need to do this to store fn in R0 bc setting TMPL will deny access to globals)
    // If this doesnt work go back to 10/19 lecture end of lecture like 1:15
    launchTaskUnprivileged((uint32_t) tcb[taskCurrent].pid);
}

// REQUIRED:
// add task if room in task list
// store the thread name
// allocate stack space and store top of stack in sp and spInit
// set the srd bits based on the memory allocation
bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes)
{
    bool ok = false;
    uint8_t i = 0;
    uint8_t j;
    uint8_t srdMask[NUM_SRAM_REGIONS];
    void* stackPtr;
    bool found = false;
    if (taskCount < MAX_TASKS)
    {
        // make sure fn not already in list (prevent reentrancy)
        while (!found && (i < MAX_TASKS))
        {
            found = (tcb[i++].pid ==  fn);
        }
        if (!found)
        {
            // find first available tcb record
            i = 0;
            while (tcb[i].state != STATE_INVALID) {i++;}

            stackPtr = mallocFromHeap(stackBytes);
            generateSramSrdMasks(srdMask, stackPtr, stackBytes);

            tcb[i].state = STATE_UNRUN;
            tcb[i].pid = fn;
            tcb[i].sp = (void *) ((uint8_t *) stackPtr + stackBytes);
            tcb[i].spInit = (void *) ((uint8_t *) stackPtr + stackBytes);
            tcb[i].priority = priority;
            for (j = 0; j < NUM_SRAM_REGIONS; j++)
                tcb[i].srd[j] = srdMask[j];
            strcpy(tcb[i].name, name);

            // increment task count
            taskCount++;
            ok = true;
        }
    }
    return ok;
}

// REQUIRED: modify this function to restart a thread
void restartThread(_fn fn)
{
    uint8_t i;

    // Find the task and set the current sp to the top of the stack, and mark as unrun
    for(i = 0; i < taskCount; i++)
    {
        if(tcb[i].pid == (void *)fn)
        {
            tcb[i].sp = tcb[i].spInit;
            tcb[i].state = STATE_UNRUN;
            //break;
        }
    }
}

// REQUIRED: modify this function to stop a thread
// REQUIRED: remove any pending semaphore waiting, unlock any mutexes
void stopThread(_fn fn)
{
    uint8_t i;
    uint8_t j;
    uint8_t k;

    // Find the task, unlock any mutexes, remove from any resource queues, and mark as stopped
    for(i = 0; i < taskCount; i++)
    {
        if(tcb[i].pid == fn)
        {
            if(tcb[i].state == STATE_BLOCKED_MUTEX)
            {
                mutexes[tcb[i].mutex].lock = false;
                if(mutexes[tcb[i].mutex].queueSize > 0)
                {
                    tcb[mutexes[tcb[i].mutex].processQueue[0]].state = STATE_READY; // Next task in queue ready
                    mutexes[tcb[i].mutex].queueSize --;
                    mutexes[tcb[i].mutex].lock = true;                              // Lock mutex with next in queue
                    mutexes[tcb[i].mutex].lockedBy = mutexes[tcb[i].mutex].processQueue[0];

                    // Dequeue
                    for(j = 0; j < MAX_MUTEX_QUEUE_SIZE - 1; j++)
                    {
                        mutexes[tcb[i].mutex].processQueue[j] = mutexes[tcb[i].mutex].processQueue[j + 1];
                    }
                }
            }
            else if(tcb[i].state == STATE_BLOCKED_SEMAPHORE)
            {
                for(j = 0; j < semaphores[tcb[i].semaphore].queueSize; j++)
                {

                   if(semaphores[tcb[i].semaphore].processQueue[j] == i)
                   {
                       // If others behind, shift queue
                       if(j < semaphores[tcb[i].semaphore].queueSize - 1)
                       {
                           for(k = j; k < semaphores[tcb[i].semaphore].queueSize - 1; k++)
                           {
                               semaphores[tcb[i].semaphore].processQueue[k] = semaphores[tcb[i].semaphore].processQueue[k + 1];
                           }
                       }
                       semaphores[tcb[i].semaphore].queueSize--;
                   }
                }
            }

            for(j = 0; j < MAX_MUTEXES; j++)
            {
                if(mutexes[j].lock && mutexes[j].lockedBy == i)
                {
                    mutexes[j].lock = false;
                    if(mutexes[j].queueSize > 0)
                    {
                        tcb[mutexes[j].processQueue[0]].state = STATE_READY; // Next task in queue ready
                        mutexes[j].queueSize--;
                        mutexes[j].lock = true;                              // Lock mutex with next in queue
                        mutexes[j].lockedBy = mutexes[j].processQueue[0];

                        // Dequeue
                        for(k = 0; k < MAX_MUTEX_QUEUE_SIZE - 1; k++)
                        {
                            mutexes[j].processQueue[k] = mutexes[j].processQueue[k + 1];
                        }
                    }
                }
            }

            tcb[i].state = STATE_STOPPED;
           // break;
        }
    }
}

// REQUIRED: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
    uint8_t i;

    // Find the task and set the current sp to the top of the stack, and mark as unrun
    for(i = 0; i < taskCount; i++)
    {
        if(tcb[i].pid == fn)
        {
            tcb[i].priority = priority;
            break;
        }
    }
}

// REQUIRED: modify this function to yield execution back to scheduler using pendsv
void yield(void)
{
    __asm("     SVC  #0");
}

// REQUIRED: modify this function to support 1ms system timer
// execution yielded back to scheduler until time elapses using pendsv
void sleep(uint32_t tick)
{
    __asm("     SVC  #1");
}

// REQUIRED: modify this function to lock a mutex using pendsv
void lock(int8_t mutex)
{
    __asm("     SVC  #2");
}

// REQUIRED: modify this function to unlock a mutex using pendsv
void unlock(int8_t mutex)
{
    __asm("     SVC  #3");
}

// REQUIRED: modify this function to wait a semaphore using pendsv
void wait(int8_t semaphore)
{
    __asm("     SVC  #4");
}

// REQUIRED: modify this function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
    __asm("     SVC  #5");
}

// gets pid of current program for faults.c
uint32_t getCurrentPid()
{
    return (uint32_t)tcb[taskCurrent].pid;
}

// REQUIRED: modify this function to add support for the system timer
// REQUIRED: in preemptive code, add code to request task switch
void systickIsr(void)
{
    intCount++;
    uint8_t i;
    tcb[taskCurrent].clocks[clockCurrent] += startClocks;
    startClocks = 40000;
    if(intCount == PERIOD_MS)
    {
        clkSum = 0;
        intCount = 0;
        clockCurrent ^= 1;

        for(i = 0; i < taskCount; i++)
        {
            tcb[i].clocks[clockCurrent] = 0;
            clkSum += tcb[i].clocks[clockCurrent ^ 1];
        }
    }

    // For all sleeping tasks, decrement tick count
    for(i = 0; i < taskCount; i++)
    {
        if(tcb[i].state == STATE_DELAYED && !(--tcb[i].ticks))
            tcb[i].state = STATE_READY;
    }

    // If preemption turned on, call pendsv
    if(preemption)
        NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

// REQUIRED: in coop and preemptive, modify this function to add support for task switching
// REQUIRED: process UNRUN and READY tasks differently
void __attribute__((naked)) pendSvIsr(void)
{
    tcb[taskCurrent].clocks[clockCurrent] += startClocks - (NVIC_ST_CURRENT_R & NVIC_ST_CURRENT_M);
    uint32_t* psp;

    // if DERR or IERR bit set, mpu fault, so must kill process
    if(NVIC_FAULT_STAT_R & (NVIC_FAULT_STAT_DERR | NVIC_FAULT_STAT_IERR))
    {
        stopThread((_fn)tcb[taskCurrent].pid);
        NVIC_FAULT_STAT_R |= (NVIC_FAULT_STAT_DERR | NVIC_FAULT_STAT_IERR);
    }

    // else save program state
    else
    {
        // Push regs to psp
        __asm("START_FN:            MRS   R0, PSP");          // Get PSP into R0
        __asm("                     MOV   R1, #0xFFFF");      // Get 0xFFFFFFED into R1
        __asm("                     LSL   R2, R1, #16");
        __asm("                     ADD   R2, R1, R2");
        __asm("                     SUB   R1, R2, #18");      // 0xFFFFFFED in R1 (FP_EXCL_RES)
        __asm("                     CMP   LR, R1");           // If fp, lazy stacking, so must push S16-31
        __asm("                     MOV   R2, LR");           // Move LR into R2 (will be overwritten by bne)
        __asm("                     BNE   PUSH_NON_FP_REGS"); // If non-fp skip S reg pushes
        __asm("                     VSTR  S31, [R0, #-4]!");  // push S31 to psp
        __asm("                     VSTR  S30, [R0, #-8]!");  // push S30 to psp
        __asm("                     VSTR  S29, [R0, #-12]!"); // push S29 to psp
        __asm("                     VSTR  S28, [R0, #-16]!"); // push S28 to psp
        __asm("                     VSTR  S27, [R0, #-20]!"); // push S27 to psp
        __asm("                     VSTR  S26, [R0, #-24]!"); // push S26 to psp
        __asm("                     VSTR  S25, [R0, #-28]!"); // push S25 to psp
        __asm("                     VSTR  S24, [R0, #-32]!"); // push S24 to psp
        __asm("                     VSTR  S23, [R0, #-36]!"); // push S23 to psp
        __asm("                     VSTR  S22, [R0, #-40]!"); // push S22 to psp
        __asm("                     VSTR  S21, [R0, #-44]!"); // push S21 to psp
        __asm("                     VSTR  S20, [R0, #-48]!"); // push S20 to psp
        __asm("                     VSTR  S19, [R0, #-52]!"); // push S19 to psp
        __asm("                     VSTR  S18, [R0, #-56]!"); // push S18 to psp
        __asm("                     VSTR  S17, [R0, #-60]!"); // push S17 to psp
        __asm("                     VSTR  S16, [R0, #-64]!"); // push S16 to psp
        __asm("                     SUB   R0, R0, #64");      // adjust psp
        __asm("PUSH_NON_FP_REGS:    STR   R2, [R0, #-4]!");   // push lr to psp
        __asm("                     STR   R11, [R0, #-4]!");  // push R11 to psp
        __asm("                     STR   R10, [R0, #-4]!");  // push R10 to psp
        __asm("                     STR   R9, [R0, #-4]!");   // push R9 to psp
        __asm("                     STR   R8, [R0, #-4]!");   // push R8 to psp
        __asm("                     STR   R7, [R0, #-4]!");   // push R7 to psp
        __asm("                     STR   R6, [R0, #-4]!");   // push R6 to psp
        __asm("                     STR   R5, [R0, #-4]!");   // push R5 to psp
        __asm("                     STR   R4, [R0, #-4]!");   // push R4 to psp
        __asm("                     MSR   PSP, R0");          // Store new PSP address

        psp = getPSP(); // Get new psp address after pushes
        tcb[taskCurrent].sp = (void *)psp; // Save PSP to tcb
    }


    // Schedule next task and apply its srd regions
    taskCurrent = rtosScheduler();
    applySramSrdMasks(tcb[taskCurrent].srd);

    // PendSV pending cleared
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_UNPEND_SV;

    psp = (uint32_t *)tcb[taskCurrent].sp;
    setPSPAddress((uint32_t)psp);       // Restore PSP address

    if(tcb[taskCurrent].state == STATE_READY)
    {
        // Pop all registers back (restore program state), and get correct EXCL_RES -> LR
        __asm("         MRS   R0, PSP");         // Get PSP into R0
        __asm("         MOV   R1, #0xFFFF");     // Get 0xFFFFFFED into R1
        __asm("         LSL   R2, R1, #16");
        __asm("         ADD   R2, R1, R2");
        __asm("         SUB   R1, R2, #18");     // 0xFFFFFFED in R1 (FP_EXCL_RES)
        __asm("         LDR   R4, [R0], #4");    // Pop program R4 off PSP
        __asm("         LDR   R5, [R0], #4");    // Pop program R5 off PSP
        __asm("         LDR   R6, [R0], #4");    // Pop program R6 off PSP
        __asm("         LDR   R7, [R0], #4");    // Pop program R7 off PSP
        __asm("         LDR   R8, [R0], #4");    // Pop program R8 off PSP
        __asm("         LDR   R9, [R0], #4");    // Pop program R9 off PSP
        __asm("         LDR   R10, [R0], #4");   // Pop program R10 off PSP
        __asm("         LDR   R11, [R0], #4");   // Pop program R11 off PSP
        __asm("         LDR   R2, [R0], #4");    // Pop program LR off PSP (EXCL_RES)
        __asm("         CMP   R2, R1");          // If fp, lazy stacking, so must pop S16-31
        __asm("         BNE   EXIT_FN");         // If non-fp, dont pop S regs
        __asm("         VLDR  S16, [R0, #0]");   // Pop program S16 off PSP
        __asm("         VLDR  S17, [R0, #4]");   // Pop program S17 off PSP
        __asm("         VLDR  S18, [R0, #8]");   // Pop program S18 off PSP
        __asm("         VLDR  S19, [R0, #12]");  // Pop program S19 off PSP
        __asm("         VLDR  S20, [R0, #16]");  // Pop program S20 off PSP
        __asm("         VLDR  S21, [R0, #20]");  // Pop program S21 off PSP
        __asm("         VLDR  S22, [R0, #24]");  // Pop program S22 off PSP
        __asm("         VLDR  S23, [R0, #28]");  // Pop program S23 off PSP
        __asm("         VLDR  S24, [R0, #32]");  // Pop program S24 off PSP
        __asm("         VLDR  S25, [R0, #36]");  // Pop program S25 off PSP
        __asm("         VLDR  S26, [R0, #40]");  // Pop program S26 off PSP
        __asm("         VLDR  S27, [R0, #44]");  // Pop program S27 off PSP
        __asm("         VLDR  S28, [R0, #48]");  // Pop program S28 off PSP
        __asm("         VLDR  S29, [R0, #52]");  // Pop program S29 off PSP
        __asm("         VLDR  S30, [R0, #56]");  // Pop program S30 off PSP
        __asm("         VLDR  S31, [R0, #60]");  // Pop program S31 off PSP
        __asm("         ADD   R0, R0, #64");     // Reposition PSP to new addr
        __asm("EXIT_FN: MSR   PSP, R0");         // Store new PSP
        __asm("         MOV   LR, R2");          // Move excl res into LR
    }
    else
    {
        // Make it look like it has run, so it can be "restored".
        // Can use non-floating pt EXCL bc only PC and xPSR matter
        pushHwHandledRegsToPsp((uint32_t)psp, 0x01000000, (uint32_t)tcb[taskCurrent].pid);
        tcb[taskCurrent].state = STATE_READY;

        // Get non-fp exclusion result into LR
        __asm("     MOV   R0, #0xFFFF");         // Get 0xFFFFFFFD into R0 (non-fp exclusion result)
        __asm("     LSL   R1, R0, #16");
        __asm("     ADD   R1, R0, R1");
        __asm("     SUB   R0, R1, #2");          // 0xFFFFFFFD in R0 (NON_FP_EXCL_RES)
        __asm("     MOV   LR, R0");              // Move non-fp excl res into LR
    }

    startClocks = NVIC_ST_CURRENT_R & NVIC_ST_CURRENT_M; //Current value of systick counter

    __asm("     BX LR"); //BX LR (Returns to program, and pushes correct registers based on EXCL_RES)
}

// REQUIRED: modify this function to add support for the service call
// REQUIRED: in preemptive code, add code to handle synchronization primitives
void svCallIsr(void)
{
    // Decode and store call #
    uint32_t* psp = getPSP();
    uint32_t pc = *(psp + 6); // Loads PC into svc_num
    uint8_t svc_num = *(((uint8_t *)pc) - 2); // Instr one before PC contains
                                              // the # bc PC pts to next instr
                                              // but # is 2nd byte of prev instr

    // Extract all possible parameters
    uint32_t r0 = *psp;
    uint32_t r1 = *(psp + 1);
    uint8_t srdMask[NUM_SRAM_REGIONS];
    bool ok;

    MUTEX_INFO* mutexInfo;
    SEMAPHORE_INFO* semaphoreInfo;
    TASK_INFO* taskInfo;
    uint32_t pid = 0;
    char* str = {0};
    char buf[BUF_SIZE] = {0};

    uint8_t i;

    // Switch functionality depending on svc #
    switch(svc_num)
    {
        case YIELD:
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; // Pend PendSV
            break;
        case SLEEP:
            tcb[taskCurrent].state = STATE_DELAYED;   // Set state to delayed
            tcb[taskCurrent].ticks = r0;              // R0 contains tick num
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV; // Pend PendSv
            break;
        case LOCK:
            // If mutex is unlocked then lock. Else add to queue
            if(!mutexes[r0].lock) //R0 contains index of mutex
            {
                mutexes[r0].lock = true;
                mutexes[r0].lockedBy = taskCurrent;
                return;
            }
            else
            {
                tcb[taskCurrent].mutex = r0;                                   // Add blocked mutex to tcb entry
                mutexes[r0].processQueue[mutexes[r0].queueSize] = taskCurrent; // Add to queue
                mutexes[r0].queueSize++;                                       // Incr. queue size
                tcb[taskCurrent].state = STATE_BLOCKED_MUTEX;                  // Set state
                NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;                      // Pend PendSv
            }
            break;
        case UNLOCK:
            // If mutex was locked by task, unlock, and allow next task in queue to run
            if(mutexes[r0].lockedBy == taskCurrent)
            {
                mutexes[r0].lock = false;
                if(mutexes[r0].queueSize > 0)
                {
                    tcb[mutexes[r0].processQueue[0]].state = STATE_READY; // Next task in queue ready
                    mutexes[r0].queueSize--;
                    mutexes[r0].lock = true;                              // Lock mutex with next in queue
                    mutexes[r0].lockedBy = mutexes[r0].processQueue[0];

                    // Dequeue
                    for(i = 0; i < MAX_MUTEX_QUEUE_SIZE - 1; i++)
                    {
                        mutexes[r0].processQueue[i] = mutexes[r0].processQueue[i + 1];
                    }
                }
            }
            break;
        case WAIT:
            // If semaphore count > 0, decrement count and return. Else place in queue and wait
            if(semaphores[r0].count > 0)
            {
                semaphores[r0].count--;
                return;
            }
            else
            {
                semaphores[r0].processQueue[semaphores[r0].queueSize] = taskCurrent;
                semaphores[r0].queueSize++;
                tcb[taskCurrent].semaphore = r0;                  // Log in tcb what semaphore is blocking task
                tcb[taskCurrent].state = STATE_BLOCKED_SEMAPHORE; // Update task state
                NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;         // Pend PendSv
            }
            break;
        case POST:
            semaphores[r0].count++;
            //If someone in queue set task to ready and update queue
            if(semaphores[r0].queueSize > 0)
            {
                tcb[semaphores[r0].processQueue[0]].state = STATE_READY;
                semaphores[r0].queueSize--;

                // Dequeue
                for(i = 0; i < MAX_SEMAPHORE_QUEUE_SIZE - 1; i++)
                {
                    semaphores[r0].processQueue[i] = semaphores[r0].processQueue[i + 1];
                }
                semaphores[r0].count--; // Decrement count since process in queue
            }
            break;
        case PIDOF:
            str = (char *) r0;

            if(str != NULL)
            {
                for(i = 0; i < taskCount; i++)
                {
                    if(strcmp(tcb[i].name, str))
                    {
                        pid = (uint32_t)tcb[i].pid;
                    }
                }
            }

            *psp = pid;
            break;
        case MUT:
            mutexInfo = (MUTEX_INFO *) r0;

            generateSramSrdMasks(srdMask, (void *)mutexInfo, sizeof(*mutexInfo));
            ok = verifyAccess(srdMask, tcb[taskCurrent].srd);

            if(r1 > MAX_MUTEXES - 1)
                ok = false;

            if(ok)
            {
                mutexInfo->lock = mutexes[r1].lock;
                strcpy(mutexInfo->lockedBy, tcb[mutexes[r1].lockedBy].name);
                strcpy(mutexInfo->name, mutexes[r1].name);
                mutexInfo->numWaiters = mutexes[r1].queueSize;
                if(mutexes[r1].queueSize > 0)
                {
                    for(i = 0; i < mutexes[r1].queueSize; i++)
                    {
                        strcpy(mutexInfo->waiters[i], tcb[mutexes[r1].processQueue[i]].name);
                    }
                }
                *psp = 1;
            }
            else
            {
                *psp = 0;
            }

            break;
        case SEM:
            semaphoreInfo = (SEMAPHORE_INFO *) r0;

            generateSramSrdMasks(srdMask, (void *)semaphoreInfo, sizeof(*semaphoreInfo));
            ok = verifyAccess(srdMask, tcb[taskCurrent].srd);

            if(r1 > MAX_SEMAPHORES - 1)
                ok = false;

            if(ok)
            {
                semaphoreInfo->count = semaphores[r1].count;
                strcpy(semaphoreInfo->name, semaphores[r1].name);
                semaphoreInfo->numWaiters = semaphores[r1].queueSize;
                if(semaphores[r1].queueSize > 0)
                {
                    for(i = 0; i < semaphores[r1].queueSize; i++)
                    {
                        strcpy(semaphoreInfo->waiters[i], tcb[semaphores[r1].processQueue[i]].name);
                    }
                }
                *psp = 1;
            }
            else
            {
                *psp = 0;
            }
            break;
        case PS:
            taskInfo = (TASK_INFO *) r0;
            generateSramSrdMasks(srdMask, (void *)taskInfo, sizeof(*taskInfo));
            ok = verifyAccess(srdMask, tcb[taskCurrent].srd);

            if(r1 > MAX_TASKS)
                ok = false;

            if(ok && r1 == MAX_TASKS)
            {
                strcpy(taskInfo->name, "Kernel\0");
                taskInfo->pid = 0;
                taskInfo->state = STATE_READY;

                strcpy(taskInfo->cpuUsage, iftoa((PERIOD_CLKS - clkSum) * 2500, 9, 2, buf));
                *psp = 1;

            }
            else if(ok)
            {
                strcpy(taskInfo->name, tcb[r1].name);
                taskInfo->pid = (uint32_t)tcb[r1].pid;
                taskInfo->state = tcb[r1].state;
                taskInfo->ticks = tcb[r1].ticks;

                if(tcb[r1].clocks[clockCurrent ^ 1] > 0)
                {
                    // avoids fp math since period is 1000000 clks
                    // shift decimal 8 times to account for div by 1000000 and 2 extra from prescaling by 10000
                    strcpy(taskInfo->cpuUsage, iftoa((uint64_t)tcb[r1].clocks[clockCurrent ^ 1] * 2500, 9, 2, buf));
                }
                else
                    strcpy(taskInfo->cpuUsage, "00.00\0");


                *psp = 1;
            }
            else
            {
                *psp = 0;
            }

            break;
        case RUN:
            restartThread((_fn)r0);
            break;
        case KILL:
            stopThread((_fn)r0);
            break;
        case PREEMPT_EN:
            preemption = true;
            break;
        case PREEMPT_DIS:
            preemption = false;
            break;
        case SCHED_PRIO:
            priorityScheduler = true;
            break;
        case SCHED_RR:
            priorityScheduler = false;
            break;
        case SET_PRIO:
            setThreadPriority((_fn)r0, r1);
            break;

    }
}

