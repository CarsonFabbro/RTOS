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

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "gpio.h"
#include "shell.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "asm.h"
#include "string.h"
#include "kernel.h"

// REQUIRED: Add header files here for your strings functions, ...

// Data Restrictions and Struct
#define MAX_CHARS 80
#define MAX_FIELDS 6
#define BUF_SIZE 32

typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;


//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Get String From Uart Function
void getsUart0(USER_DATA* data)
{
    uint16_t count = 0;
    char c;

    while(true)
    {
        // Get character from uart
        c = getcUart0();

        // If character is backspace, and string is not empty (count > 0), delete character (count --)
        if((c == 8 || c == 127) && count > 0)
        {
            count--;
        }

        // If character is line feed or carriage return, add null terminator & return
        else if(c == 10 || c == 13)
        {
            data->buffer[count] = '\0';
            return;
        }

        // If character is a space or printable, then store it in data, and increment count. If count then equals maxChars,
        // add null and return
        else if(c >= 32)
        {
            data->buffer[count] = c;
            count++;

            if(count == MAX_CHARS)
            {
                data->buffer[count] = '\0';
                return;
            }
        }
    }
}

// Parse the fields in the buffer
void parseFields(USER_DATA* data)
{
    uint16_t count = 0;

    // Loop through entire buffer until 5 fields are parsed, or null is found
    while(data->fieldCount < MAX_FIELDS)
    {
        // Loop through delimeters until alpha or numeric is found (ascii 45-46 are - . and 48-57 are numbers, and 65-90 & 97-122
        // are alphas)
        while(((data->buffer[count] != 45 && data->buffer[count] != 46) && ((data->buffer[count] < 48) || ((data->buffer[count] >57) &&
                (data->buffer[count] < 65)) || ((data->buffer[count] > 90) && (data->buffer[count] < 97)) || data->buffer[count] > 122)))
        {
            if(data->buffer[count] == '\0')
                return;

            data->buffer[count++] = NULL;
        }

        // Since has to be either alpha or numeric at this point, if less than 57 it is a numeric, else it is alpha
        if(data->buffer[count] <= 57)
        {
            data->fieldType[data->fieldCount] = 'n';
        }
        else
        {
            data->fieldType[data->fieldCount] = 'a';
        }

        // Set beginning of field in the position array
        data->fieldPosition[data->fieldCount] = count;

        // While in the field loop through each character until a delimeter is found
        while((data->buffer[count] == 45 || data->buffer[count] == 46) || (data->buffer[count] >= 48 && data->buffer[count] <= 57) ||
                (data->buffer[count] >= 65 && data->buffer[count] <= 90) || (data->buffer[count] >= 97 && data->buffer[count] <= 122))
        {
            count++;
        }

        // If last field, set final delimeter equal to NULL
        if(data->fieldCount + 1 == MAX_FIELDS)
        {
            data->buffer[count] = NULL;
        }

        // Increment field count
        data->fieldCount = data->fieldCount + 1;
    }

    return;
}


// Get the string at a given field index, returns char* on success, and NULL otherwise
char* getFieldString(USER_DATA* data, uint8_t fieldNumber)
{
    // If field number is within range, return the string in the requested field
    if(fieldNumber <= MAX_FIELDS - 1)
    {
        return &(data->buffer[data->fieldPosition[fieldNumber]]);
    }
    return NULL;
}

// Get an integer at the requested field index, returns the number parsed on succes, and 0 otherwise
int32_t getFieldInteger(USER_DATA *data, uint8_t fieldNumber)
{
    if(fieldNumber <= MAX_FIELDS - 1 && data->fieldType[fieldNumber] == 'n')
    {
        return (int32_t) atoi(getFieldString(data, fieldNumber));
    }
    return 0;
}

