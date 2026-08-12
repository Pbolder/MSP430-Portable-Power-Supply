#ifndef SAFETY_H_
#define SAFETY_H_

/*
 * Initial protection thresholds.  Temperatures use 0.1 degree C and
 * current uses mA so they can be compared directly with g_measurements.
 */
#define HIGH_TEMPERATURE_TRIP_DECI_C     550
#define HIGH_TEMPERATURE_RESET_DECI_C    450
#define BUCK_OVERCURRENT_TRIP_MA        2700L
#define BUCK_OVERCURRENT_RESET_MA        500L

extern volatile unsigned char g_highTemperatureLockout;
extern volatile unsigned char g_overcurrentLockout;
extern volatile unsigned char g_highTemperatureWarningVisible;

void Safety_Init(void);
void Safety_Update100ms(unsigned char buckCurrentSampleValid);

/* Returns 1 only when neither requested safety lockout is active. */
unsigned char Safety_IsBuckPermitted(void);

/* Returns and clears the OLED-refresh request. */
unsigned char Safety_TakeDisplayRefreshEvent(void);

#endif /* SAFETY_H_ */
