#ifndef SSD1309_H_
#define SSD1309_H_

#define SSD1309_ADDRESS 0x3C
#define SSD1309_WIDTH   128U
#define SSD1309_PAGES   8U

unsigned char SSD1309_Init(void);
unsigned char SSD1309_Clear(void);
unsigned char SSD1309_Fill(unsigned char pattern);
unsigned char SSD1309_SetDisplayEnabled(unsigned char enabled);


unsigned char SSD1309_WriteChar(
    unsigned char page,
    unsigned char column,
    char character);

unsigned char SSD1309_WriteString(
    unsigned char page,
    unsigned char column,
    const char *text);

/*
 * Clears a rectangular region measured in display pages and
 * pixel columns. Used to erase a two-page icon cleanly.
 */
unsigned char SSD1309_ClearRegion(
    unsigned char startPage,
    unsigned char startColumn,
    unsigned char width,
    unsigned char pageCount);


typedef enum
{
    SSD1309_ICON_BATTERY = 0,
    SSD1309_ICON_PLUG,
    SSD1309_ICON_LIGHTNING,
    SSD1309_ICON_THERMOMETER
} SSD1309_Icon;

/* Custom character code rendered as a small degree symbol. */
#define SSD1309_CHAR_DEGREE ((char)0x7F)

unsigned char SSD1309_WriteIcon(
    unsigned char page,
    unsigned char column,
    SSD1309_Icon icon);

/*
 * Draws the battery outline with 0 through 4 filled charge bars.
 */
unsigned char SSD1309_WriteBatteryLevel(
    unsigned char page,
    unsigned char column,
    unsigned char filledBars);



#endif
