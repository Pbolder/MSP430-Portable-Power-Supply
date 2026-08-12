#ifndef MEASUREMENTS_H_
#define MEASUREMENTS_H_

typedef struct
{
    unsigned char batteryAdsPresent;
    unsigned char systemAdsPresent;
    unsigned char batteryTemperatureValid;

    signed int chargeCurrentRaw;
    signed int cell3Raw;
    signed int cell2Raw;
    signed int cell1Raw;

    signed int voutRaw;
    signed int buckCurrentRaw;
    signed int temperatureRaw;
    signed int batteryTemperatureDeciC;
    signed int batteryCurrentRaw;

    signed long chargeCurrentMilliamps;

    signed long cell1Millivolts;
    signed long cell2Millivolts;
    signed long cell3Millivolts;
    signed long packMillivolts;

    signed long voutMillivolts;
    signed long buckCurrentMilliamps;

    signed long temperatureAdcMillivolts;
    signed long thermistorOhms;

    signed long batteryCurrentMilliamps;

} MeasurementData;

extern volatile MeasurementData g_measurements;

/*
 * Nominal zero-current ADC codes.
 * These will be replaced with measured calibration values later.
 */
extern volatile signed int g_chargeCurrentZeroRaw;
extern volatile signed int g_batteryCurrentZeroRaw;

/*
 * Reads both ADS1115 modules and updates g_measurements.
 *
 * Returns:
 *     1 = both modules read successfully
 *     0 = at least one module failed
 */
unsigned char Measurements_Update(void);
unsigned char Measurements_UpdateBuckCurrent(void);
#endif
