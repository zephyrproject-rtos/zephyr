<!--
Copyright (c) 2026 NotioNext Ltd.
SPDX-License-Identifier: Apache-2.0
-->

# ESP32-S3-BOX-3 Board Configuration for Zephyr

This board configuration enables Zephyr OS support for the ESP32-S3-BOX-3 development kit, a comprehensive IoT development platform with display, touch and wireless capabilities.

## Hardware Features

- **MCU**: ESP32-S3-WROOM-1 module (N16R16 variant)
- **Flash**: 16MB Quad SPI Flash
- **PSRAM**: 16MB Octal PSRAM
- **Display**: 2.4" ILI9342C LCD with ESP32 LCD driver (320x240 resolution)
- **Touch**: GT911 capacitive touch controller
- **Interfaces**:
  - USB Type-C (power, programming, debugging)
  - SPI2 for display communication
  - I2C1 for touch controller and sensors
- **Controls**: Boot button (GPIO0), LCD backlight control (GPIO47)
- **Connectivity**: Wi-Fi 802.11 b/g/n, Bluetooth 5.0 LE

## Pin Mapping

| Function | GPIO | Description |
|----------|------|-------------|
| Boot Button | GPIO0 | Boot/Download mode (active low, pull-up enabled) |
| Touch Interrupt | GPIO3 | GT911 touch controller interrupt |
| LCD DC | GPIO4 | Display data/command control (active high) |
| LCD CS | GPIO5 | Display chip select |
| LCD MOSI | GPIO6 | Display data (SPI MOSI) |
| LCD SCK | GPIO7 | Display clock (SPI SCLK) |
| I2C1 SDA | GPIO8 | Touch controller and sensors data |
| I2C1 SCL | GPIO18 | Touch controller and sensors clock |
| LCD Backlight | GPIO47 | Display backlight control (active high) |
| LCD Reset | GPIO48 | Display reset (active low) |

## Supported Features

### PROCPU (Main Core)
- [x] GPIO (gpio0, gpio1)
- [x] UART (console via USB serial)
- [x] SPI2 (display interface)
- [x] I2C1 (touch controller, sensors)
- [x] Display (ESP32 LCD driver)
- [x] Touch (GT911 capacitive touch)
- [x] Input subsystem (touch events)
- [x] Wi-Fi
- [x] Bluetooth LE HCI
- [x] USB Serial
- [x] Watchdog (wdt0)
- [x] Entropy/TRNG (trng0)
- [x] DMA
- [x] NVS (Non-Volatile Storage)
- [x] Timers (timer0-3)

### APPCPU (Application Core)
- [x] Entropy/TRNG (trng0)
- [x] IPM (Inter-Processor Messaging)

## Peripheral Configuration

### Display (ESP32 LCD)
- **Controller**: ESP32 LCD driver (espressif,esp32-lcd)
- **Resolution**: 320x240 pixels
- **Interface**: SPI2 at 20 MHz
- **Pixel Format**: RGB565 (16-bit)
- **Rotation**: 0 degrees (portrait)
- **DC GPIO**: GPIO4 (active high)
- **Reset GPIO**: GPIO48 (active low)
- **Backlight**: GPIO47 (active high, configured as LED)

### Touch Controller (GT911)
- **Controller**: GT911 capacitive touch controller
- **Interface**: I2C1 at address 0x5D
- **Interrupt**: GPIO3 (active low)
- **Resolution**: 320x240 touch points
- **Multi-touch**: Up to 5 simultaneous touch points
- **Integration**: Zephyr input subsystem

### SPI2 Configuration
- **SCLK**: GPIO7
- **MOSI**: GPIO6
- **CS**: GPIO5
- **Status**: Enabled for LCD display

### I2C1 Configuration
- **SDA**: GPIO8
- **SCL**: GPIO18
- **Devices**: GT911 touch controller (0x5D), AHT30 sensor (0x38)

### GPIO Configuration
- **GPIO0**: Boot button (active low, pull-up enabled)
- **GPIO3**: Touch interrupt (active low)
- **GPIO47**: LCD backlight control (active high, configured as LED)

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

## Sample Applications

The ESP32-S3-BOX-3 board comes with several specialized sample applications that demonstrate its capabilities:

### 1. LCD LVGL Demo (`samples/boards/espressif/lcd_lvgl`)

**Purpose**: Basic LVGL display functionality demonstration

**Features**:
- LVGL initialization and display driver setup
- Simple "Hello World!" text display
- Green background with white text
- LCD backlight control
- Display blanking control

**Build and Run**:
```bash
west build -b esp32s3_box3/esp32s3/procpu samples/boards/espressif/lcd_lvgl
west flash
```

**What it demonstrates**:
- ESP32 LCD driver integration
- LVGL graphics library usage
- Display initialization sequence
- Basic LVGL object creation and styling

### 2. LCD LVGL Touch Demo (`samples/boards/espressif/lcd_lvgl_touch`)

**Purpose**: Advanced LVGL with touch navigation and Bluetooth beacon

**Features**:
- **Two-screen LVGL interface**:
  - Screen 1: "Click Next for next screen" with Next button
  - Screen 2: "Hello World!" with green background
