/*
 * gpio.c
 *
 * Low-level GPIO configuration and control.
 */

#include "gpio.h"

void GPIO_Init(void)
{
    /*
    * Set  output values 
    *
    * P1.0 low: buck disabled
    * P1.1 high: active-low button LED off
    * P1.2 low: status LED off
    */

    P1OUT &= ~(BUCK_ENABLE_PIN | STATUS_LED_PIN);
    P1OUT |= BUTTON_LED_PIN;

    /*
    * set signals as outputs.
    */
    P1DIR |= BUCK_ENABLE_PIN |
            BUTTON_LED_PIN |
            STATUS_LED_PIN;

    /*
    * BUCK_ALLOWED and CHARGE_ALLOWED inputs from the
    * physical mode switch & have external
    * 100-kohm pulldown resistors.
    * Internal pulldowns for defined behavior during testing.
    */
    P1DIR &= ~(CHARGE_ALLOWED_PIN | BUCK_ALLOWED_PIN);
    P1REN |= CHARGE_ALLOWED_PIN | BUCK_ALLOWED_PIN;
    P1OUT &= ~(CHARGE_ALLOWED_PIN | BUCK_ALLOWED_PIN );

    /*
    * P1.5  unused. Held it low 
    */
    P1DIR &= ~BIT5;
    P1REN |= BIT5;
    P1OUT &= ~BIT5;

    /*
    * I2C pins as GPIO inputs with pulldowns
    * until the I2C driver takes control.
    */
    P1DIR &= ~(I2C_SCL_PIN | I2C_SDA_PIN);
    P1REN |= I2C_SCL_PIN | I2C_SDA_PIN;
    P1OUT &= ~(I2C_SCL_PIN | I2C_SDA_PIN);

    /*
    * ENABLE is an active-low button, so use an internal pull-up.
    */
    P2DIR &= ~ENABLE_BUTTON_PIN;
    P2REN |= ENABLE_BUTTON_PIN;
    P2OUT |= ENABLE_BUTTON_PIN;

    /*
    * MENU and AUX do not have assigned functions yet.
    */
    P2DIR &= ~(MENU_PIN | AUX_PIN);
    P2REN |= MENU_PIN | AUX_PIN;
    P2OUT |= MENU_PIN | AUX_PIN;

    /*
     * Voltage-selection signals from the panel-mounted
     * rotary switch and are active-high. Internal pulldowns
     * supplement the external 100-kohm pulldowns.
     */
    P2DIR &= ~(VSEL_3V3_PIN | VSEL_5V_PIN | VSEL_9V_PIN);
    P2REN |= VSEL_3V3_PIN | VSEL_5V_PIN | VSEL_9V_PIN;
    P2OUT &= ~(VSEL_3V3_PIN | VSEL_5V_PIN | VSEL_9V_PIN);    

    /*
     * P2.6 and P2.7 unused.
     */
    P2DIR &= ~(BIT6 | BIT7);
    P2REN |= BIT6 | BIT7;
    P2OUT &= ~(BIT6 | BIT7);
}    

void Buck_Disable(void)
{
    P1OUT &= ~BUCK_ENABLE_PIN;
}

void Buck_Enable(void)
{
    /* check for physical switch enable permission
    * also enforced by hardware transistor circut
    */
    if (Buck_IsAllowed())
    {
        P1OUT |= BUCK_ENABLE_PIN;

    } else {
    Buck_Disable();
    }

}

unsigned char Buck_IsEnabled(void)
{
    /*
     * Read the output latch, not the external permission input.
     * This reports whether the MSP430 is actually commanding
     * the buck on.
     */
    return (P1OUT & BUCK_ENABLE_PIN) != 0U;
}


void StatusLED_Off(void)
{
    P1OUT &= ~STATUS_LED_PIN;
}

void StatusLED_On(void)
{
    P1OUT |= STATUS_LED_PIN;
}

void StatusLED_Toggle(void)
{
    P1OUT ^= STATUS_LED_PIN;
}

void ButtonLED_Off(void)
{
    /*
     * This LED is connected to 3.3 V and controlled by
     * sinking current through P1.1, so high means off.
     */
    P1OUT |= BUTTON_LED_PIN;
}

void ButtonLED_On(void)
{
    P1OUT &= ~BUTTON_LED_PIN;
}

/*buck allowed bool*/
unsigned char Buck_IsAllowed(void)
{
    return (P1IN & BUCK_ALLOWED_PIN) !=0;
}
/*charge allowed bool*/
unsigned char Charge_IsAllowed(void)
{
    return (P1IN & CHARGE_ALLOWED_PIN) !=0;
}


unsigned char EnableButton_IsPressed(void)
{
    /*
     * Active-low button:
     *     low  = pressed
     *     high = released
     */
    return (P2IN & ENABLE_BUTTON_PIN) == 0;
}

unsigned char Voltage3V3_IsSelected(void)
{
    return (P2IN & VSEL_3V3_PIN) != 0;
}

unsigned char Voltage5V_IsSelected(void)
{
    return (P2IN & VSEL_5V_PIN) != 0;
}

unsigned char Voltage9V_IsSelected(void)
{
    return (P2IN & VSEL_9V_PIN) != 0;
}
