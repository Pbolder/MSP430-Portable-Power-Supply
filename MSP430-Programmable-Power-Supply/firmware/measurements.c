/*
 * measurements.c
 *
 * Converts the eight ADS1115 channels into correct units.
 */

#include "measurements.h"
#include "ads1115.h"

#define LOGIC_SUPPLY_MV             3300L
#define THERMISTOR_FIXED_OHMS      10000L

/*
 * Nominal sensor offsets:
 *
 * ACS70331-2P5U3:
 *     0.250 V / 0.125 mV per count = 2000 counts
 *
 * ACS758-050U at 3.3 V:
 *     0.600 V * (3.3 / 5.0) = 0.396 V
 *     0.396 V / 0.125 mV per count = 3168 counts
 */
volatile signed int g_chargeCurrentZeroRaw = 2000;
volatile signed int g_batteryCurrentZeroRaw = 3168;

volatile MeasurementData g_measurements = {0};

static signed long RawToMillivolts(signed int raw)
{
    signed long microvolts;

    microvolts = (signed long)raw * 125L;

    if (microvolts >= 0)
    {
        return (microvolts + 500L) / 1000L;
    }

    return (microvolts - 500L) / 1000L;
}

unsigned char Measurements_UpdateBuckCurrent(void)
{
    signed int buckCurrentRaw;

    if (!ADS1115_ReadChannel(ADS1115_SYSTEM_ADDRESS,
                             ADS1115_CHANNEL_A1,
                             &buckCurrentRaw))
    {
        return 0U;
    }

    g_measurements.buckCurrentRaw = buckCurrentRaw;

    /*
     * Shunt and amplifier produce 1 V/A, so the ADC
     * millivolt value numerically equals milliamps.
     */
    g_measurements.buckCurrentMilliamps =
        RawToMillivolts(buckCurrentRaw);

    return 1U;
}

static unsigned char ReadBatteryMeasurements(void)
{
    signed int chargeRaw;
    signed int cell3Raw;
    signed int cell2Raw;
    signed int cell1Raw;

    signed long cell3AdcMv;
    signed long cell2AdcMv;
    signed long cell1AdcMv;

    signed long cell1TapMv;
    signed long cell2TapMv;
    signed long cell3TapMv;

    signed long chargeDeltaRaw;

    /*
     * Battery ADS1115, address 0x48:
     *
     * A0 = CHG_CURRENT_ADC
     * A1 = ADC_CELL3
     * A2 = ADC_CELL2
     * A3 = ADC_CELL1
     */
    if (!ADS1115_ReadChannel(ADS1115_BATTERY_ADDRESS,
                             ADS1115_CHANNEL_A0,
                             &chargeRaw))
    {
        g_measurements.batteryAdsPresent = 0;
        return 0;
    }

    if (!ADS1115_ReadChannel(ADS1115_BATTERY_ADDRESS,
                             ADS1115_CHANNEL_A1,
                             &cell3Raw))
    {
        g_measurements.batteryAdsPresent = 0;
        return 0;
    }

    if (!ADS1115_ReadChannel(ADS1115_BATTERY_ADDRESS,
                             ADS1115_CHANNEL_A2,
                             &cell2Raw))
    {
        g_measurements.batteryAdsPresent = 0;
        return 0;
    }

    if (!ADS1115_ReadChannel(ADS1115_BATTERY_ADDRESS,
                             ADS1115_CHANNEL_A3,
                             &cell1Raw))
    {
        g_measurements.batteryAdsPresent = 0;
        return 0;
    }

    g_measurements.chargeCurrentRaw = chargeRaw;
    g_measurements.cell3Raw = cell3Raw;
    g_measurements.cell2Raw = cell2Raw;
    g_measurements.cell1Raw = cell1Raw;

    /*
     * ACS70331EOLCTR-2P5U3:
     *
     * Zero-current output = 0.250 V nominal
     * Sensitivity = 800 mV/A
     *
     * One ADS1115 count = 0.125 mV
     *
     * mA = raw difference * 0.125 / 0.8
     *     = raw difference * 5 / 32
     */
    chargeDeltaRaw =
        (signed long)chargeRaw -
        (signed long)g_chargeCurrentZeroRaw;

    g_measurements.chargeCurrentMilliamps =
        (chargeDeltaRaw * 5L) / 32L;

    cell1AdcMv = RawToMillivolts(cell1Raw);
    cell2AdcMv = RawToMillivolts(cell2Raw);
    cell3AdcMv = RawToMillivolts(cell3Raw);

    /*
     * Cell 1 divider:
     *
     * Top = 330k
     * Bottom = 330k
     * Scale factor = 2
     */
    cell1TapMv = cell1AdcMv * 2L;

    /*
     * Cell 2 cumulative divider:
     *
     * Top = 680k
     * Bottom = 330k
     * Scale factor = 1010 / 330 = 101 / 33
     */
    cell2TapMv =
        ((cell2AdcMv * 101L) + 16L) / 33L;

    /*
     * Cell 3/pack cumulative divider:
     *
     * Top = 1.3M
     * Bottom = 330k
     * Scale factor = 1630 / 330 = 163 / 33
     */
    cell3TapMv =
        ((cell3AdcMv * 163L) + 16L) / 33L;

    /*
     * Divider measurements are cumulative:
     *
     * Tap 1 = cell 1
     * Tap 2 = cell 1 + cell 2
     * Tap 3 = cell 1 + cell 2 + cell 3
     */
    g_measurements.cell1Millivolts = cell1TapMv;
    g_measurements.cell2Millivolts =
        cell2TapMv - cell1TapMv;
    g_measurements.cell3Millivolts =
        cell3TapMv - cell2TapMv;
    g_measurements.packMillivolts = cell3TapMv;

    g_measurements.batteryAdsPresent = 1;

    return 1;
}


