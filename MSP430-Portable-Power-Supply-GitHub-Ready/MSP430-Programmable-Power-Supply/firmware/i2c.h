/*
 * i2c.h
 *
 * MSP430G2553 USCI_B0 I2C master interface.
 */
#ifndef I2C_H_
#define I2C_H_

void I2C_Init(void);

unsigned char I2C_Probe(unsigned char address);

unsigned char I2C_WriteBytes(unsigned char address,
                             const unsigned char *data,
                             unsigned int length);

unsigned char I2C_WriteRegister16(unsigned char address,
                                  unsigned char registerAddress,
                                  unsigned int data);

unsigned char I2C_ReadRegister16(unsigned char address,
                                 unsigned char registerAddress,
                                 unsigned int *data);

#endif
