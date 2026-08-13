/*
 * ssd1309.c
 *
 * Minimal SSD1309 128x64 I2C driver.
 *
 * This implementation does not use a framebuffer because the
 * MSP430G2553 has only 512 bytes of RAM.
 */

#include <msp430.h>

#include "i2c.h"
#include "ssd1309.h"

#define SSD1309_CONTROL_COMMAND 0x00
#define SSD1309_CONTROL_DATA    0x40

#define SSD1309_DATA_CHUNK      16U
#define SSD1309_ICON_WIDTH       8U
#define SSD1309_LARGE_ICON_WIDTH 16U
#define SSD1309_BATTERY_WIDTH    18U


/*
 * Compact 5x7 font.
 *
 * Only characters needed by the BMS interface are included:
 * uppercase letters, numbers, space, and common symbols.
 *
 * The sixth blank column is added while writing each character.
 */
static const unsigned char digitFont[10][5] =
{
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, /* 0 */
    {0x00, 0x42, 0x7F, 0x40, 0x00}, /* 1 */
    {0x42, 0x61, 0x51, 0x49, 0x46}, /* 2 */
    {0x21, 0x41, 0x45, 0x4B, 0x31}, /* 3 */
    {0x18, 0x14, 0x12, 0x7F, 0x10}, /* 4 */
    {0x27, 0x45, 0x45, 0x45, 0x39}, /* 5 */
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, /* 6 */
    {0x01, 0x71, 0x09, 0x05, 0x03}, /* 7 */
    {0x36, 0x49, 0x49, 0x49, 0x36}, /* 8 */
    {0x06, 0x49, 0x49, 0x29, 0x1E}  /* 9 */
};

static const unsigned char letterFont[26][5] =
{
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, /* A */
    {0x7F, 0x49, 0x49, 0x49, 0x36}, /* B */
    {0x3E, 0x41, 0x41, 0x41, 0x22}, /* C */
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, /* D */
    {0x7F, 0x49, 0x49, 0x49, 0x41}, /* E */
    {0x7F, 0x09, 0x09, 0x09, 0x01}, /* F */
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, /* G */
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, /* H */
    {0x00, 0x41, 0x7F, 0x41, 0x00}, /* I */
    {0x20, 0x40, 0x41, 0x3F, 0x01}, /* J */
    {0x7F, 0x08, 0x14, 0x22, 0x41}, /* K */
    {0x7F, 0x40, 0x40, 0x40, 0x40}, /* L */
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, /* M */
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, /* N */
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, /* O */
    {0x7F, 0x09, 0x09, 0x09, 0x06}, /* P */
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, /* Q */
    {0x7F, 0x09, 0x19, 0x29, 0x46}, /* R */
    {0x46, 0x49, 0x49, 0x49, 0x31}, /* S */
    {0x01, 0x01, 0x7F, 0x01, 0x01}, /* T */
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, /* U */
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, /* V */
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, /* W */
    {0x63, 0x14, 0x08, 0x14, 0x63}, /* X */
    {0x07, 0x08, 0x70, 0x08, 0x07}, /* Y */
    {0x61, 0x51, 0x49, 0x45, 0x43}  /* Z */
};




/*
 * Large icons occupy two OLED pages.
 * Each array contains one horizontal eight-pixel section.
 */

/* Battery outline with three internal charge bars. */
static const unsigned char batteryIconTop[16] =
{
    0x00U, 0xFEU, 0x02U, 0xF2U,
    0xF2U, 0x02U, 0xF2U, 0xF2U,
    0x02U, 0xF2U, 0xF2U, 0x02U,
    0x02U, 0xFEU, 0xE0U, 0xE0U
};

static const unsigned char batteryIconBottom[16] =
{
    0x00U, 0x7FU, 0x40U, 0x4FU,
    0x4FU, 0x40U, 0x4FU, 0x4FU,
    0x40U, 0x4FU, 0x4FU, 0x40U,
    0x40U, 0x7FU, 0x07U, 0x07U
};

