/*
 * mode_control.h
 *
 * Handles button requests for buck operation.
 */

#ifndef MODE_CONTROL_H_
#define MODE_CONTROL_H_

/*
 * User's debounced buck-enable request:
 *     0 = buck not requested
 *     1 = buck requested
 *
 * does not yet mean that the physical buck is enabled.
 */
extern volatile unsigned char g_buckRequested;

/*
 * Actual MSP430 buck-enable command:
 *     0 = P1.0 low, buck disabled
 *     1 = P1.0 high, buck enabled
 *
 * This is deliberately separate from g_buckRequested.
 */
extern volatile unsigned char g_buckEnabled;

/*
 * CHARGE-mode rejected-enable warning state.
 * The visible flag is shared by the OLED indicator and
 * the active-low enable-button LED.
 */
extern volatile unsigned char g_chargeEnableWarningActive;
extern volatile unsigned char g_chargeEnableWarningVisible;

void ModeControl_Init(void);
void ModeControl_Update10ms(void);

/*
 * Returns 1 once whenever the charge-warning indicator changes
 * and the OLED should be refreshed. Reading clears the event.
 */
unsigned char ModeControl_TakeDisplayRefreshEvent(void);

#endif /* MODE_CONTROL_H_ */
