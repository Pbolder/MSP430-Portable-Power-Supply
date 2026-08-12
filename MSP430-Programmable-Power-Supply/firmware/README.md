# MSP430 Firmware

This directory is an importable Texas Instruments Code Composer Studio project for the MSP430G2553. It contains the complete C source required for the prototype; generated `Debug` and `Release` build artifacts are intentionally excluded.

## Toolchain

- Target MCU: MSP430G2553
- Development environment: Texas Instruments Code Composer Studio
- Original compiler family: TI MSP430 Compiler 21.6.x
- Programmer/debugger: MSP-EXP430G2ET LaunchPad
- Target configuration: `targetConfigs/MSP430G2553.ccxml`

Newer CCS releases may prompt to migrate the project metadata or compiler version.

## Import, build, and program

1. Install Code Composer Studio with MSP430 device support.
2. Connect the MSP-EXP430G2ET LaunchPad to the computer and connect its Spy-Bi-Wire programming signals to the target MSP430G2553.
3. In CCS, choose **File > Import > CCS Projects**.
4. Select this `firmware` directory as the search location and import `MSP430_BMS&BUCK_Controller`.
5. Confirm that the selected device is MSP430G2553 and the target configuration uses the TI MSP430 USB connection.
6. Build the project, then start a debug session to program and verify the target.

## Pin assignments

| Pin | Firmware name | Function |
|---|---|---|
| P1.0 | `BUCK_ENABLE_PIN` | Buck-enable command output |
| P1.1 | `BUTTON_LED_PIN` | Illuminated enable-button LED, active-low |
| P1.2 | `STATUS_LED_PIN` | Status indicator output |
| P1.3 | `BUCK_ALLOWED_PIN` | OUTPUT-mode request input |
| P1.4 | `CHARGE_ALLOWED_PIN` | CHARGE-mode request input |
| P1.6 | `I2C_SCL_PIN` | I2C clock |
| P1.7 | `I2C_SDA_PIN` | I2C data |
| P2.0 | `ENABLE_BUTTON_PIN` | Output-enable button, active-low |
| P2.1 | `MENU_PIN` | Reserved input |
| P2.2 | `AUX_PIN` | Reserved input |
| P2.3 | `VSEL_3V3_PIN` | 3.3 V selector sense |
| P2.4 | `VSEL_5V_PIN` | 5 V selector sense |
| P2.5 | `VSEL_9V_PIN` | 9 V selector sense |

## I2C devices

| Address | Device |
|---:|---|
| `0x48` | Battery-measurement ADS1115 |
| `0x49` | System-measurement ADS1115 |
| `0x3C` | SSD1309 OLED |

## Scheduler

Timer_A produces cooperative task flags used by the main loop:

- 10 ms: debounce controls and update mode logic.
- 100 ms: sample buck current and evaluate protection logic.
- 500 ms: update the full measurement set and OLED screen.

## Configuration and calibration

- Overcurrent and temperature limits are defined in `safety.h`.
- Voltage-divider, thermistor, and current-sensor scaling is implemented in `measurements.c`.
- ADS1115 addresses and conversion configuration are defined in `ads1115.h` and `ads1115.c`.
- GPIO assignments and electrical polarity are defined in `gpio.h`.

Calibration constants are specific to the assembled prototype. Verify sensor zero points, divider ratios, and protection behavior before using the firmware on revised hardware.

## Module summary

| Files | Purpose |
|---|---|
| `clock.*` | Configure the calibrated 1 MHz system clock |
| `gpio.*` | Configure pins and expose hardware-control functions |
| `timer.*` | Provide scheduler timing flags |
| `inputs.*` | Debounce and decode switches and buttons |
| `mode_control.*` | Control operating mode and enable requests |
| `i2c.*` | Implement I2C bus transactions |
| `ads1115.*` | Read the two external ADCs |
| `measurements.*` | Convert raw readings into engineering units |
| `safety.*` | Apply output-current and temperature lockouts |
| `ssd1309.*` | Drive the OLED panel |
| `display.*` | Render operating screens and warning messages |
| `main.c` | Initialize the system and run the cooperative loop |