- **GT911 touch controller integration**
- **Touch navigation** between screens
- **Bluetooth LE beacon** advertising as "ESP32S3BOX3"
- **Dual functionality**: LVGL + Bluetooth running simultaneously
- **Touch debouncing** and proper event handling

**Build and Run**:
```bash
west build -b esp32s3_box3/esp32s3/procpu samples/boards/espressif/lcd_lvgl_touch
west flash
```

**What it demonstrates**:
- GT911 capacitive touch controller usage
- Zephyr input subsystem integration
- Multi-screen LVGL applications
- Touch event handling and debouncing
- Bluetooth LE beacon functionality
- Concurrent operation of display and wireless features

### 3. WiFi BLE Provisioning with LVGL (`samples/boards/espressif/wifi_ble_provisioning_lvgl`)

**Purpose**: Complete WiFi provisioning system with modern UI

**Features**:
- **Minimalistic white LVGL UI** with modern design
- **Two-screen navigation**:
  - Main screen: Status display with "Device IP" button
  - IP display screen: Shows device IP address
- **BLE WiFi provisioning**: Receive credentials via Bluetooth
- **NVS persistent storage**: Save WiFi credentials across reboots
- **Python provisioning script** with retry logic
- **Touch debouncing** (300ms) for smooth navigation
- **Auto-reconnect**: Automatically connects using stored credentials
- **Factory reset**: 5-button press sequence
- **Real-time status updates** with color-coded feedback

**Build and Run**:
```bash
west build -b esp32s3_box3/esp32s3/procpu samples/boards/espressif/wifi_ble_provisioning_lvgl
west flash
```

**Provisioning**:
```bash
# Install Python dependencies
pip install bleak

# Provision WiFi credentials
python3 provision_wifi.py --ssid "YourNetwork" --password "YourPassword"
```

**What it demonstrates**:
- Complete IoT device provisioning workflow
- BLE GATT server implementation
- WiFi connection management
- NVS storage for persistent data
- Modern UI design principles
- Touch input with proper debouncing
- Asynchronous WiFi connection handling
- Python-based provisioning tools

## Programming and Debugging

The board supports programming and debugging via the built-in USB interface:

```bash
west flash
west debug
```

## Device Tree Aliases

- `watchdog0`: &wdt0
- `sw0`: &button0 (Boot button)
- `zephyr,display`: &ili9342c (LCD display)

## Sample Application Comparison

| Feature | lcd_lvgl | lcd_lvgl_touch | wifi_ble_provisioning_lvgl |
|---------|----------|----------------|---------------------------|
| **Display** | ✅ Basic | ✅ Advanced | ✅ Modern UI |
| **Touch** | ❌ | ✅ GT911 | ✅ GT911 + Debouncing |
| **LVGL** | ✅ Simple | ✅ Multi-screen | ✅ Event-driven |
| **Bluetooth** | ❌ | ✅ Beacon | ✅ GATT Server |
| **WiFi** | ❌ | ❌ | ✅ Full management |
| **Storage** | ❌ | ❌ | ✅ NVS |
| **Provisioning** | ❌ | ❌ | ✅ Complete system |
| **Python Tools** | ❌ | ❌ | ✅ Provisioning script |

## Getting Started Recommendations

1. **Start with `lcd_lvgl`**: Learn basic display and LVGL concepts
2. **Progress to `lcd_lvgl_touch`**: Add touch interaction and Bluetooth
3. **Advanced: `wifi_ble_provisioning_lvgl`**: Complete IoT device development

## AMP (Asymmetric Multiprocessing) Configuration

The board uses the `partitions_0x0_amp_16M.dtsi` partition scheme for dual-core operation:
- **PROCPU**: Main application core with full peripheral access
- **APPCPU**: Application core with IPM for inter-core communication
- **Shared Memory**: Used for IPC between cores

## Troubleshooting

### Display Issues
- Ensure backlight is enabled (GPIO47)
- Check SPI2 configuration and wiring
- Verify LVGL memory pool size in `prj.conf`

### Touch Issues
- Verify GT911 is detected on I2C1 address 0x5D
- Check interrupt line (GPIO3) configuration
- Monitor touch coordinates in logs

### Bluetooth Issues
- Ensure Bluetooth is enabled in `prj.conf`
- Check for memory conflicts with LVGL
- Verify advertising data format

### WiFi Issues
- Use 2.4GHz networks only (5GHz not supported)
- Check antenna connection
- Monitor WiFi manager logs for detailed errors

### Memory Issues
- Increase heap size if experiencing crashes
- Adjust LVGL buffer sizes
- Monitor stack usage in threads

## References

- [ESP32-S3-BOX-3 Official Page](https://www.espressif.com/en/dev-board/esp32-s3-box-3-en)
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [ESP-BOX GitHub Repository](https://github.com/espressif/esp-box)
- [GT911 Touch Controller Datasheet](https://github.com/goodix/gt9xx_driver_generic)
- [LVGL Documentation](https://docs.lvgl.io/)
- [Zephyr Display Interface](https://docs.zephyrproject.org/latest/hardware/peripherals/display.html)
- [Zephyr Input Subsystem](https://docs.zephyrproject.org/latest/hardware/peripherals/input.html)