/* Solid lightning-bolt silhouette. */
static const unsigned char lightningIconTop[16] =
{
    0x00U, 0x00U, 0x00U, 0x40U,
    0x60U, 0x70U, 0x78U, 0xFCU,
    0xFEU, 0xFFU, 0xC7U, 0xC3U,
    0xC0U, 0x00U, 0x00U, 0x00U
};

static const unsigned char lightningIconBottom[16] =
{
    0x00U, 0x00U, 0x60U, 0x78U,
    0x7CU, 0x3EU, 0x1FU, 0x0FU,
    0x0FU, 0x07U, 0x03U, 0x01U,
    0x00U, 0x00U, 0x00U, 0x00U
};

/* Solid two-prong electrical plug silhouette. */
static const unsigned char plugIconTop[16] =
{
    0x00U, 0x00U, 0xF0U, 0xF0U,
    0xFFU, 0xFFU, 0xF0U, 0xF0U,
    0xF0U, 0xF0U, 0xFFU, 0xFFU,
    0xF0U, 0xF0U, 0x00U, 0x00U
};

static const unsigned char plugIconBottom[16] =
{
    0x00U, 0x00U, 0x00U, 0x03U,
    0x07U, 0x0FU, 0x1FU, 0xFFU,
    0xFFU, 0x1FU, 0x0FU, 0x07U,
    0x03U, 0x00U, 0x00U, 0x00U
};

static const unsigned char thermometerIconTop[16] =
{
    0x00U, 0x00U, 0x00U, 0x00U,
    0x00U, 0xFEU, 0x01U, 0xFDU,
    0xFDU, 0x01U, 0xFEU, 0x00U,
    0x00U, 0x00U, 0x00U, 0x00U
};

static const unsigned char thermometerIconBottom[16] =
{
    0x00U, 0x00U, 0x00U, 0x38U,
    0x44U, 0xF7U, 0xF8U, 0xFFU,
    0xFFU, 0xF8U, 0xF7U, 0x44U,
    0x38U, 0x00U, 0x00U, 0x00U
};




static void SSD1309_GetCharacter(
    char character,
    unsigned char *columns)
{
    unsigned char index;

    for (index = 0U; index < 5U; index++)
    {
        columns[index] = 0x00U;
    }

    /*
     * Convert lowercase input to uppercase.
     */
    if ((character >= 'a') && (character <= 'z'))
    {
        character = (char)(character - 'a' + 'A');
    }

    if ((character >= '0') && (character <= '9'))
    {
        for (index = 0U; index < 5U; index++)
        {
            columns[index] =
                digitFont[character - '0'][index];
        }

        return;
    }

    if ((character >= 'A') && (character <= 'Z'))
    {
        for (index = 0U; index < 5U; index++)
        {
            columns[index] =
                letterFont[character - 'A'][index];
        }

        return;
    }

    switch (character)
    {
        case '.':
            columns[2] = 0x60U;
            break;

        case ':':
            columns[2] = 0x36U;
            break;

        case '-':
            columns[1] = 0x08U;
            columns[2] = 0x08U;
            columns[3] = 0x08U;
            break;

        case '/':
            columns[0] = 0x20U;
            columns[1] = 0x10U;
            columns[2] = 0x08U;
            columns[3] = 0x04U;
            columns[4] = 0x02U;
            break;

        case '%':
            columns[0] = 0x23U;
            columns[1] = 0x13U;
            columns[2] = 0x08U;
            columns[3] = 0x64U;
            columns[4] = 0x62U;
            break;

        case '+':
            columns[1] = 0x08U;
            columns[2] = 0x1CU;
            columns[3] = 0x08U;
            break;

        case 0x7F:
            columns[1] = 0x06U;
            columns[2] = 0x09U;
            columns[3] = 0x06U;
            break;

        case ' ':
        default:
            break;
    }
}

