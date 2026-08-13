/*
 * Paul Bolder
 * display.c
 *
 * Automatic mode-specific SSD1309 screens.  Values are formatted
 * with integer arithmetic to avoid pulling printf into flash.
 */

#include "display.h"
#include "inputs.h"
#include "measurements.h"
#include "mode_control.h"
#include "safety.h"
#include "ssd1309.h"

#define DISPLAY_MODE_UNSET 0xFFU
#define BATTERY_BARS_UNSET 0xFFU
#define BATTERY_HYSTERESIS_MV 25L

static unsigned char s_drawnMode = DISPLAY_MODE_UNSET;
static unsigned char s_panelEnabled = 1U;
static unsigned char s_batteryBars = BATTERY_BARS_UNSET;

static void FormatVoltage(signed long millivolts, char *text)
{
    unsigned long centivolts;
    unsigned int volts;
    unsigned int fraction;

    if ((millivolts < 0L) || (millivolts > 99990L))
    {
        text[0] = '-';
        text[1] = '-';
        text[2] = '.';
        text[3] = '-';
        text[4] = '-';
        text[5] = 'V';
        text[6] = '\0';
        return;
    }

    centivolts = (unsigned long)(millivolts + 5L) / 10UL;
    volts = (unsigned int)(centivolts / 100UL);
    fraction = (unsigned int)(centivolts % 100UL);

    text[0] = (volts >= 10U) ?
        (char)('0' + (volts / 10U)) : ' ';
    text[1] = (char)('0' + (volts % 10U));
    text[2] = '.';
    text[3] = (char)('0' + (fraction / 10U));
    text[4] = (char)('0' + (fraction % 10U));
    text[5] = 'V';
    text[6] = '\0';
}

static void FormatCellVoltage(signed long millivolts, char *text)
{
    unsigned long centivolts;

    if ((millivolts < 0L) || (millivolts > 9990L))
    {
        text[0] = '-';
        text[1] = '.';
        text[2] = '-';
        text[3] = '-';
        text[4] = 'V';
        text[5] = '\0';
        return;
    }

    centivolts = (unsigned long)(millivolts + 5L) / 10UL;
    text[0] = (char)('0' + ((centivolts / 100UL) % 10UL));
    text[1] = '.';
    text[2] = (char)('0' + ((centivolts / 10UL) % 10UL));
    text[3] = (char)('0' + (centivolts % 10UL));
    text[4] = 'V';
    text[5] = '\0';
}

static void FormatCurrent(signed long milliamps, char *text)
{
    unsigned long magnitude;
    unsigned long hundredths;
    unsigned int amps;
    unsigned int fraction;
    unsigned char negative;

    if ((milliamps < -99990L) || (milliamps > 99990L))
    {
        text[0] = '-';
        text[1] = '-';
        text[2] = '.';
        text[3] = '-';
        text[4] = '-';
        text[5] = 'A';
        text[6] = '\0';
        return;
    }

    negative = (milliamps < 0L) ? 1U : 0U;
    magnitude = negative ?
        (unsigned long)(-milliamps) : (unsigned long)milliamps;
    hundredths = (magnitude + 5UL) / 10UL;
    amps = (unsigned int)(hundredths / 100UL);
    fraction = (unsigned int)(hundredths % 100UL);

    if (negative)
    {
        text[0] = '-';
    }
    else if (amps >= 10U)
    {
        text[0] = (char)('0' + (amps / 10U));
    }
    else
    {
        text[0] = ' ';
    }

    text[1] = (char)('0' + (amps % 10U));
    text[2] = '.';
    text[3] = (char)('0' + (fraction / 10U));
    text[4] = (char)('0' + (fraction % 10U));
    text[5] = 'A';
    text[6] = '\0';
}

