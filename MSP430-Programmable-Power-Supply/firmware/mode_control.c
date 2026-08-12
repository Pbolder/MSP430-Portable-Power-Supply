/*
 * mode_control.c
 *
 * Processes operating-mode and enable-button requests.
 */

#include "mode_control.h"
#include "inputs.h"
#include "gpio.h"
#include "safety.h"

#define CHARGE_WARNING_HALF_PERIOD_TICKS 10U
#define CHARGE_WARNING_TRANSITIONS       10U

volatile unsigned char g_buckRequested = 0;
volatile unsigned char g_buckEnabled = 0;
volatile unsigned char g_chargeEnableWarningActive = 0;
volatile unsigned char g_chargeEnableWarningVisible = 1;

static unsigned char s_chargeWarningTicks = 0;
static unsigned char s_chargeWarningTransitions = 0;
static unsigned char s_displayRefreshEvent = 0;

static unsigned char VoltageSelectionIsValid(void)
{
    return (g_outputVoltageSelection == OUTPUT_VOLTAGE_3V3) ||
           (g_outputVoltageSelection == OUTPUT_VOLTAGE_5V) ||
           (g_outputVoltageSelection == OUTPUT_VOLTAGE_9V);
}

static void StopChargeWarning(void)
{
    g_chargeEnableWarningActive = 0;
    g_chargeEnableWarningVisible = 1;
    s_chargeWarningTicks = 0;
    s_chargeWarningTransitions = 0;
}

static void StartChargeWarning(void)
{
    g_chargeEnableWarningActive = 1;
    g_chargeEnableWarningVisible = 1;
    s_chargeWarningTicks = 0;
    s_chargeWarningTransitions = 0;
    s_displayRefreshEvent = 1;
    ButtonLED_On();
    StatusLED_On();
}

static void UpdateChargeWarning10ms(void)
{
    if (!g_chargeEnableWarningActive)
    {
        return;
    }

    s_chargeWarningTicks++;

    if (s_chargeWarningTicks < CHARGE_WARNING_HALF_PERIOD_TICKS)
    {
        return;
    }

    s_chargeWarningTicks = 0;
    g_chargeEnableWarningVisible ^= 1U;
    s_chargeWarningTransitions++;
    s_displayRefreshEvent = 1;

    if (s_chargeWarningTransitions >= CHARGE_WARNING_TRANSITIONS)
    {
        /*
         * End with the normal charging indicator visible and
         * the enable-button LED off.
         */
        StopChargeWarning();
        ButtonLED_Off();
        StatusLED_Off();
        return;
    }

    if (g_chargeEnableWarningVisible)
    {
        ButtonLED_On();
        StatusLED_On();
    }
    else
    {
        ButtonLED_Off();
        StatusLED_Off();
    }
}

void ModeControl_Init(void)
{
    g_buckRequested = 0;
    g_buckEnabled = 0;
    StopChargeWarning();
    s_displayRefreshEvent = 0;
    Buck_Disable();
    ButtonLED_Off();
    StatusLED_Off();
}

void ModeControl_Update10ms(void)
{
    unsigned char enablePress;

    enablePress = Inputs_TakeEnablePressEvent();

    if (g_operatingMode == OPERATING_MODE_CHARGE)
    {
        g_buckRequested = 0;
        Buck_Disable();
        g_buckEnabled = 0;

        /*
         * A press in CHARGE mode is rejected. The buck remains
         * off while the button LED and charging indicator flash.
         */
        if (enablePress)
        {
            StartChargeWarning();
        }

        UpdateChargeWarning10ms();
        return;
    }

    if (g_operatingMode != OPERATING_MODE_OUTPUT)
    {
        g_buckRequested = 0;
        Buck_Disable();
        g_buckEnabled = 0;
        StopChargeWarning();
        ButtonLED_Off();
        StatusLED_Off();
        return;
    }
    else{
        StatusLED_On();
    }

    StopChargeWarning();

if (!Safety_IsBuckPermitted())
{
    /*
     * Clear the request so cooling or overcurrent recovery
     * cannot restart the buck automatically.
     */
    g_buckRequested = 0U;
    Buck_Disable();
    g_buckEnabled = 0U;
    ButtonLED_Off();
    return;
}
if (g_buckRequested &&
    VoltageSelectionIsValid() &&
    Safety_IsBuckPermitted())
{
    Buck_Enable();
}
else
{
    Buck_Disable();
}

    /*
     * Toggle the request once for each debounced button press.
     */
    if (enablePress)
    {
        if (g_buckRequested)
        {
            g_buckRequested = 0;
        }
        else if (VoltageSelectionIsValid())
        {
            g_buckRequested = 1;
        }
    }

    /*
     * Never retain an enable request through an invalid or
     * disconnected rotary-switch position. This preserves the
     * required select-voltage-then-enable sequence.
     */
    if (!VoltageSelectionIsValid())
    {
        g_buckRequested = 0;
    }

    /*
     * The buck can run only in OUTPUT mode with exactly one
     * valid rotary-switch voltage selected. Future protection
     * checks belong in this same permission decision.
     */
    if (g_buckRequested && VoltageSelectionIsValid())
    {
        Buck_Enable();
    }
    else
    {
        Buck_Disable();
    }

    g_buckEnabled = Buck_IsEnabled();

    if (g_buckEnabled)
    {
        ButtonLED_On();
    }
    else
    {
        ButtonLED_Off();
    }
}

unsigned char ModeControl_TakeDisplayRefreshEvent(void)
{
    unsigned char event;

    event = s_displayRefreshEvent;
    s_displayRefreshEvent = 0;
    return event;
}