static unsigned char SSD1309_WriteCommands(
    const unsigned char *commands,
    unsigned int commandCount)
{
    unsigned char packet[24];
    unsigned int index;

    if ((commands == 0) ||
        (commandCount == 0U) ||
        (commandCount > 23U))
    {
        return 0;
    }

    packet[0] = SSD1309_CONTROL_COMMAND;

    for (index = 0U; index < commandCount; index++)
    {
        packet[index + 1U] = commands[index];
    }

    return I2C_WriteBytes(SSD1309_ADDRESS,
                          packet,
                          commandCount + 1U);
}

static unsigned char SSD1309_SetCursor(
    unsigned char page,
    unsigned char column)
{
    unsigned char commands[3];

    if ((page >= SSD1309_PAGES) ||
        (column >= SSD1309_WIDTH))
    {
        return 0;
    }

    commands[0] = (unsigned char)(0xB0U | page);
    commands[1] = (unsigned char)(column & 0x0FU);
    commands[2] =
        (unsigned char)(0x10U | ((column >> 4) & 0x0FU));

    return SSD1309_WriteCommands(commands, 3U);
}

unsigned char SSD1309_Fill(unsigned char pattern)
{
    unsigned char packet[SSD1309_DATA_CHUNK + 1U];
    unsigned char page;
    unsigned char chunk;
    unsigned char index;

    packet[0] = SSD1309_CONTROL_DATA;

    for (index = 1U;
         index <= SSD1309_DATA_CHUNK;
         index++)
    {
        packet[index] = pattern;
    }

    for (page = 0U; page < SSD1309_PAGES; page++)
    {
        if (!SSD1309_SetCursor(page, 0U))
        {
            return 0;
        }

        /*
         * 128 columns / 16 bytes per transfer =
         * eight transfers per page.
         */
        for (chunk = 0U; chunk < 8U; chunk++)
        {
            if (!I2C_WriteBytes(
                    SSD1309_ADDRESS,
                    packet,
                    SSD1309_DATA_CHUNK + 1U))
            {
                return 0;
            }
        }
    }

    return 1;
}

unsigned char SSD1309_Clear(void)
{
    return SSD1309_Fill(0x00U);
}

unsigned char SSD1309_ClearRegion(
    unsigned char startPage,
    unsigned char startColumn,
    unsigned char width,
    unsigned char pageCount)
{
    unsigned char packet[SSD1309_DATA_CHUNK + 1U];
    unsigned char pageOffset;
    unsigned char index;
    unsigned char remaining;
    unsigned char transferWidth;

    if ((width == 0U) ||
        (pageCount == 0U) ||
        (startPage >= SSD1309_PAGES) ||
        (startColumn >= SSD1309_WIDTH) ||
        (((unsigned int)startPage + pageCount) > SSD1309_PAGES) ||
        (((unsigned int)startColumn + width) > SSD1309_WIDTH))
    {
        return 0;
    }

    packet[0] = SSD1309_CONTROL_DATA;

    for (index = 1U;
         index <= SSD1309_DATA_CHUNK;
         index++)
    {
        packet[index] = 0x00U;
    }

    for (pageOffset = 0U;
         pageOffset < pageCount;
         pageOffset++)
    {
        if (!SSD1309_SetCursor(
                (unsigned char)(startPage + pageOffset),
                startColumn))
        {
            return 0;
        }

        remaining = width;

        while (remaining > 0U)
        {
            transferWidth =
                (remaining > SSD1309_DATA_CHUNK) ?
                SSD1309_DATA_CHUNK : remaining;

            if (!I2C_WriteBytes(
                    SSD1309_ADDRESS,
                    packet,
                    (unsigned int)transferWidth + 1U))
            {
                return 0;
            }

            remaining =
                (unsigned char)(remaining - transferWidth);
        }
    }

    return 1;
}

unsigned char SSD1309_SetDisplayEnabled(unsigned char enabled)
{
    unsigned char command;

    command = enabled ? 0xAFU : 0xAEU;
    return SSD1309_WriteCommands(&command, 1U);
}