static void FormatTemperature(signed int deciCelsius, char *text)
{
    signed long deciFahrenheit;
    signed int fahrenheit;
    unsigned int magnitude;

    deciFahrenheit =
        (((signed long)deciCelsius * 9L) / 5L) + 320L;

    if (deciFahrenheit >= 0L)
    {
        fahrenheit = (signed int)((deciFahrenheit + 5L) / 10L);
    }
    else
    {
        fahrenheit = (signed int)((deciFahrenheit - 5L) / 10L);
    }

    if ((fahrenheit < -99) || (fahrenheit > 999))
    {
        text[0] = ' ';
        text[1] = '-';
        text[2] = '-';
    }
    else if (fahrenheit < 0)
    {
        magnitude = (unsigned int)(-fahrenheit);
        if (magnitude >= 10U)
        {
            text[0] = '-';
            text[1] = (char)('0' + (magnitude / 10U));
        }
        else
        {
            text[0] = ' ';
            text[1] = '-';
        }
        text[2] = (char)('0' + (magnitude % 10U));
    }
    else
    {
        magnitude = (unsigned int)fahrenheit;
        text[0] = (magnitude >= 100U) ?
            (char)('0' + (magnitude / 100U)) : ' ';
        text[1] = (magnitude >= 10U) ?
            (char)('0' + ((magnitude / 10U) % 10U)) : ' ';
        text[2] = (char)('0' + (magnitude % 10U));
    }

    text[3] = SSD1309_CHAR_DEGREE;
    text[4] = 'F';
    text[5] = '\0';
}

static void FormatSelectedVoltage(
    OutputVoltageSelection selection,
    char *text)
{
    text[0] = ' ';

    switch (selection)
    {
        case OUTPUT_VOLTAGE_3V3:
            text[1] = '3';
            text[2] = '.';
            text[3] = '3';
            text[4] = 'V';
            break;

        case OUTPUT_VOLTAGE_5V:
            text[1] = '5';
            text[2] = '.';
            text[3] = '0';
            text[4] = 'V';
            break;

        case OUTPUT_VOLTAGE_9V:
            text[1] = '9';
            text[2] = '.';
            text[3] = '0';
            text[4] = 'V';
            break;

        case OUTPUT_VOLTAGE_NONE:
        case OUTPUT_VOLTAGE_INVALID:
        default:
            text[1] = '-';
            text[2] = '.';
            text[3] = '-';
            text[4] = 'V';
            break;
    }

    text[5] = '\0';
}

static unsigned char DrawCommonHeader(void)
{
    return SSD1309_WriteIcon(0U, 80U, SSD1309_ICON_THERMOMETER);
}

static unsigned char DrawChargingTemplate(void)
{
    return SSD1309_Clear() &&
           DrawCommonHeader() &&
           SSD1309_WriteString(2U, 0U, "CHG CURRENT") &&
           SSD1309_WriteString(3U, 0U, "C1") &&
           SSD1309_WriteString(3U, 60U, "C2") &&
           SSD1309_WriteString(4U, 0U, "C3") &&
           SSD1309_WriteString(5U, 0U, "BAT CURRENT") &&
           SSD1309_WriteIcon(6U, 31U, SSD1309_ICON_PLUG) &&
           SSD1309_WriteString(6U, 49U, "CHARGING");
}

static unsigned char DrawOutputTemplate(void)
{
    return SSD1309_Clear() &&
           DrawCommonHeader() &&
           SSD1309_WriteString(2U, 0U, "VOLT OUT") &&
           SSD1309_WriteString(3U, 0U, "CURRENT") &&
           SSD1309_WriteString(4U, 0U, "SELECTED VOLT") &&
           SSD1309_WriteString(5U, 0U, "BAT CURRENT") &&
           SSD1309_WriteIcon(6U, 25U, SSD1309_ICON_LIGHTNING);
}

