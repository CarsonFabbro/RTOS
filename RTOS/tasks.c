// Tasks
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
#include "gpio.h"
#include "wait.h"
#include "kernel.h"
// get rid of these later
#include "uart0.h"
#include "asm.h"
#include "string.h"
//
#include "tasks.h"

#define BLUE_LED   PORTF,2 // on-board blue LED
#define RED_LED    PORTC,6 // of-board red LED
#define ORANGE_LED PORTC,5 // off-board orange LED
#define YELLOW_LED PORTC,4 // off-board yellow LED
#define GREEN_LED  PORTB,3 // off-board green LED

#define PB_1 PORTA,2
#define PB_2 PORTA,3
#define PB_3 PORTA,4
#define PB_4 PORTB,6
#define PB_5 PORTB,7
#define PB_6 PORTB,2

#define NUM_BUTTONS 6

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
// REQUIRED: Add initialization for blue, orange, red, green, and yellow LEDs
//           Add initialization for 6 pushbuttons
void initHw(void)
{
    // Setup LEDs and pushbuttons
    // Enable Clocks
    enablePort(PORTA);
    enablePort(PORTB);
    enablePort(PORTC);
    enablePort(PORTF);

    // Configure pins
    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(ORANGE_LED);
    selectPinPushPullOutput(YELLOW_LED);
    selectPinPushPullOutput(GREEN_LED);
    selectPinPushPullOutput(BLUE_LED);

    setPinValue(RED_LED, 0);
    setPinValue(ORANGE_LED, 0);
    setPinValue(YELLOW_LED, 0);
    setPinValue(GREEN_LED, 0);
    setPinValue(BLUE_LED, 0);

    selectPinDigitalInput(PB_1);
    enablePinPullup(PB_1);
    selectPinDigitalInput(PB_2);
    enablePinPullup(PB_2);
    selectPinDigitalInput(PB_3);
    enablePinPullup(PB_3);
    selectPinDigitalInput(PB_4);
    enablePinPullup(PB_4);
    selectPinDigitalInput(PB_5);
    enablePinPullup(PB_5);
    selectPinDigitalInput(PB_6);
    enablePinPullup(PB_6);

    // Power-up flash
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(250000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(250000);
}

// REQUIRED: add code to return a value from 0-63 indicating which of 6 PBs are pressed
uint8_t readPbs(void)
{
    int8_t buttons = 0;

    if (!getPinValue(PB_1)) buttons |= 1;
    if (!getPinValue(PB_2)) buttons |= 2;
    if (!getPinValue(PB_3)) buttons |= 4;
    if (!getPinValue(PB_4)) buttons |= 8;
    if (!getPinValue(PB_5)) buttons |= 16;
    if (!getPinValue(PB_6)) buttons |= 32;

    return buttons;
}

// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle(void)
{
    while(true)
    {
        setPinValue(ORANGE_LED, 1);
        waitMicrosecond(1000);
        setPinValue(ORANGE_LED, 0);
        yield();
    }
}

// For step 8
void idle2(void)
{
    while(true)
    {
        setPinValue(YELLOW_LED, 1);
        waitMicrosecond(1000);
        setPinValue(YELLOW_LED, 0);
        yield();
    }
}

void flash4Hz(void)
{
    while(true)
    {
        setPinValue(GREEN_LED, !getPinValue(GREEN_LED));
        sleep(125);
    }
}

void oneshot(void)
{
    while(true)
    {
        wait(flashReq);
        setPinValue(YELLOW_LED, 1);
        sleep(1000);
        setPinValue(YELLOW_LED, 0);
    }
}

void partOfLengthyFn(void)
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}

void lengthyFn(void)
{
    uint16_t i;
    while(true)
    {
        lock(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
        }
        setPinValue(RED_LED, !getPinValue(RED_LED));
        unlock(resource);
    }
}

void readKeys(void)
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((buttons & 1) != 0)
        {
            setPinValue(YELLOW_LED, !getPinValue(YELLOW_LED));
            setPinValue(RED_LED, 1);
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq);
            setPinValue(RED_LED, 0);
        }
        if ((buttons & 4) != 0)
        {
            runThread((uint32_t)flash4Hz);
        }
        if ((buttons & 8) != 0)
        {
            killThread((uint32_t)flash4Hz);
        }
        if ((buttons & 16) != 0)
        {
            changeThreadPriority((uint32_t)lengthyFn, 4);
        }
        yield();
    }
}

void debounce(void)
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() == 0)
                count--;
            else
                count = 10;
        }
        post(keyReleased);
    }
}

void uncooperative(void)
{
    while(true)
    {
        while (readPbs() == 8)
        {
        }
        yield();
    }
}

void errant(void)
{
    uint32_t* p = (uint32_t*)0x20000000;
    while(true)
    {
        while (readPbs() == 32)
        {
            *p = 0;
        }
        yield();
    }
}

void important(void)
{
    while(true)
    {
        lock(resource);
        setPinValue(BLUE_LED, 1);
        sleep(1000);
        setPinValue(BLUE_LED, 0);
        unlock(resource);
    }
}