/*
 * Converts thermistor resistance to temperature using
 * the NXRT15XH103FA1B040 nominal characteristics:
 *
 * R25 = 10k ohms
 * B25/50 = 3380 K
 *
 * The table covers the thermistor's rated range in
 * 5-degree increments. Linear interpolation produces
 * temperature in tenths of a degree Celsius.
 */
static unsigned char ThermistorToDeciC(
    signed long resistanceOhms,
    signed int *temperatureDeciC)
{
    static const unsigned long resistanceTable[] =
    {
        235831UL, 173946UL, 129917UL, 98180UL,
         75022UL,  57926UL,  45168UL, 35548UL,
         28224UL,  22595UL,  18231UL, 14820UL,
         12133UL,  10000UL,   8295UL,  6922UL,
          5810UL,   4903UL,   4160UL,  3547UL,
          3039UL,   2616UL,   2261UL,  1963UL,
          1711UL,   1497UL,   1315UL,  1158UL,
          1024UL,    909UL,    809UL,   722UL,
           646UL,    580UL
    };

    unsigned char index;
    unsigned char tableLength;
    unsigned long resistance;
    unsigned long span;
    unsigned long position;

    if ((temperatureDeciC == 0) ||
        (resistanceOhms <= 0L))
    {
        return 0;
    }

    resistance = (unsigned long)resistanceOhms;

    tableLength =
        sizeof(resistanceTable) /
        sizeof(resistanceTable[0]);

    /*
     * Outside the thermistor's rated -40 to 125 C range.
     */
    if ((resistance > resistanceTable[0]) ||
        (resistance <
         resistanceTable[tableLength - 1]))
    {
        return 0;
    }

    for (index = 0;
         index < (tableLength - 1);
         index++)
    {
        if ((resistance <= resistanceTable[index]) &&
            (resistance >= resistanceTable[index + 1]))
        {
            span = resistanceTable[index] -
                   resistanceTable[index + 1];

            position =
                resistanceTable[index] - resistance;

            /*
             * Each table interval represents 5.0 C,
             * or 50 tenths of a degree.
             */
            *temperatureDeciC =
                (signed int)(-400 +
                ((signed int)index * 50) +
                (signed int)
                ((position * 50UL + span / 2UL) /
                 span));

            return 1;
        }
    }

    return 0;
}