static unsigned char DrawChargingIndicator(void)
{
    if ((!g_chargeEnableWarningActive) ||
        g_chargeEnableWarningVisible)
    {
        return SSD1309_WriteIcon(
                   6U,
                   31U,
                   SSD1309_ICON_PLUG) &&
               SSD1309_WriteString(6U, 49U, "CHARGING");
    }

    return SSD1309_ClearRegion(6U, 31U, 17U, 2U) &&
           SSD1309_WriteString(6U, 49U, "        ");
}

static unsigned char DrawOutputIndicator(void)
{
    if (g_highTemperatureLockout)
    {
        if (g_highTemperatureWarningVisible)
        {
            return SSD1309_WriteString(6U, 43U, "HIGH TEMP   ");
        }

        return SSD1309_WriteString(6U, 43U, "OUTPUT OFF  ");
    }

    if (g_overcurrentLockout)
    {
        return SSD1309_WriteString(6U, 43U, "OVER CURRENT");
    }

    if (g_buckEnabled)
    {
        return SSD1309_WriteString(6U, 43U, "OUTPUT ON   ");
    }

    return SSD1309_WriteString(6U, 43U, "OUTPUT OFF  ");
}

static unsigned char DrawTemperature(void)
{
    char text[6];

    if (g_measurements.systemAdsPresent &&
        g_measurements.batteryTemperatureValid)
    {
        FormatTemperature(
            g_measurements.batteryTemperatureDeciC,
            text);
    }
    else
    {
        text[0] = ' ';
        text[1] = '-';
        text[2] = '-';
        text[3] = SSD1309_CHAR_DEGREE;
        text[4] = 'F';
        text[5] = '\0';
    }

    return SSD1309_WriteString(0U, 98U, text);
}

static signed long BatteryThreshold(unsigned char bars)
{
    switch (bars)
    {
        case 1U:
            return 3300L;

        case 2U:
            return 3600L;

        case 3U:
            return 3800L;

        case 4U:
        default:
            return 4000L;
    }
}

static unsigned char InitialBatteryBars(signed long averageCellMv)
{
    if (averageCellMv >= 4000L)
    {
        return 4U;
    }

    if (averageCellMv >= 3800L)
    {
        return 3U;
    }

    if (averageCellMv >= 3600L)
    {
        return 2U;
    }

    if (averageCellMv >= 3300L)
    {
        return 1U;
    }

    return 0U;
}

static unsigned char DrawBatteryLevel(void)
{
    signed long averageCellMv;

    if (!g_measurements.batteryAdsPresent ||
        (g_measurements.cell1Millivolts < 2500L) ||
        (g_measurements.cell1Millivolts > 4500L) ||
        (g_measurements.cell2Millivolts < 2500L) ||
        (g_measurements.cell2Millivolts > 4500L) ||
        (g_measurements.cell3Millivolts < 2500L) ||
        (g_measurements.cell3Millivolts > 4500L))
    {
        s_batteryBars = BATTERY_BARS_UNSET;
        return SSD1309_WriteBatteryLevel(0U, 0U, 0U);
    }

    averageCellMv =
        (g_measurements.cell1Millivolts +
         g_measurements.cell2Millivolts +
         g_measurements.cell3Millivolts) / 3L;

    if (s_batteryBars == BATTERY_BARS_UNSET)
    {
        s_batteryBars = InitialBatteryBars(averageCellMv);
    }
    else
    {
        while ((s_batteryBars < 4U) &&
               (averageCellMv >=
                (BatteryThreshold(
                    (unsigned char)(s_batteryBars + 1U)) +
                 BATTERY_HYSTERESIS_MV)))
        {
            s_batteryBars++;
        }

        while ((s_batteryBars > 0U) &&
               (averageCellMv <
                (BatteryThreshold(s_batteryBars) -
                 BATTERY_HYSTERESIS_MV)))
        {
            s_batteryBars--;
        }
    }

    return SSD1309_WriteBatteryLevel(
        0U,
        0U,
        s_batteryBars);
}

