/*
 * i2c.c
 *
 * Polling-based MSP430G2553 I2C master driver.
 */

#include <msp430.h>
#include "i2c.h"

/*
 * Prevent a hardware fault or disconnected bus from permanently
 * trapping the application inside an I2C polling loop.
 */
#define I2C_TIMEOUT_COUNT 10000U

static unsigned char I2C_WaitForStop(void)
{
    unsigned int timeout;

    timeout = I2C_TIMEOUT_COUNT;

    while (UCB0CTL1 & UCTXSTP)
    {
        if (timeout == 0)
        {
            return 0;
        }

        timeout--;
    }

    return 1;
}

void I2C_Init(void)
{
    /*
     * Hold USCI_B0 in reset while it is configured.
     */
    UCB0CTL1 = UCSWRST;

    /*
     * UCMST:    master mode
     * UCMODE_3: I2C mode
     * UCSYNC:   synchronous operation
     */
    UCB0CTL0 = UCMST | UCMODE_3 | UCSYNC;

    /*
     * Select SMCLK as the I2C peripheral clock.
     */
    UCB0CTL1 = UCSWRST | UCSSEL_2;

    /*
     * SMCLK = 1 MHz
     *
     *     1,000,000 / 10 = 100,000 Hz
     */
    UCB0BR0 = 10;
    UCB0BR1 = 0;

/*
 * Configure P1.6 and P1.7 for USCI_B0 I2C.
 *
 * P1.6 = UCB0SCL
 * P1.7 = UCB0SDA
 */
P1REN &= ~(BIT6 | BIT7);   /* Disable internal resistors */
P1OUT |=  (BIT6 | BIT7);   /* Safe state if resistors are later enabled */
P1DIR |=  (BIT6 | BIT7);   /* Required for USCI_B0 function */
P1SEL |=  (BIT6 | BIT7);
P1SEL2 |= (BIT6 | BIT7);

    /*
     * Clear any stale NACK indication.
     */
    UCB0STAT &= ~UCNACKIFG;

    /*
     * Release USCI_B0 from reset.
     */
    UCB0CTL1 &= ~UCSWRST;
}


