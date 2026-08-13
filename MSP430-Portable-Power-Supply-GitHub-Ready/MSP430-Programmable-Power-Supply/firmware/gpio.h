/*
 * gpio.h
 *
 * GPIO definitions for the MSP430 battery and buck controller.
 */
#ifndef GPIO_H_
#define GPIO_H_

#include <msp430.h>




/* Port 1 assignments */
#define BUCK_ENABLE_PIN       BIT0    /* P1.0: MSP_BUCK_ENABLE */
#define BUTTON_LED_PIN        BIT1    /* P1.1: LED-, active-low */
#define STATUS_LED_PIN        BIT2    /* P1.2: active-high */
#define BUCK_ALLOWED_PIN      BIT3    /* P1.3: physical switch input active-high */
#define CHARGE_ALLOWED_PIN    BIT4    /* P1.4: physical switch input active-high */
#define I2C_SCL_PIN           BIT6    /* P1.6 */
#define I2C_SDA_PIN           BIT7    /* P1.7 */

/* Port 2 assignments */
#define ENABLE_BUTTON_PIN     BIT0    /* P2.0: active-low */

/* Menu and auxiliary inputs reserved for possible future use. */
#define MENU_PIN              BIT1    /* P2.1: active-low button */
#define AUX_PIN               BIT2    /* P2.2: active-low button */

#define VSEL_3V3_PIN          BIT3    /* P2.3: active-high */
#define VSEL_5V_PIN           BIT4    /* P2.4: active-high */
#define VSEL_9V_PIN           BIT5    /* P2.5: active-high */


/* GPIO initialization */
void GPIO_Init(void);

/* Buck-control functions */
void Buck_Disable(void);
void Buck_Enable(void);
unsigned char Buck_IsEnabled(void);

/* LED functions */
void StatusLED_Off(void);
void StatusLED_On(void);
void StatusLED_Toggle(void);

void ButtonLED_Off(void);
void ButtonLED_On(void);

/* Input-reading functions */
unsigned char Buck_IsAllowed(void);
unsigned char Charge_IsAllowed(void);
unsigned char EnableButton_IsPressed(void);

unsigned char Voltage3V3_IsSelected(void);
unsigned char Voltage5V_IsSelected(void);
unsigned char Voltage9V_IsSelected(void);

#endif /* GPIO_H_ */

