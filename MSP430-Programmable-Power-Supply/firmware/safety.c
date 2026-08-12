/*
 * safety.c
 *
 * Temperature and buck-output overcurrent protection.
 */

#include "safety.h"
#include "gpio.h"
#include "measurements.h"

#define TEMPERATURE_FLASH_TICKS          5U
#define OVERCURRENT_RESET_SAMPLES       10U
#define OVERCURRENT_TRIP_SAMPLES      3U

static unsigned char s_overcurrentTripSamples = 0U;
volatile unsigned char g_highTemperatureLockout = 0U;
volatile unsigned char g_overcurrentLockout = 0U;
volatile unsigned char g_highTemperatureWarningVisible = 1U;

static unsigned char s_temperatureFlashTicks = 0U;
static unsigned char s_overcurrentResetSamples = 0U;
static unsigned char s_displayRefreshEvent = 0U;

static void TripHighTemperature(void)
{
    g_highTemperatureLockout = 1U;
    g_highTemperatureWarningVisible = 1U;
    s_temperatureFlashTicks = 0U;
    s_displayRefreshEvent = 1U;
    Buck_Disable();
}

static void TripOvercurrent(void)
{
    g_overcurrentLockout = 1U;
    s_overcurrentResetSamples = 0U;
    s_displayRefreshEvent = 1U;
    Buck_Disable();
}

void Safety_Init(void)
{
    g_highTemperatureLockout = 0U;
    g_overcurrentLockout = 0U;
    g_highTemperatureWarningVisible = 1U;
    s_temperatureFlashTicks = 0U;
    s_overcurrentResetSamples = 0U;
    s_displayRefreshEvent = 0U;
}

void Safety_Update100ms(unsigned char buckCurrentSampleValid)
{
    /*
     * Temperature hysteresis prevents rapid on/off cycling near the
     * threshold.  A valid reading is required to trip or clear it.
     */
    if (g_measurements.systemAdsPresent &&
        g_measurements.batteryTemperatureValid)
    {
        if (!g_highTemperatureLockout &&
            (g_measurements.batteryTemperatureDeciC >=
             HIGH_TEMPERATURE_TRIP_DECI_C))
        {
            TripHighTemperature();
        }
        else if (g_highTemperatureLockout &&
                 (g_measurements.batteryTemperatureDeciC <=
                  HIGH_TEMPERATURE_RESET_DECI_C))
        {
            g_highTemperatureLockout = 0U;
            g_highTemperatureWarningVisible = 1U;
            s_temperatureFlashTicks = 0U;
            s_displayRefreshEvent = 1U;
        }
    }

    if (g_highTemperatureLockout)
    {
        s_temperatureFlashTicks++;

        if (s_temperatureFlashTicks >= TEMPERATURE_FLASH_TICKS)
        {
            s_temperatureFlashTicks = 0U;
            g_highTemperatureWarningVisible ^= 1U;
            s_displayRefreshEvent = 1U;
        }
    }

    /*
     * Three consecutive samples at or above 2.7 A trip the output.
     * After shutdown, measured current must remain below 0.5 A for one
     * second before another manual enable request is accepted.
     */
    if (!buckCurrentSampleValid)
{
    s_overcurrentTripSamples = 0U;

    if (g_overcurrentLockout)
    {
        s_overcurrentResetSamples = 0U;
    }
}
else if (!g_overcurrentLockout)
{
    s_overcurrentResetSamples = 0U;

    if (Buck_IsEnabled() &&
        (g_measurements.buckCurrentMilliamps >=
         BUCK_OVERCURRENT_TRIP_MA))
    {
        if (s_overcurrentTripSamples <
            OVERCURRENT_TRIP_SAMPLES)
        {
            s_overcurrentTripSamples++;
        }

        if (s_overcurrentTripSamples >=
            OVERCURRENT_TRIP_SAMPLES)
        {
            TripOvercurrent();
        }
    }
    else
    {
        s_overcurrentTripSamples = 0U;
    }
}
else
{
    s_overcurrentTripSamples = 0U;

    if (g_measurements.buckCurrentMilliamps <=
        BUCK_OVERCURRENT_RESET_MA)
    {
        if (s_overcurrentResetSamples <
            OVERCURRENT_RESET_SAMPLES)
        {
            s_overcurrentResetSamples++;
        }

        if (s_overcurrentResetSamples >=
            OVERCURRENT_RESET_SAMPLES)
        {
            g_overcurrentLockout = 0U;
            s_overcurrentResetSamples = 0U;
            s_displayRefreshEvent = 1U;
        }
    }
    else
    {
        s_overcurrentResetSamples = 0U;
    }
}
}

unsigned char Safety_IsBuckPermitted(void)
{
    return (!g_highTemperatureLockout &&
            !g_overcurrentLockout) ? 1U : 0U;
}

unsigned char Safety_TakeDisplayRefreshEvent(void)
{
    unsigned char event;

    event = s_displayRefreshEvent;
    s_displayRefreshEvent = 0U;
    return event;
}
