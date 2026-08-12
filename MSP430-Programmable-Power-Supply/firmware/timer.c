/*
 * timer.c
 *
 * Provides a 1 ms Timer_A0 interrupt and periodic task flags.
 */

#include <msp430.h>
#include "timer.h"

/*
 * Shared scheduler flags.
 *
 * volatile tells the compiler that values can change
 * unexpectedly inside an interrupt.
 */
volatile unsigned char g_task10ms = 0;
volatile unsigned char g_task100ms = 0;
volatile unsigned char g_task500ms = 0;

volatile unsigned int g_heartbeatCount = 0;

void Timer_Init(void)
{
    /*
     * Stop and clear Timer_A0 before configuring it.
     */
    TA0CTL = TACLR;

    /*
     * SMCLK = 1,000,000 Hz
     *
     * One timer period:
     *
     *     1,000 timer counts = 1 ms
     *
     * Timer counts from 0 through CCR0, inclusive, so:
     *
     *     CCR0 = 1,000 - 1 = 999
     */
    TA0CCR0 = 999;

    /*
     * Enable the CCR0 interrupt.
     */
    TA0CCTL0 = CCIE;

    /*
     * TASSEL_2: SMCLK
     * MC_1:     count in up mode
     * TACLR:    clear the timer before starting
     */
    TA0CTL = TASSEL_2 | MC_1 | TACLR;
}

/*
 * Timer_A0 CCR0 interrupt service routine.
 *
 * runs once every millisecond.
 */
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0_ISR(void)
{
    static unsigned char count10ms = 0;
    static unsigned char count100ms = 0;
    static unsigned char count500ms = 0;

    /*
     * Generate the first scheduler event every 10 ms.
     */
    count10ms++;

    if (count10ms >= 10)
    {
        count10ms = 0;
        g_task10ms = 1;

        /*
         * counters advance every 10 ms instead of every
         * 1 ms to keep interrupt small.
         */
        count100ms++;
        count500ms++;

        if (count100ms >= 10)
        {
            count100ms = 0;
            g_task100ms = 1;
        }

        if (count500ms >= 50)
        {
            count500ms = 0;
            g_task500ms = 1;
        }
    }
 }


