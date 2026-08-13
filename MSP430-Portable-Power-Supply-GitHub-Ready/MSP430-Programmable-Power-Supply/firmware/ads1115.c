/*
 * ads1115.c
 *
 * ADS1115 single-ended channel driver.
 */

#include <msp430.h>

#include "ads1115.h"
#include "i2c.h"



/*
 * Configuration bits shared by all four channels:
 *
 * OS   = 1: Start single conversion
 * PGA  = 001: +/-4.096 V full-scale range
 * MODE = 1: Single-shot mode
 * DR   = 100: 128 samples per second
 * COMP = 11: Comparator disabled
 *
 * The channel-selection MUX bits are added separately.
 */
#define ADS1115_CONFIG_COMMON 0x8383
#define ADS1115_REG_CONVERSION 0x00
#define ADS1115_REG_CONFIG     0x01



unsigned char ADS1115_ReadChannel(unsigned char address,
                                  unsigned char channel,
                                  signed int *result)
{
    unsigned int muxBits;
    unsigned int configuration;
    unsigned int conversion;

    if (result == 0)
    {
        return 0;
    }

    /*
     * Only accept valid ADS1115 addresses.
     */
    if ((address < 0x48) || (address > 0x4B))
    {
        return 0;
    }

    /*
     * Select a single-ended input relative to GND.
     */
    switch (channel)
    {
        case ADS1115_CHANNEL_A0:
            muxBits = 0x4000;      /* MUX = 100 */
            break;

        case ADS1115_CHANNEL_A1:
            muxBits = 0x5000;      /* MUX = 101 */
            break;

        case ADS1115_CHANNEL_A2:
            muxBits = 0x6000;      /* MUX = 110 */
            break;

        case ADS1115_CHANNEL_A3:
            muxBits = 0x7000;      /* MUX = 111 */
            break;

        default:
            return 0;
    }

    configuration = ADS1115_CONFIG_COMMON | muxBits;

    /*
     * Configure the selected ADS1115 and begin conversion.
     */
    if (!I2C_WriteRegister16(address,
                             ADS1115_REG_CONFIG,
                             configuration))
    {
        return 0;
    }

    /*
     * Conversion takes about 7.8 ms at 128 samples/second.
     * This provides a 10 ms delay with a 1 MHz CPU clock.
     */
    __delay_cycles(10000);

    if (!I2C_ReadRegister16(address,
                            ADS1115_REG_CONVERSION,
                            &conversion))
    {
        return 0;
    }

    *result = (signed int)conversion;

    return 1;
}