unsigned char SSD1309_Init(void)
{
    static const unsigned char initializationCommands[] =
    {
        0xFD, 0x12,       /* Unlock command interface */
        0xAE,             /* Display OFF */

        0xD5, 0x80,       /* Display clock divide */
        0xA8, 0x3F,       /* Multiplex ratio: 64 rows */
        0xD3, 0x00,       /* Display offset: zero */
        0x40,             /* Display start line: zero */

        0xA1,             /* Segment remap */
        0xC8,             /* COM scan direction remapped */
        0xDA, 0x12,       /* COM pin configuration */

        0x81, 0x7F,       /* Contrast */
        0xD9, 0x22,       /* Precharge period */
        0xDB, 0x34,       /* VCOMH deselect level */

        0xA4,             /* Display follows RAM */
        0xA6              /* Normal, non-inverted display */
    };

    static const unsigned char displayOnCommand = 0xAF;

    /*
     * Allow the OLED power rails to stabilize.
     * This is approximately 100 ms at a 1 MHz CPU clock.
     */
    __delay_cycles(100000UL);

    if (!SSD1309_WriteCommands(
            initializationCommands,
            sizeof(initializationCommands)))
    {
        return 0;
    }

    if (!SSD1309_Clear())
    {
        return 0;
    }

    if (!SSD1309_WriteCommands(&displayOnCommand, 1U))
    {
        return 0;
    }

    return 1;
}
unsigned char SSD1309_WriteChar(
    unsigned char page,
    unsigned char column,
    char character)
{
    unsigned char packet[7];
    unsigned char columns[5];
    unsigned char index;

    if ((page >= SSD1309_PAGES) ||
        (column > (SSD1309_WIDTH - 6U)))
    {
        return 0;
    }

    SSD1309_GetCharacter(character, columns);

    packet[0] = SSD1309_CONTROL_DATA;

    for (index = 0U; index < 5U; index++)
    {
        packet[index + 1U] = columns[index];
    }

    packet[6] = 0x00U;

    if (!SSD1309_SetCursor(page, column))
    {
        return 0;
    }

    return I2C_WriteBytes(
        SSD1309_ADDRESS,
        packet,
        sizeof(packet));
}

unsigned char SSD1309_WriteString(
    unsigned char page,
    unsigned char column,
    const char *text)
{
    if ((text == 0) || (page >= SSD1309_PAGES))
    {
        return 0;
    }

    while (*text != '\0')
    {
        if (column > (SSD1309_WIDTH - 6U))
        {
            break;
        }

        if (!SSD1309_WriteChar(page, column, *text))
        {
            return 0;
        }

        column = (unsigned char)(column + 6U);
        text++;
    }

    return 1;
}

unsigned char SSD1309_WriteIcon(
    unsigned char page,
    unsigned char column,
    SSD1309_Icon icon)
{
    const unsigned char *topBitmap;
    const unsigned char *bottomBitmap;
    unsigned char packet[SSD1309_LARGE_ICON_WIDTH + 2U];
    unsigned char index;

    /*
     * Large icons use two consecutive pages and occupy
     * 16 columns plus one blank spacing column.
     */
    if ((page >= (SSD1309_PAGES - 1U)) ||
        (column > (SSD1309_WIDTH -
                   SSD1309_LARGE_ICON_WIDTH - 1U)))
    {
        return 0;
    }

    switch (icon)
    {
        case SSD1309_ICON_BATTERY:
            topBitmap = batteryIconTop;
            bottomBitmap = batteryIconBottom;
            break;

        case SSD1309_ICON_PLUG:
            topBitmap = plugIconTop;
            bottomBitmap = plugIconBottom;
            break;

        case SSD1309_ICON_LIGHTNING:
            topBitmap = lightningIconTop;
            bottomBitmap = lightningIconBottom;
            break;

        case SSD1309_ICON_THERMOMETER:
            topBitmap = thermometerIconTop;
            bottomBitmap = thermometerIconBottom;
            break;

        default:
            return 0;
    }

    packet[0] = SSD1309_CONTROL_DATA;

    for (index = 0U;
         index < SSD1309_LARGE_ICON_WIDTH;
         index++)
    {
        packet[index + 1U] = topBitmap[index];
    }

    packet[SSD1309_LARGE_ICON_WIDTH + 1U] = 0x00U;

    if (!SSD1309_SetCursor(page, column))
    {
        return 0;
    }

    if (!I2C_WriteBytes(
            SSD1309_ADDRESS,
            packet,
            sizeof(packet)))
    {
        return 0;
    }

    for (index = 0U;
         index < SSD1309_LARGE_ICON_WIDTH;
         index++)
    {
        packet[index + 1U] = bottomBitmap[index];
    }

    if (!SSD1309_SetCursor(
            (unsigned char)(page + 1U),
            column))
    {
        return 0;
    }

    return I2C_WriteBytes(
        SSD1309_ADDRESS,
        packet,
        sizeof(packet));
}

