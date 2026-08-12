#ifndef ADS1115_H_
#define ADS1115_H_

#define ADS1115_BATTERY_ADDRESS 0x48
#define ADS1115_SYSTEM_ADDRESS  0x49

#define ADS1115_CHANNEL_A0 0
#define ADS1115_CHANNEL_A1 1
#define ADS1115_CHANNEL_A2 2
#define ADS1115_CHANNEL_A3 3

/*
 * Performs one single-ended conversion on the selected
 * channel of the ADS1115 at the specified I2C address.
 *
 * Returns:
 *     1 = successful
 *     0 = invalid argument or I2C failure
 */
unsigned char ADS1115_ReadChannel(unsigned char address,
                                  unsigned char channel,
                                  signed int *result);

#endif
