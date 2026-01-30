# ESP32-S3-BOX-3 Board Configuration for Zephyr

This board configuration enables Zephyr OS support for the ESP32-S3-BOX-3 development kit.

## Hardware Features

- **MCU**: ESP32-S3-WROOM-1 module (N16R16 variant)
- **Flash**: 16MB Quad SPI Flash
- **PSRAM**: 16MB Octal PSRAM
- **Display**: 2.4" LCD with ESP32 LCD driver (320x240 resolution)
- **Interfaces**: 
  - USB Type-C (power, programming, debugging)
  - SPI2 for display communication
- **Controls**: Boot button (GPIO0), LCD backlight control (GPIO47)

## Pin Mapping

| Function | GPIO | Description |
|----------|------|-------------|
| Boot Button | GPIO0 | Boot/Download mode (active low, pull-up enabled) |
| LCD DC | GPIO4 | Display data/command control (active high) |
| LCD CS | GPIO5 | Display chip select |
| LCD MOSI | GPIO6 | Display data (SPI MOSI) |
| LCD SCK | GPIO7 | Display clock (SPI SCLK) |
| LCD Backlight | GPIO47 | Display backlight control (active high) |
| LCD Reset | GPIO48 | Display reset (active low) |

## Supported Features

### PROCPU (Main Core)
- [x] GPIO (gpio0, gpio1)
- [x] UART (console via USB serial)
- [x] SPI2 (display interface)
- [x] Display (ESP32 LCD driver)
- [x] Wi-Fi
- [x] Bluetooth LE HCI
- [x] USB Serial
- [x] Watchdog (wdt0)
- [x] Entropy/TRNG (trng0)
- [x] DMA
- [x] Input (GPIO keys)
- [x] Timers (timer0-3)

### APPCPU (Application Core)
- [x] Entropy/TRNG (trng0)
- [x] IPM (Inter-Processor Messaging)

## Peripheral Configuration

### Display (ESP32 LCD)
- **Controller**: ESP32 LCD driver (espressif,esp32-lcd)
- **Resolution**: 320x240 pixels
- **Interface**: SPI2 at 20 MHz
- **Pixel Format**: 0 (RGB565)
- **Rotation**: 0 degrees (portrait)
- **DC GPIO**: GPIO4 (active high)
- **Reset GPIO**: GPIO48 (active low)
- **Backlight**: GPIO47 (active high, configured as LED)

### SPI2 Configuration
- **SCLK**: GPIO7
- **MOSI**: GPIO6
- **CS**: GPIO5
- **Status**: Enabled for LCD display

### GPIO Configuration
- **GPIO0**: Boot button (active low, pull-up enabled)
- **GPIO47**: LCD backlight control (active high, configured as LED)

### Console and Shell
- **Console**: USB Serial
- **Shell**: USB Serial

## Building Applications

To build for ESP32-S3-BOX-3 PROCPU:

```bash
west build -b esp32s3_box3/esp32s3/procpu samples/hello_world
```

For dual-core applications (AMP mode):

```bash
# Build for PROCPU (main core)
west build -b esp32s3_box3/esp32s3/procpu samples/hello_world

# Build for APPCPU (application core)
west build -b esp32s3_box3/esp32s3/appcpu samples/hello_world
```

## Display Demo

To test the display functionality:

```bash
west build -b esp32s3_box3/esp32s3/procpu samples/boards/espressif/ESP32S3_BOX3/LCD_LVGL
```

## Programming and Debugging

The board supports programming and debugging via the built-in USB interface:

```bash
west flash
west debug
```

## Device Tree Aliases

- `watchdog0`: &wdt0
- `sw0`: &button0 (Boot button)

## AMP (Asymmetric Multiprocessing) Configuration

The board uses the `partitions_0x0_amp_16M.dtsi` partition scheme for dual-core operation:
- **PROCPU**: Main application core with full peripheral access
- **APPCPU**: Application core with IPM for inter-core communication
- **Shared Memory**: Used for IPC between cores

## References

- [ESP32-S3-BOX-3 Official Page](https://www.espressif.com/en/dev-board/esp32-s3-box-3-en)
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [ESP-BOX GitHub Repository](https://github.com/espressif/esp-box)
