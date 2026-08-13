#ifndef DISPLAY_H_
#define DISPLAY_H_

/*
 * Initializes the mode-screen state after SSD1309_Init().
 */
void Display_Init(void);

/*
 * Updates the automatic CHARGE or OUTPUT screen.
 * OFF and invalid modes turn the panel off.
 *
 * Returns 1 when all required OLED writes succeed.
 */
unsigned char Display_Update(void);

#endif /* DISPLAY_H_ */
