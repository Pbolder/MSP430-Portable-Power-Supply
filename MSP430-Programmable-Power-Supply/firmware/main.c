/*
 * Paul Bolder
 * main.c
 *
 * MSP430 Battery and Buck Converter Controller
 * Initial fail-safe GPIO configuration
 */

#include <msp430.h>

#include "clock.h"
#include "gpio.h"
#include "timer.h"
#include "inputs.h"
#include "mode_control.h"
#include "i2c.h"
#include "ads1115.h"
#include "measurements.h"
#include "ssd1309.h"
#include "display.h"
#include "safety.h"

/*
 * Communication status:
 *     1 = all four channels read successfully
 *     0 = at least one channel failed
 */
volatile unsigned char g_batteryAdsPresent = 0;
volatile unsigned char g_systemAdsPresent = 0;

/*
 * Battery ADS1115, address 0x48.
 */
volatile signed int g_batteryRawA0 = 0;
volatile signed int g_batteryRawA1 = 0;
volatile signed int g_batteryRawA2 = 0;
volatile signed int g_batteryRawA3 = 0;

volatile signed long g_batteryMvA0 = 0;
volatile signed long g_batteryMvA1 = 0;
volatile signed long g_batteryMvA2 = 0;
volatile signed long g_batteryMvA3 = 0;

/*
 * System ADS1115, address 0x49.
 */
volatile signed int g_systemRawA0 = 0;
volatile signed int g_systemRawA1 = 0;
volatile signed int g_systemRawA2 = 0;
volatile signed int g_systemRawA3 = 0;

volatile signed long g_systemMvA0 = 0;
volatile signed long g_systemMvA1 = 0;
volatile signed long g_systemMvA2 = 0;
volatile signed long g_systemMvA3 = 0;

/* OLED address 0x3C status. */
volatile unsigned char g_oled3CPresent = 0;
volatile unsigned char g_oledTestPassed = 0;
volatile unsigned char g_displayUpdatePassed = 0;


int main(void)
{
    /*
     * Stop the watchdog during initial development.
     * It will be re-enabled later as a safety feature.
     */
    WDTCTL = WDTPW | WDTHOLD;


    /*
    * configure GPIO
    * MSP_BUCK_ENABLE low 
    */
    GPIO_Init();
    Buck_Disable(); /* Buck off during startup. */
    Inputs_Init();  /* Initialize switch inputs. */
    ModeControl_Init();
    Safety_Init();

    /* Set MCLK and SMCLK to 1 MHz. */
    if (!Clock_Init1MHz())
    {
        /*
         * Missing clock calibration is treated as a startup
         * failure. Keep the buck disabled.
         */
        while (1)
        {
            Buck_Disable();
        }
    }

    /*
    * P1.6 and P1.7 become the shared I2C bus.
    */
    I2C_Init();

    g_oled3CPresent = I2C_Probe(SSD1309_ADDRESS);

    if (g_oled3CPresent && SSD1309_Init())
    {
        g_oledTestPassed = 1U;
        Display_Init();
    }
    else
    {
        g_oledTestPassed = 0U;
    }



     /*
     * Configure Timer_A0 for a 1 ms interrupt.
     */
    Timer_Init();

    /*
     * Globally enable maskable interrupts.
     *
         */
    __enable_interrupt();

 while (1)
    {
        /*
         * Future use:
         * - button sampling
         * - switch debouncing
         */
        if (g_task10ms)
        {
            g_task10ms = 0;
        /*
        * Sample and debounce all physical control inputs.
        */
        Inputs_Update10ms();
        ModeControl_Update10ms();

        /*
         * Refresh immediately when a charge-warning flash
         * changes state. This keeps the plug, CHARGING text,
         * and enable-button LED visually synchronized without
         * blocking the scheduler.
         */
        if (g_oledTestPassed &&
            ModeControl_TakeDisplayRefreshEvent())
        {
            g_displayUpdatePassed = Display_Update();
        }
        }

        /*
        
         * - sensor measurements
         * - safety evaluation
         */
       if (g_task100ms)
{
    g_task100ms = 0;

    Safety_Update100ms(
        Measurements_UpdateBuckCurrent());

    if (Safety_TakeDisplayRefreshEvent())
    {
        if (g_oledTestPassed)
        {
            g_displayUpdatePassed = Display_Update();
        }
    }
}

        /*
         * Current use:
         * - scheduler verification
         * - status heartbeat
         */


         if (g_task500ms)
        {
            g_task500ms = 0;

            (void)Measurements_Update();

            if (g_oledTestPassed)
            {
                g_displayUpdatePassed = Display_Update();
            }

            g_heartbeatCount++;
            
        }
        

    }
}

