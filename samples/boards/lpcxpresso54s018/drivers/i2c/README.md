# I2C Scanner Sample

This sample demonstrates I2C communication on the LPC54S018 using FLEXCOMM4
configured as I2C master.

## Overview

The sample:
- Scans the I2C bus for connected devices
- Identifies common I2C device types
- Provides examples for EEPROM and I/O expander communication
- Includes I2C shell commands for manual control

## Hardware Requirements

- LPCXpresso54S018M board
- Optional I2C devices (EEPROM, sensors, displays, etc.)
- Pull-up resistors on I2C lines (usually on board)

## Pin Configuration

FLEXCOMM4 is configured as I2C and mapped to Arduino headers:
- P0.26: I2C SCL (Arduino D15)
- P0.25: I2C SDA (Arduino D14)

## Building and Running

### Build the sample:
```bash
# From project root
west build -b lpcxpresso54s018 samples/drivers/i2c -p auto
```

### Flash to device:
```bash
west flash
```

## Operation

The sample continuously:
1. Scans I2C addresses 0x08-0x77
2. Reports found devices with possible identification
3. Runs examples if specific devices are found
4. Waits 10 seconds before next scan

## Shell Commands

Connect to UART console (115200 baud) to use I2C shell commands:

```bash
# Scan I2C bus
i2c scan flexcomm4

# Read byte from device
i2c read flexcomm4 0x50 0x00

# Write byte to device
i2c write flexcomm4 0x50 0x00 0xAA

# Read multiple bytes
i2c read flexcomm4 0x50 0x00 16

# Configure I2C speed
i2c speed flexcomm4 400000
```

## Common I2C Devices

The scanner identifies these common devices:
- 0x20-0x27: PCF8574 I/O expanders
- 0x3C-0x3D: OLED displays (SSD1306, etc.)
- 0x48-0x4B: Temperature sensors (LM75, TMP102)
- 0x50-0x57: EEPROM (24C02, 24C08, etc.)
- 0x68-0x69: RTC (DS3231, DS1307)
- 0x76-0x77: BME280/BMP280 environmental sensors

## LED Status

- Blinking: I2C scanning active

## Adding I2C Devices

To add an I2C device to your application:

1. Connect device to I2C pins (D14/D15 on Arduino header)
2. Ensure proper pull-up resistors (typically 4.7kΩ)
3. Add device node to device tree overlay:

```dts
&flexcomm4 {
    my_sensor: sensor@76 {
        compatible = "bosch,bme280";
        reg = <0x76>;
        status = "okay";
    };
};
```

## Troubleshooting

1. **No devices found**: 
   - Check connections and pull-up resistors
   - Verify device power supply
   - Try slower I2C speed (100 kHz)

2. **Communication errors**:
   - Check for correct device address
   - Ensure device supports 400 kHz speed
   - Add delays between operations if needed

3. **Console shows no output**:
   - Connect to UART at 115200 baud
   - Check UART TX/RX connections

## Notes

- FLEXCOMM4 is shared with Arduino I2C header
- Default speed is 400 kHz (Fast mode)
- Some devices may require 100 kHz (Standard mode)
- Shell commands provide additional debugging capability