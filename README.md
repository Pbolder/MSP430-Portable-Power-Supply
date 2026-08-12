# MSP430 Protable Power Supply

A portable, rechargeable power supply built around an MSP430G2553 and a custom discrete-component buck converter. The device accepts a 3S lithium-polymer battery and provides user-selectable 3.3 V, 5 V, and 9 V outputs while monitoring battery condition, output voltage, current, and temperature.

The full design calculations, component-selection reasoning, schematics, development history, and testing discussion are available in the [Full engineering report](MSP430-Programmable-Power-Supply/documentation/MSP430-Programmable-Power-Supply-Engineering-Report.pdf).


## Project overview

This project was developed to create a portable alternative to a benchtop power supply and to gain practical experience with power electronics, embedded firmware, mixed-signal PCB design, and mechanical integration. Rather than using a preassembled DC-DC module, the power stage was designed from individual components and implemented on a custom four-layer PCB.

The finished prototype integrates a TL494 PWM controller, IRS2110 gate driver, external MOSFET, 68 uH inductor, selectable analog feedback network, battery charger, voltage and current sensing, two ADS1115 ADC modules, an SSD1309 OLED, front-panel controls, and an MSP430G2553 microcontroller. The electronics, battery, controls, display, and wiring are installed in a custom 3D-printed enclosure.

The complete design process included LTspice simulation, Altium schematic capture and PCB layout, C firmware in Texas Instruments Code Composer Studio, Onshape enclosure design, hardware bring-up, fault diagnosis, and measured validation.

## Key results

- Produced selectable 3.3 V, 5 V, and 9 V outputs from a 3S battery.
- Successfully powered LED loads and a 380 brushed DC motor at all three voltage settings.
- Matched OLED output-voltage measurements to a digital multimeter within 0.02 V in the completed load tests.
- Implemented mutually exclusive OFF, CHARGE, and OUTPUT operating modes.
- Implemented firmware overcurrent and overtemperature shutdown behavior.
- Diagnosed and corrected a reversed catch-diode footprint during hardware bring-up.
- Integrated the complete electrical system into a custom portable enclosure.

## Specifications

| Item | Implementation |
|---|---|
| Battery | 3S, 2200 mAh lithium-polymer pack |
| Battery range | Approximately 9.0-12.6 V |
| Microcontroller | Texas Instruments MSP430G2553 |
| Selectable outputs | 3.3 V, 5 V, and 9 V |
| Output-current design target | 2.5 A |
| Firmware overcurrent trip | 2.7 A |
| Firmware overtemperature trip | 95° C |
| PWM controller | TL494 |
| Gate driver | IRS2110 |
| Output inductor | 68 uH |
| Display | 2.42-inch, 128x64 SSD1309 OLED |
| ADCs | Two ADS1115 modules on a shared I2C bus |
| Battery Charger | Integrated 3S charger module |
| PCB | Custom four-layer board, primarily through-hole |
| Enclosure | Custom Onshape design, 3D printed |

## System architecture

![System block diagram](MSP430-Portable-Power-Supply/images/system-block-diagram.png)

The three-position mode switch provides hardware-level separation between charging and output operation. The MSP430 supervises these requests, reads the front-panel voltage selector and enable button, collects measurements, updates the display, and permits the buck converter to run only when the selected mode and safety state are valid. The TL494 and analog feedback network regulate the output; the MSP430 does not generate the converter's PWM waveform.

| Mode | Charging path | Buck converter | Logic and display |
|---|---|---|---|
| OFF | Disconnected | Disabled | Off |
| CHARGE | Connected through relays | Disabled | Active |
| OUTPUT | Disconnected | Available after enable request | Active |

## Electrical design

### Custom buck converter

The converter uses a TL494 PWM controller operating at approximately 45.5 kHz. A rotary switch selects one of three resistor networks in the analog feedback path to regulate 3.3 V, 5 V, or 9 V. An IRS2110 drives the external N-channel MOSFET, and a Schottky catch diode, 68 uH inductor, and output capacitors form the power stage.

The initial component values and expected transient behavior were evaluated in LTspice before the circuit was transferred to Altium.

| LTspice power-stage model | Simulated startup waveforms |
|---|---|
| ![LTspice power-stage model](images/ltspice-power-stage.png) | ![LTspice startup waveforms](images/ltspice-startup-waveforms.png) |

### Battery, charging, and sensing

The main battery path includes a polarized XT60 connector, replaceable 5 A blade fuse, and an ACS758 Hall-effect sensor. A separate internal 3S balance charger connects through relay-switched balance leads only in CHARGE mode.

Two ADS1115 modules digitize the system measurements. The monitored signals include cumulative battery cell taps, pack voltage, buck output voltage, buck output current, battery current, charging current, and battery temperature. Firmware reconstructs the individual cell voltages from the cumulative balance-connector voltages.