unsigned char SSD1309_WriteBatteryLevel(
    unsigned char page,
    unsigned char column,
    unsigned char filledBars)
{
    unsigned char packet[SSD1309_BATTERY_WIDTH + 2U];
    unsigned char index;
    unsigned char top;
    unsigned char bottom;
    unsigned char barNumber;

    if ((page >= (SSD1309_PAGES - 1U)) ||
        (column > (SSD1309_WIDTH -
                   SSD1309_BATTERY_WIDTH - 1U)))
    {
        return 0;
    }

    if (filledBars > 4U)
    {
        filledBars = 4U;
    }

    packet[0] = SSD1309_CONTROL_DATA;

    for (index = 0U; index < SSD1309_BATTERY_WIDTH; index++)
    {
        if (index == 0U)
        {
            top = 0x00U;
            bottom = 0x00U;
        }
        else if ((index == 1U) || (index == 15U))
        {
            top = 0xFEU;
            bottom = 0x7FU;
        }
        else if (index >= 16U)
        {
            /* Centered two-column positive terminal. */
            top = 0xE0U;
            bottom = 0x07U;
        }
        else
        {
            /* Empty interior with top and bottom outline pixels. */
            top = 0x02U;
            bottom = 0x40U;

            /*
             * Four two-column bars occupy columns 3-4, 6-7,
             * 9-10, and 12-13.  Bars fill from left to right.
             */
            if ((index >= 3U) && (index <= 13U) &&
                (((index - 3U) % 3U) < 2U))
            {
                barNumber = (unsigned char)(((index - 3U) / 3U) + 1U);

                if (barNumber <= filledBars)
                {
                    top = 0xF2U;
                    bottom = 0x4FU;
                }
            }
        }

        packet[index + 1U] = top;
    }

    packet[SSD1309_BATTERY_WIDTH + 1U] = 0x00U;

    if (!SSD1309_SetCursor(page, column))
    {
        return 0;
    }

    if (!I2C_WriteBytes(
            SSD1309_ADDRESS,
            packet,
            sizeof(packet)))
    {
        return 0;
    }

    packet[0] = SSD1309_CONTROL_DATA;

    for (index = 0U; index < SSD1309_BATTERY_WIDTH; index++)
    {
        if (index == 0U)
        {
            bottom = 0x00U;
        }
        else if ((index == 1U) || (index == 15U))
        {
            bottom = 0x7FU;
        }
        else if (index >= 16U)
        {
            bottom = 0x07U;
        }
        else
        {
            bottom = 0x40U;

            if ((index >= 3U) && (index <= 13U) &&
                (((index - 3U) % 3U) < 2U))
            {
                barNumber = (unsigned char)(((index - 3U) / 3U) + 1U);

                if (barNumber <= filledBars)
                {
                    bottom = 0x4FU;
                }
            }
        }

        packet[index + 1U] = bottom;
    }

    packet[SSD1309_BATTERY_WIDTH + 1U] = 0x00U;

    if (!SSD1309_SetCursor(
            (unsigned char)(page + 1U),
            column))
    {
        return 0;
    }

    return I2C_WriteBytes(
        SSD1309_ADDRESS,
        packet,
        sizeof(packet));
}



