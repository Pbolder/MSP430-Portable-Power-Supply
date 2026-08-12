# Testing and Measured Performance

The completed prototype was tested with a digital multimeter, low-current LED loads, and a 380 brushed DC motor. These tests verify the operating modes, selectable output voltages, display measurements, current-sensing paths, user controls, and basic protection behavior.

## Output-voltage testing

The 380 motor was used as the load for the final voltage comparisons. The multimeter was treated as the reference instrument.

| Selected output | Central DMM reading | Observed variation | OLED difference from DMM |
|---|---:|---:|---:|
| 3.3 V | 3.30 V | +/-0.04 V | +/-0.01 V |
| 5 V | 5.00 V | +/-0.03 V | +/-0.01 V |
| 9 V | 8.95 V | +/-0.07 V | +/-0.02 V |

The motor operated at all three voltage settings. LED tests were also successful and helped confirm regulation with a simple low-current load.

## Measurement checks

- Buck output current: the shunt and MCP6002 measurement differed from the series multimeter reading by approximately +/-0.02 A.
- Charging current: the charging-current channel differed from the series multimeter reading by approximately +/-0.01 A.
- Cell voltages: displayed values followed direct balance-connector measurements; the largest observed error was approximately 2.5% on the calculated third-cell value.
- Output voltage: the OLED reading remained within 0.02 V of the multimeter during the documented loaded tests.

## Functional checks

- OFF mode removed logic power and disabled charging and output operation.
- CHARGE mode connected the internal balance charger while keeping the buck converter disabled.
- OUTPUT mode disconnected the charger and allowed the user to enable the selected voltage.
- Pressing the enable button during CHARGE mode produced the intended warning indication without enabling the output.
- Heating the battery thermistor caused the firmware to disable the converter and display the temperature warning.

## Bring-up correction

Initial buck-converter activation opened the 5 A input fuse and damaged the MOSFET. The root cause was a catch-diode PCB footprint whose anode and cathode mapping was reversed relative to the schematic. Installing the diode in the electrically correct orientation and replacing the damaged MOSFET restored normal converter operation.

## Current testing limitations

The following items were not directly characterized because a programmable electronic load and oscilloscope were unavailable:

- Continuous operation at the 2.5 A design target
- Conversion efficiency
- Output ripple and switching-node waveforms
- Load-transient response
- Full-load temperature rise
- Long-duration endurance

The 50 A ACS758 total battery-current sensor also proved too insensitive for accurate measurement in the system's normal 0-3 A range. This limitation is specific to the total battery-current channel; the separate buck-output shunt measurement agreed closely with the multimeter.

Future testing should use a current-limited electronic load, oscilloscope, differential or appropriately rated probes, and temperature measurements at the MOSFET, diode, inductor, and output capacitors.