### PCB implementation

The four-layer PCB separates the switching power stage from the lower-level analog measurement and digital-control circuitry. High-current paths were routed with short, wide traces, while the internal ground plane provides a low-impedance return path.

| Routed PCB | 3D PCB view |
|---|---|
| ![Four-layer PCB routing](hardware/pcb/pcb-layout.png) | ![PCB 3D render](hardware/pcb/pcb-3d-render.png) |

## Firmware

The MSP430 firmware is written in C and organized into modules.

| Module | Responsibility |
|---|---|
| `main.c` | Fail-safe startup and cooperative task scheduling |
| `gpio.c` | Pin configuration, buck-enable output, LEDs, and raw inputs |
| `inputs.c` | Switch decoding and input debouncing |
| `mode_control.c` | OFF, CHARGE, and OUTPUT state behavior |
| `i2c.c` | MSP430 I2C transactions |
| `ads1115.c` | External ADC configuration and channel reads |
| `measurements.c` | Sensor scaling, cell calculations, current, and temperature conversion |
| `safety.c` | Overcurrent and overtemperature lockouts |
| `ssd1309.c` | OLED driver and drawing primitives |
| `display.c` | Mode-specific user-interface screens and warnings |
| `timer.c` | 10 ms, 100 ms, and 500 ms scheduler flags |

The firmware explicitly disables the converter during startup. In OUTPUT mode, the enable button toggles the output request. A valid mode, a user enable request, and a clear safety state are all required before the MSP430 asserts the buck-enable signal.

See [firmware/README.md](firmware/README.md) for the pin map, I2C addresses, project-import instructions, and calibration locations.

## Measured performance

Testing used a digital multimeter, LED loads, and a 380 brushed DC motor. The multimeter was treated as the reference measurement.

| Selected output | Central multimeter reading | Observed variation | OLED difference from multimeter |
|---|---:|---:|---:|
| 3.3 V | 3.30 V | +/-0.04 V | +/-0.01 V |
| 5 V | 5.00 V | +/-0.03 V | +/-0.01 V |
| 9 V | 8.95 V | +/-0.07 V | +/-0.02 V |

The shunt-and-op-amp buck-current measurement agreed with the series multimeter measurement within approximately 0.02 A. The charging-current measurement agreed within approximately 0.01 A. Additional test details and the current limitations are documented in [testing/README.md](testing/README.md).

## Hardware bring-up and engineering lessons

During the first buck-converter tests, enabling the output immediately opened the input fuse and damaged the MOSFET. Gate-drive checks and continuity measurements narrowed the fault to the switching stage. A direct comparison of the schematic, PCB footprint, and diode datasheet then showed that the physical catch-diode footprint had reversed anode and cathode mapping.

Installing the diode in the electrically correct orientation and replacing the damaged MOSFET restored normal operation. This failure reinforced the importance of verifying symbol-to-footprint mapping and component polarity directly against manufacturer documentation before fabrication.

Testing also showed that the 50 A ACS758 battery-current sensor is poorly matched to the system's normal 0-3 A operating range. Its low sensitivity makes offset and noise significant at the currents of interest. A future revision should use a lower-range Hall sensor or a shunt-based current monitor such as an INA226.

## Mechanical integration

The Onshape enclosure positions the PCB, battery, charger, OLED, mode switch, rotary voltage selector, illuminated enable button, indicator LEDs, and external connections in one portable assembly.

| Front-panel render | Open enclosure render |
|---|---|
| ![Front view of the enclosure](mechanical/enclosure-front-render.png) | ![Open enclosure showing internal organization](mechanical/enclosure-open-render.png) |

## Repository contents

```text
.
|-- README.md
|-- firmware/       MSP430 source code and CCS project files
|-- hardware/       Schematic PDF, PCB images, and complete BOM
|-- mechanical/     Enclosure renders
|-- documentation/  Full engineering report
|-- testing/        Measured results and test limitations
`-- images/         README photographs, diagrams, and simulations
```



Primary design references:

- [Complete electrical schematic](hardware/schematics/complete-electrical-schematic.pdf)
- [Complete bill of materials](hardware/bom/complete-bom.xlsx)
- [Detailed testing record](testing/README.md)

## Known limitations and next revision

- Efficiency, output ripple, switching waveforms, transient response, and full-load thermal behavior have not yet been verified with an oscilloscope and programmable load.
- The ACS758 total battery-current sensor lacks sufficient resolution for accurate sub-ampere measurement.
- A future PCB revision should correct the catch-diode footprint rather than relying on the assembly correction used on the prototype.


## Author

Designed and built by [Paul Bolder](https://github.com/Pbolder).