static unsigned char ReadSystemMeasurements(void)
{
    signed int voutRaw;
    signed int buckCurrentRaw;
    signed int temperatureRaw;
    signed int batteryCurrentRaw;

    signed long voutAdcMv;
    signed long buckCurrentAdcMv;
    signed long temperatureAdcMv;
    signed long batteryDeltaRaw;

    /*
     * System ADS1115, address 0x49:
     *
     * A0 = VOUT_ADC
     * A1 = BUCK_CURRENT_ADC
     * A2 = TEMP_BAT
     * A3 = IBAT_ADC
     */
    if (!ADS1115_ReadChannel(ADS1115_SYSTEM_ADDRESS,
                             ADS1115_CHANNEL_A0,
                             &voutRaw))
    {
        g_measurements.systemAdsPresent = 0;
        return 0;
    }

    if (!ADS1115_ReadChannel(ADS1115_SYSTEM_ADDRESS,
                             ADS1115_CHANNEL_A1,
                             &buckCurrentRaw))
    {
        g_measurements.systemAdsPresent = 0;
        return 0;
    }

    if (!ADS1115_ReadChannel(ADS1115_SYSTEM_ADDRESS,
                             ADS1115_CHANNEL_A2,
                             &temperatureRaw))
    {
        g_measurements.systemAdsPresent = 0;
        return 0;
    }

    if (!ADS1115_ReadChannel(ADS1115_SYSTEM_ADDRESS,
                             ADS1115_CHANNEL_A3,
                             &batteryCurrentRaw))
    {
        g_measurements.systemAdsPresent = 0;
        return 0;
    }

    g_measurements.voutRaw = voutRaw;
    g_measurements.buckCurrentRaw = buckCurrentRaw;
    g_measurements.temperatureRaw = temperatureRaw;
    g_measurements.batteryCurrentRaw = batteryCurrentRaw;

    /*
     * VOUT divider:
     *
     * Top = 30k
     * Bottom = 10k
     * Scale factor = 4
     */
    voutAdcMv = RawToMillivolts(voutRaw);
    g_measurements.voutMillivolts =
        voutAdcMv * 4L;

    /*
     * Buck shunt circuit:
     *
     * Shunt = 0.05 ohm
     * Op-amp gain = 1 + 190k/10k = 20
     *
     * Output = current * 0.05 * 20
     *        = 1 V/A
     *
     * Therefore, the numerical ADC millivolt value equals mA.
     */
    buckCurrentAdcMv = RawToMillivolts(buckCurrentRaw);
    g_measurements.buckCurrentMilliamps =
        buckCurrentAdcMv;

/*
 * Thermistor divider:
 *
 * 3.3 V -- 10k fixed resistor -- TEMP_BAT
 * TEMP_BAT -- 10k NTC -- GND
 *
 * Rntc = Rfixed * Vtemp / (Vsupply - Vtemp)
 */
temperatureAdcMv = RawToMillivolts(temperatureRaw);

g_measurements.temperatureAdcMillivolts =
    temperatureAdcMv;

if ((temperatureAdcMv > 0L) &&
    (temperatureAdcMv < LOGIC_SUPPLY_MV))
{
    g_measurements.thermistorOhms =
        (THERMISTOR_FIXED_OHMS *
         temperatureAdcMv) /
        (LOGIC_SUPPLY_MV - temperatureAdcMv);

    g_measurements.batteryTemperatureValid =
        ThermistorToDeciC(
            g_measurements.thermistorOhms,
            (signed int *)
            &g_measurements.batteryTemperatureDeciC);
}
else
{
    /*
     * Possible open circuit, short circuit,
     * or invalid ADC reading.
     */
    g_measurements.thermistorOhms = -1L;
    g_measurements.batteryTemperatureDeciC = 0;
    g_measurements.batteryTemperatureValid = 0;
}

    /*
     * ACS758LCB-050U powered from 3.3 V.
     *
     * At 5 V:
     *     Zero output = 600 mV
     *     Sensitivity = 60 mV/A
     *
     * Ratiometrically scaled to 3.3 V:
     *     Zero output = 396 mV
     *     Sensitivity = 39.6 mV/A
     *
     * ADS1115:
     *     125 microvolts/count
     *
     * mA = deltaRaw * 125 / 39.6
     *     = deltaRaw * 1250 / 396
     */
    batteryDeltaRaw =
        (signed long)batteryCurrentRaw -
        (signed long)g_batteryCurrentZeroRaw;

    g_measurements.batteryCurrentMilliamps =
        (batteryDeltaRaw * 1250L) / 396L;

    g_measurements.systemAdsPresent = 1;

    return 1;
}

unsigned char Measurements_Update(void)
{
    unsigned char batterySuccess;
    unsigned char systemSuccess;

    batterySuccess = ReadBatteryMeasurements();
    systemSuccess = ReadSystemMeasurements();

    if (batterySuccess && systemSuccess)
    {
        return 1;
    }

    return 0;
}
