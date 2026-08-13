/*
 * Paul Bolder
 * inputs.c
 *
 * Decodes and debounces the mode and voltage-selection switches.
 */

#include "inputs.h"
#include "gpio.h"

#define INPUT_DEBOUNCE_SAMPLES 5U

/*
 * Public debounced input states.
 */
volatile OperatingMode g_operatingMode = OPERATING_MODE_INVALID;

volatile OutputVoltageSelection g_outputVoltageSelection =
    OUTPUT_VOLTAGE_INVALID;

volatile unsigned char g_enableButtonPressed = 0;

/*
 * Operating-mode debounce state.
 */
static OperatingMode s_modeCandidate = OPERATING_MODE_INVALID;
static unsigned char s_modeCandidateCount = 0;

/*
 * Voltage-selection debounce state.
 */
static OutputVoltageSelection s_voltageCandidate =
    OUTPUT_VOLTAGE_INVALID;

static unsigned char s_voltageCandidateCount = 0;

/*
 * Enable-button debounce state.
 */
static unsigned char s_enableCandidate = 0;
static unsigned char s_enableCandidateCount = 0;
static unsigned char s_enablePressEvent = 0;
static unsigned char s_enableArmed = 0;

static OperatingMode ReadRawOperatingMode(void)
{
    unsigned char buckAllowed;
    unsigned char chargeAllowed;

    buckAllowed = Buck_IsAllowed();
    chargeAllowed = Charge_IsAllowed();

    /*
     * Both permission signals low means OFF.
     */
    if ((!buckAllowed) && (!chargeAllowed))
    {
        return OPERATING_MODE_OFF;
    }

    /*
     * Only CHARGE_ALLOWED high means charge mode.
     */
    if ((!buckAllowed) && chargeAllowed)
    {
        return OPERATING_MODE_CHARGE;
    }

    /*
     * Only BUCK_ALLOWED high means output mode.
     */
    if (buckAllowed && (!chargeAllowed))
    {
        return OPERATING_MODE_OUTPUT;
    }

    /*
     * Both signals high is not a valid switch position.
     */
    return OPERATING_MODE_INVALID;
}
static OutputVoltageSelection ReadRawVoltageSelection(void)
{
    unsigned char select3V3;
    unsigned char select5V;
    unsigned char select9V;

    select3V3 = Voltage3V3_IsSelected();
    select5V = Voltage5V_IsSelected();
    select9V = Voltage9V_IsSelected();

    /*
     * No voltage-selection signal is currently active.
     */
    if ((!select3V3) && (!select5V) && (!select9V))
    {
        return OUTPUT_VOLTAGE_NONE;
    }

    /*
     * Accept a voltage only when exactly one signal is high.
     */
    if (select3V3 && (!select5V) && (!select9V))
    {
        return OUTPUT_VOLTAGE_3V3;
    }

    if ((!select3V3) && select5V && (!select9V))
    {
        return OUTPUT_VOLTAGE_5V;
    }

    if ((!select3V3) && (!select5V) && select9V)
    {
        return OUTPUT_VOLTAGE_9V;
    }

    /*
     * Multiple active selection signals are invalid.
     */
    return OUTPUT_VOLTAGE_INVALID;
}

void Inputs_Init(void)
{
    /*
     * The inputs must remain stable for the complete debounce
     * period before becoming valid application states.
     */
    g_operatingMode = OPERATING_MODE_INVALID;
    g_outputVoltageSelection = OUTPUT_VOLTAGE_INVALID;

    s_modeCandidate = ReadRawOperatingMode();
    s_modeCandidateCount = 0;

    s_voltageCandidate = ReadRawVoltageSelection();
    s_voltageCandidateCount = 0;

/*
 * Capture the initial button condition 
 */
s_enableCandidate = EnableButton_IsPressed();
g_enableButtonPressed = s_enableCandidate;
s_enableCandidateCount = 0;
s_enablePressEvent = 0;

/*
 * If the button starts released, it is ready to detect a press.
 * If it starts pressed, it must be released first.
 */
if (g_enableButtonPressed)
{
    s_enableArmed = 0;
}
else
{
    s_enableArmed = 1;
}
}

void Inputs_Update10ms(void)
{
    OperatingMode rawMode;
    OutputVoltageSelection rawVoltage;

    rawMode = ReadRawOperatingMode();
    rawVoltage = ReadRawVoltageSelection();

    /*
     * Debounce the operating-mode switch.
     */
    if (rawMode != s_modeCandidate)
    {
        /*
         * The reading changed, so begin testing the new value.
         */
        s_modeCandidate = rawMode;
        s_modeCandidateCount = 1;
    }
    else
    {
        if (s_modeCandidateCount < INPUT_DEBOUNCE_SAMPLES)
        {
            s_modeCandidateCount++;
        }

        if (s_modeCandidateCount >= INPUT_DEBOUNCE_SAMPLES)
        {
            g_operatingMode = s_modeCandidate;
        }
    }

    /*
     * Debounce the voltage-selection switch independently.
     */
    if (rawVoltage != s_voltageCandidate)
    {
        s_voltageCandidate = rawVoltage;
        s_voltageCandidateCount = 1;
    }
    else
    {
        if (s_voltageCandidateCount < INPUT_DEBOUNCE_SAMPLES)
        {
            s_voltageCandidateCount++;
        }

        if (s_voltageCandidateCount >= INPUT_DEBOUNCE_SAMPLES)
        {
            g_outputVoltageSelection = s_voltageCandidate;
        }
    }



/*
 * Debounce the ENABLE button.
 */
{
    unsigned char rawEnableButton;

    rawEnableButton = EnableButton_IsPressed();

    if (rawEnableButton != s_enableCandidate)
    {
        /*
         * The raw state changed. Begin testing the new state.
         */
        s_enableCandidate = rawEnableButton;
        s_enableCandidateCount = 1;
    }
    else
    {
        if (s_enableCandidateCount < INPUT_DEBOUNCE_SAMPLES)
        {
            s_enableCandidateCount++;
        }

        /*
         * Accept the new button state after five identical
         * samples, representing approximately 50 ms.
         */
        if ((s_enableCandidateCount >= INPUT_DEBOUNCE_SAMPLES) &&
            (g_enableButtonPressed != s_enableCandidate))
        {
            g_enableButtonPressed = s_enableCandidate;

            if (g_enableButtonPressed)
            {
                /*
                 * Generate one event on the pressed transition,
                 * but only if a release previously armed it.
                 */
                if (s_enableArmed)
                {
                    s_enablePressEvent = 1;
                    s_enableArmed = 0;
                }
            }
            else
            {
                /*
                 * A confirmed release arms the next press.
                 */
                s_enableArmed = 1;
            }
        }
    }
}


}

unsigned char Inputs_TakeEnablePressEvent(void)
{
    unsigned char event;

    event = s_enablePressEvent;
    s_enablePressEvent = 0;

    return event;
}