// Checks if the command given matches a given command, returns true if the command matches, and false otherwise
bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments)
{
    uint8_t i = 0;

    // If the number of arguments is less than the required arguments, return false
    if(data->fieldCount - 1 < minArguments)
    {
        return false;
    }

    // Loop through the strings, and check if each character is equivalent (strcmp)
    while(strCommand[i] != NULL || data->buffer[i + data->fieldPosition[0]] != NULL)
    {
        if(strCommand[i] == data->buffer[i + data->fieldPosition[0]])
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

// Clear all fields
void clearFields(USER_DATA* data)
{
    uint8_t i;
    // Clear buffer of \n so next input can be read
    getcUart0();

    // Clear all fields in data
    for(i = 0; i < MAX_CHARS; i++)
    {
        data->buffer[i] = 0;
    }
    for(i = 0; i < MAX_FIELDS; i++)
    {
        data->fieldPosition[i] = 0;
        data->fieldType[i] = 0;
    }
    data->fieldCount = 0;
}

void ps()
{

    TASK_INFO taskTable;
    uint8_t i;
    uint8_t ok;
    char str[BUF_SIZE] = {0};

    putsUart0("--------------- TASKS ---------------\n");
    for(i = 0; i < MAX_TASKS; i++)
    {
        ok = getTaskInfo((void *)&taskTable, i);
        if(ok == 0)
        {
            putsUart0("ERROR: Attempting to access illegal memory address\n");
        }

        if(taskTable.state != STATE_INVALID)
        {
            putsUart0(taskTable.name);
            putsUart0("\n\t");

            putsUart0("Pid: ");
            putsUart0(itoa(taskTable.pid, str));
            putsUart0("\n\t");

            putsUart0("State: ");
            if(taskTable.state == STATE_DELAYED)
            {
                putsUart0("Sleep for ");
                putsUart0(itoa(taskTable.ticks, str));
                putsUart0(" ms\n\t");
            }
            else if(taskTable.state == STATE_BLOCKED_MUTEX)
            {
                putsUart0("Blocked by Mutex");
                putsUart0("\n\t");
            }
            else if(taskTable.state == STATE_BLOCKED_SEMAPHORE)
            {
                putsUart0("Blocked by Semaphore");
                putsUart0("\n\t");
            }
            else if(taskTable.state == STATE_UNRUN)
            {
                putsUart0("Unrun");
                putsUart0("\n\t");
            }
            else if(taskTable.state == STATE_READY)
            {
                putsUart0("Ready");
                putsUart0("\n\t");
            }
            else if(taskTable.state == STATE_STOPPED)
            {
                putsUart0("Stopped");
                putsUart0("\n\t");
            }

            putsUart0("CPU Usage: ");
            putsUart0(taskTable.cpuUsage);
            putsUart0("%\n");
         }
    }

    getTaskInfo((void *)&taskTable, i);

    //Kernel
    putsUart0("Kernel");
    putsUart0("\n\t");

    putsUart0("Pid: ");
    putcUart0('-');
    putsUart0("\n\t");

    putsUart0("State: ");
    putcUart0('-');
    putsUart0("\n\t");

    putsUart0("CPU Usage: ");
    putsUart0(taskTable.cpuUsage);
    putsUart0("%\n");
}


void ipcs()
{
    MUTEX_INFO mutexTable;
    SEMAPHORE_INFO semaphoreTable;
    uint8_t i;
    char str[BUF_SIZE] = {0};
    uint8_t ok;

    putsUart0("--------------- MUTEXES ---------------\n");

    for(i = 0; i < MAX_MUTEXES; i++)
    {
        ok = getMutexInfo((void *)&mutexTable, i);
        if(ok == 0)
        {
            putsUart0("ERROR: Attempting to access illegal memory address\n");
        }

        putsUart0(mutexTable.name);
        putsUart0(" [");
        putsUart0(itoa(i, str));
        putsUart0("]\n\t");

        if(mutexTable.lock)
        {
            putsUart0("Locked By: ");
            putsUart0(mutexTable.lockedBy);
            putsUart0("\n\t");

            if(mutexTable.numWaiters > 0)
            {
                uint8_t j;

                putsUart0("Wait List:");

                for(j = 0; j < mutexTable.numWaiters; j++)
                {
                    putsUart0("\n\t\t");
                    putsUart0(mutexTable.waiters[j]);
                }
            }
            else
            {
                putsUart0("No wait list");
            }
        }
        else
        {
            putsUart0("Unlocked");
        }
        putcUart0('\n');

    }

    putsUart0("--------------- SEMAPHORES ---------------\n");

    for(i = 0; i < MAX_SEMAPHORES; i++)
    {
        getSemaphoreInfo((void *)&semaphoreTable, i);
        if(ok == 0)
        {
            putsUart0("ERROR: Attempting to access illegal memory address\n");
        }

        putsUart0(semaphoreTable.name);
        putsUart0(" [");
        putsUart0(itoa(i, str));
        putsUart0("]\n\t");

        putsUart0("Count: ");
        putsUart0(itoa(semaphoreTable.count, str));
        putsUart0("\n\t");

        if(semaphoreTable.numWaiters > 0)
        {
            uint8_t j;

            putsUart0("Wait List:");

            for(j = 0; j < semaphoreTable.numWaiters; j++)
            {
                putsUart0("\n\t\t");
                putsUart0(semaphoreTable.waiters[j]);
            }
        }
        else
        {
            putsUart0("No wait list");
        }

        putcUart0('\n');
    }
}

void kill(uint32_t pid)
{
    char str[BUF_SIZE] = {0};
    killThread(pid);
    putsUart0(itoa(pid, str));
    putsUart0(" killed\n");
}

void pkill(char* proc_name)
{
    killThread(getPid(proc_name));
    putsUart0(proc_name);
    putsUart0(" killed\n");
}

void preempt(bool on)
{
    if(on)
    {
        enablePreemption();
        putsUart0("preempt on\n");
    }
    else
    {
        disablePreemption();
        putsUart0("preempt off\n");
    }
}

void sched(bool prio_on)
{
    if(prio_on)
    {
        setSchedPriority();
        putsUart0("sched prio\n");
    }
    else
    {
        setSchedRoundRobin();
        putsUart0("sched rr\n");
    }
}

void pidof(const char name[])
{
    uint32_t pid = getPid(name);
    char str[BUF_SIZE] = {0};

    if(pid > 0)
        putsUart0(itoa(pid, str));
    else
    {
        putsUart0(name);
        putsUart0(" does not exist...");
    }
    putcUart0('\n');
}

void run(const char name[])
{
    runThread(getPid(name));
    putsUart0(name);
    putsUart0(" launched\n");
}


// REQUIRED: add processing for the shell commands through the UART here
void shell(void)
{
    USER_DATA data;
    data.fieldCount = 0;
    bool valid = false;

    while(true)
    {
        if(kbhitUart0())
        {

            // Get string from uart and store in data
            getsUart0(&data);
            parseFields(&data);

            // Command evaluation //

            // reboot: Reboots the device
            if(isCommand(&data, "reboot", 0))
            {
                valid = true;
                clearFields(&data);
                NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
            }

            // ps: Displays the process (thread) status
            else if(isCommand(&data, "ps", 0))
            {
                ps();
                valid = true;
            }

            // ipcs: Displays the inter-process (thread) communication status
            else if(isCommand(&data, "ipcs", 0))
            {
                ipcs();
                valid = true;
            }

            // kill [PID]: Kills the process (thread) with the matching PID
            else if(isCommand(&data, "kill", 1))
            {
                kill(getFieldInteger(&data, 1));
                valid = true;
            }

            // Pkill [proc_name]: Kills the process by name
            else if(isCommand(&data, "pkill", 1))
            {
                pkill(getFieldString(&data, 1));
                valid = true;
            }

            // preempt ON | OFF: Turns preemption on or off
            else if(isCommand(&data, "preempt", 1))
            {
                char* str = getFieldString(&data, 1);

                if(strcmp(str, "on"))
                {
                    preempt(true);
                    valid = true;
                }
                else if(strcmp(str, "off"))
                {
                    preempt(false);
                    valid = true;
                }
            }

            // sched PRIO | RR: Selected priority or round-robin scheduling
            else if(isCommand(&data, "sched", 1))
            {
                char* str = getFieldString(&data, 1);

                if(strcmp(str, "prio"))
                {
                    sched(true);
                    valid = true;
                }
                else if(strcmp(str, "rr"))
                {
                    sched(false);
                    valid = true;
                }

            }

            // pidof proc_name: Displays the PID of the process
            else if(isCommand(&data, "pidof", 1))
            {
                pidof(getFieldString(&data, 1));
                valid = true;
            }

            // run proc_name: Runs the selected program in the background
            else if(isCommand(&data, "run", 1))
            {
                run(getFieldString(&data, 1));
                valid = true;
            }

            // Look for error
            if(!valid)
                putsUart0("Invalid command\n");

            valid = false;
            clearFields(&data);

        }
        yield();
    }
}