unsigned char I2C_Probe(unsigned char address)
{
    unsigned int timeout;

    if (address > 0x7F)
    {
        return 0;
    }

    /*
     * Do not start while the bus is already busy.
     */
    timeout = I2C_TIMEOUT_COUNT;

    while (UCB0STAT & UCBBUSY)
    {
        if (timeout == 0)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    UCB0I2CSA = address;
    UCB0STAT &= ~UCNACKIFG;
    IFG2 &= ~UCB0TXIFG;

    /*
     * Begin an ordinary master-transmitter transaction.
     */
    UCB0CTL1 |= UCTR;
    UCB0CTL1 |= UCTXSTT;

    /*
     * Wait until the USCI is ready for the first data byte,
     * or until the address is rejected.
     */
    timeout = I2C_TIMEOUT_COUNT;

    while (!(IFG2 & UCB0TXIFG))
    {
        if (UCB0STAT & UCNACKIFG)
        {
            UCB0CTL1 |= UCTXSTP;
            (void)I2C_WaitForStop();
            UCB0STAT &= ~UCNACKIFG;
            return 0;
        }

        if (timeout == 0)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    /*
     * ADS1115 pointer byte:
     * 0x00 selects the conversion register.
     */
    UCB0TXBUF = 0x00;

    /*
     * Wait for the pointer byte to finish.
     */
    timeout = I2C_TIMEOUT_COUNT;

    while (!(IFG2 & UCB0TXIFG))
    {
        if (UCB0STAT & UCNACKIFG)
        {
            UCB0CTL1 |= UCTXSTP;
            (void)I2C_WaitForStop();
            UCB0STAT &= ~UCNACKIFG;
            return 0;
        }

        if (timeout == 0)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    if (UCB0STAT & UCNACKIFG)
    {
        UCB0CTL1 |= UCTXSTP;
        (void)I2C_WaitForStop();
        UCB0STAT &= ~UCNACKIFG;
        return 0;
    }

    /*
     * Pointer byte was acknowledged.
     */
    UCB0CTL1 |= UCTXSTP;

    if (!I2C_WaitForStop())
    {
        I2C_Init();
        return 0;
    }

    return 1;
}


unsigned char I2C_WriteRegister16(unsigned char address,
                                  unsigned char registerAddress,
                                  unsigned int data)
{
    unsigned int timeout;

    if (address > 0x7F)
    {
        return 0;
    }

    /*
     * Wait for any previous transaction to finish.
     */
    timeout = I2C_TIMEOUT_COUNT;

    while (UCB0STAT & UCBBUSY)
    {
        if (timeout == 0)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    UCB0I2CSA = address;
    UCB0STAT &= ~UCNACKIFG;
    IFG2 &= ~UCB0TXIFG;

    /*
     * Begin transmitter transaction.
     */
    UCB0CTL1 |= UCTR;
    UCB0CTL1 |= UCTXSTT;

    timeout = I2C_TIMEOUT_COUNT;

    while (!(IFG2 & UCB0TXIFG))
    {
        if (UCB0STAT & UCNACKIFG)
        {
            UCB0CTL1 |= UCTXSTP;
            (void)I2C_WaitForStop();
            UCB0STAT &= ~UCNACKIFG;
            return 0;
        }

        if (timeout == 0)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    /*
     * Send the ADS1115 register-pointer byte.
     */
    UCB0TXBUF = registerAddress;

    timeout = I2C_TIMEOUT_COUNT;

    while (!(IFG2 & UCB0TXIFG))
    {
        if (UCB0STAT & UCNACKIFG)
        {
            UCB0CTL1 |= UCTXSTP;
            (void)I2C_WaitForStop();
            UCB0STAT &= ~UCNACKIFG;
            return 0;
        }

        if (timeout == 0)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    /*
     * ADS1115 sends and receives the most-significant byte first.
     */
    UCB0TXBUF = (unsigned char)(data >> 8);

    timeout = I2C_TIMEOUT_COUNT;

    while (!(IFG2 & UCB0TXIFG))
    {
        if (UCB0STAT & UCNACKIFG)
        {
            UCB0CTL1 |= UCTXSTP;
            (void)I2C_WaitForStop();
            UCB0STAT &= ~UCNACKIFG;
            return 0;
        }

        if (timeout == 0)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    UCB0TXBUF = (unsigned char)(data & 0x00FF);

    timeout = I2C_TIMEOUT_COUNT;

    while (!(IFG2 & UCB0TXIFG))
    {
        if (UCB0STAT & UCNACKIFG)
        {
            UCB0CTL1 |= UCTXSTP;
            (void)I2C_WaitForStop();
            UCB0STAT &= ~UCNACKIFG;
            return 0;
        }

        if (timeout == 0)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    if (UCB0STAT & UCNACKIFG)
    {
        UCB0CTL1 |= UCTXSTP;
        (void)I2C_WaitForStop();
        UCB0STAT &= ~UCNACKIFG;
        return 0;
    }

    UCB0CTL1 |= UCTXSTP;

    if (!I2C_WaitForStop())
    {
        I2C_Init();
        return 0;
    }

    return 1;
}

unsigned char I2C_ReadRegister16(unsigned char address,
                                 unsigned char registerAddress,
                                 unsigned int *data)
{
    unsigned int timeout;
    unsigned char highByte;
    unsigned char lowByte;

    if ((address > 0x7F) || (data == 0))
    {
        return 0;
    }

    /*
     * Wait for any previous transaction.
     */
    timeout = I2C_TIMEOUT_COUNT;

    while (UCB0STAT & UCBBUSY)
    {
        if (timeout == 0)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    UCB0I2CSA = address;
    UCB0STAT &= ~UCNACKIFG;
    IFG2 &= ~(UCB0TXIFG | UCB0RXIFG);

    /*
     * Transmit the register pointer.
     */
    UCB0CTL1 |= UCTR;
    UCB0CTL1 |= UCTXSTT;

    timeout = I2C_TIMEOUT_COUNT;

    while (!(IFG2 & UCB0TXIFG))
    {
        if (UCB0STAT & UCNACKIFG)
        {
            UCB0CTL1 |= UCTXSTP;
            (void)I2C_WaitForStop();
            UCB0STAT &= ~UCNACKIFG;
            return 0;
        }

        if (timeout == 0)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    UCB0TXBUF = registerAddress;

    timeout = I2C_TIMEOUT_COUNT;

    while (!(IFG2 & UCB0TXIFG))
    {
        if (UCB0STAT & UCNACKIFG)
        {
            UCB0CTL1 |= UCTXSTP;
            (void)I2C_WaitForStop();
            UCB0STAT &= ~UCNACKIFG;
            return 0;
        }

        if (timeout == 0)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    /*
     * Issue a repeated START in receiver mode.
     */
    UCB0CTL1 &= ~UCTR;
    UCB0CTL1 |= UCTXSTT;

    timeout = I2C_TIMEOUT_COUNT;

    while (UCB0CTL1 & UCTXSTT)
    {
        if (UCB0STAT & UCNACKIFG)
        {
            UCB0CTL1 |= UCTXSTP;
            (void)I2C_WaitForStop();
            UCB0STAT &= ~UCNACKIFG;
            return 0;
        }

        if (timeout == 0)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    /*
     * Receive the first byte.
     */
    timeout = I2C_TIMEOUT_COUNT;

    while (!(IFG2 & UCB0RXIFG))
    {
        if (timeout == 0)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    highByte = UCB0RXBUF;

    /*
     * Request STOP before receiving the final byte so that
     * the controller NACKs the second byte correctly.
     */
    UCB0CTL1 |= UCTXSTP;

    timeout = I2C_TIMEOUT_COUNT;

    while (!(IFG2 & UCB0RXIFG))
    {
        if (timeout == 0)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    lowByte = UCB0RXBUF;

    if (!I2C_WaitForStop())
    {
        I2C_Init();
        return 0;
    }

    *data = ((unsigned int)highByte << 8) | lowByte;

    return 1;
}


unsigned char I2C_WriteBytes(unsigned char address,
                             const unsigned char *data,
                             unsigned int length)
{
    unsigned int timeout;
    unsigned int index;

    if ((address > 0x7F) ||
        ((data == 0) && (length != 0U)))
    {
        return 0;
    }

    if (length == 0U)
    {
        return 1;
    }

    /*
     * Wait for any previous transaction to finish.
     */
    timeout = I2C_TIMEOUT_COUNT;

    while (UCB0STAT & UCBBUSY)
    {
        if (timeout == 0U)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    UCB0I2CSA = address;
    UCB0STAT &= ~UCNACKIFG;
    IFG2 &= ~UCB0TXIFG;

    /*
     * Enter transmitter mode and issue START.
     */
    UCB0CTL1 |= UCTR;
    UCB0CTL1 |= UCTXSTT;

    for (index = 0U; index < length; index++)
    {
        timeout = I2C_TIMEOUT_COUNT;

        while (!(IFG2 & UCB0TXIFG))
        {
            if (UCB0STAT & UCNACKIFG)
            {
                UCB0CTL1 |= UCTXSTP;
                (void)I2C_WaitForStop();
                UCB0STAT &= ~UCNACKIFG;
                return 0;
            }

            if (timeout == 0U)
            {
                I2C_Init();
                return 0;
            }

            timeout--;
        }

        UCB0TXBUF = data[index];
    }

    /*
     * Wait until the final byte leaves UCB0TXBUF.
     */
    timeout = I2C_TIMEOUT_COUNT;

    while (!(IFG2 & UCB0TXIFG))
    {
        if (UCB0STAT & UCNACKIFG)
        {
            UCB0CTL1 |= UCTXSTP;
            (void)I2C_WaitForStop();
            UCB0STAT &= ~UCNACKIFG;
            return 0;
        }

        if (timeout == 0U)
        {
            I2C_Init();
            return 0;
        }

        timeout--;
    }

    if (UCB0STAT & UCNACKIFG)
    {
        UCB0CTL1 |= UCTXSTP;
        (void)I2C_WaitForStop();
        UCB0STAT &= ~UCNACKIFG;
        return 0;
    }

    UCB0CTL1 |= UCTXSTP;

    if (!I2C_WaitForStop())
    {
        I2C_Init();
        return 0;
    }

    return 1;
}

