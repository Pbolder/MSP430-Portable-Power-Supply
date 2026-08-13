/*
 * clock.h
 *
 * System clock configuration.
 */
 #ifndef CLOCK_H_
 #define CLOCK_H_

 /* configure MCLK and SMCLK for 1 MHz
 * 
 * Returns:
  1 if calibration valid 0 if not
 */

 unsigned char Clock_Init1MHz(void);

 #endif /* CLOCK_H_*/
