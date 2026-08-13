/*
 * Paul Bolder
 * clock.c
 *
 * MSP430G2553 clock configuration.
 */

#include <msp430.h>
#include "clock.h"

unsigned char Clock_Init1MHz(void)
{
    /*
    * The factory stores calibrated DCO constants in info memory
    * a falue of 0xFF indicates that the calibration info is unavailable
    */
   if ((CALBC1_1MHZ == 0xFF) ||
        (CALDCO_1MHZ == 0xFF))
    {
        return 0;
    }

    /*
     * Temporarily clear the DCO control register before loading
     * the calibrated range and modulation values.
     */
    DCOCTL = 0;

    /*
     * Load the factory calibration constants for 1 MHz.
     */
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;

    /*
     * Use the DCO for both MCLK and SMCLK with no division.
     *
     * MCLK  = 1 MHz
     * SMCLK = 1 MHz
     */
    BCSCTL2 = 0;

    return 1;
}

