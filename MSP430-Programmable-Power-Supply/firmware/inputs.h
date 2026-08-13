/*
 * inputs.h
 *
 * Physical control input decoding and debouncing.
 */

#ifndef INPUTS_H_
#define INPUTS_H_

/* system input/output mode options*/
typedef enum
{
    OPERATING_MODE_OFF = 0,
    OPERATING_MODE_CHARGE = 1,
    OPERATING_MODE_OUTPUT = 2,
    OPERATING_MODE_INVALID = 3
} OperatingMode;


/* voltage output options*/
typedef enum
{
    OUTPUT_VOLTAGE_NONE = 0,
    OUTPUT_VOLTAGE_3V3 = 1,
    OUTPUT_VOLTAGE_5V = 2,
    OUTPUT_VOLTAGE_9V = 3,
    OUTPUT_VOLTAGE_INVALID = 4
} OutputVoltageSelection;

/*
 * These globals are temporarily exposed so they can be inspected
 * in the CCS Watch view.
 */
extern volatile OperatingMode g_operatingMode;
extern volatile OutputVoltageSelection g_outputVoltageSelection;

void Inputs_Init(void);
void Inputs_Update10ms(void);

/*
 * Current stable state of the enable button:
 *     0 = released
 *     1 = pressed
 */
extern volatile unsigned char g_enableButtonPressed;

/*
 * Returns 1 once for each debounced button press.
 * Reading the event clears it.
 */
unsigned char Inputs_TakeEnablePressEvent(void);

#endif /* INPUTS_H_ */