static unsigned char DrawChargingValues(void)
{
    char currentText[7];
    char batteryCurrentText[7];
    char cell1Text[6];
    char cell2Text[6];
    char cell3Text[6];

    if (g_measurements.batteryAdsPresent)
    {
        FormatCurrent(
            g_measurements.chargeCurrentMilliamps,
            currentText);
        FormatCellVoltage(g_measurements.cell1Millivolts, cell1Text);
        FormatCellVoltage(g_measurements.cell2Millivolts, cell2Text);
        FormatCellVoltage(g_measurements.cell3Millivolts, cell3Text);
    }
    else
    {
        FormatCurrent(100000L, currentText);
        FormatCellVoltage(-1L, cell1Text);
        FormatCellVoltage(-1L, cell2Text);
        FormatCellVoltage(-1L, cell3Text);
    }

    if (g_measurements.systemAdsPresent)
    {
        FormatCurrent(
            g_measurements.batteryCurrentMilliamps,
            batteryCurrentText);
    }
    else
    {
        FormatCurrent(100000L, batteryCurrentText);
    }

    return SSD1309_WriteString(2U, 92U, currentText) &&
           SSD1309_WriteString(3U, 18U, cell1Text) &&
           SSD1309_WriteString(3U, 78U, cell2Text) &&
           SSD1309_WriteString(4U, 18U, cell3Text) &&
           SSD1309_WriteString(5U, 92U, batteryCurrentText);
}

static unsigned char DrawOutputValues(void)
{
    char voltageText[7];
    char currentText[7];
    char batteryCurrentText[7];
    char selectedText[6];

    if (g_measurements.systemAdsPresent)
    {
        FormatVoltage(g_measurements.voutMillivolts, voltageText);
        FormatCurrent(
            g_measurements.buckCurrentMilliamps,
            currentText);
        FormatCurrent(
            g_measurements.batteryCurrentMilliamps,
            batteryCurrentText);
    }
    else
    {
        FormatVoltage(-1L, voltageText);
        FormatCurrent(100000L, currentText);
        FormatCurrent(100000L, batteryCurrentText);
    }

    FormatSelectedVoltage(g_outputVoltageSelection, selectedText);

    return SSD1309_WriteString(2U, 92U, voltageText) &&
           SSD1309_WriteString(3U, 92U, currentText) &&
           SSD1309_WriteString(4U, 98U, selectedText) &&
           SSD1309_WriteString(5U, 92U, batteryCurrentText);
}

void Display_Init(void)
{
    s_drawnMode = DISPLAY_MODE_UNSET;
    s_panelEnabled = 1U;
    s_batteryBars = BATTERY_BARS_UNSET;
}

unsigned char Display_Update(void)
{
    unsigned char mode;

    mode = (unsigned char)g_operatingMode;

    if ((g_operatingMode != OPERATING_MODE_CHARGE) &&
        (g_operatingMode != OPERATING_MODE_OUTPUT))
    {
        s_drawnMode = mode;

        if (s_panelEnabled)
        {
            if (!SSD1309_SetDisplayEnabled(0U))
            {
                return 0;
            }
            s_panelEnabled = 0U;
        }

        return 1;
    }

    if (!s_panelEnabled)
    {
        if (!SSD1309_SetDisplayEnabled(1U))
        {
            return 0;
        }
        s_panelEnabled = 1U;
    }

    if (s_drawnMode != mode)
    {
        if (g_operatingMode == OPERATING_MODE_CHARGE)
        {
            if (!DrawChargingTemplate())
            {
                return 0;
            }
        }
        else
        {
            if (!DrawOutputTemplate())
            {
                return 0;
            }
        }

        s_drawnMode = mode;
    }

    if (!DrawBatteryLevel() || !DrawTemperature())
    {
        return 0;
    }

    if (g_operatingMode == OPERATING_MODE_CHARGE)
    {
        return DrawChargingValues() &&
               DrawChargingIndicator();
    }

    return DrawOutputValues() &&
           DrawOutputIndicator();
}
