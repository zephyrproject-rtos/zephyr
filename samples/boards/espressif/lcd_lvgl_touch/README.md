<!--
Copyright (c) 2026 NotioNext Ltd.
SPDX-License-Identifier: Apache-2.0
-->

# ESP32-S3-BOX-3 LVGL Touch Demo with Bluetooth Beacon

This sample demonstrates LVGL GUI with GT911 touch navigation and Bluetooth beacon functionality on the ESP32-S3-BOX-3 board.

## Features

- **Screen 1**: Shows "Click Next for next screen" with a "Next" button
- **Screen 2**: Shows "Hello World!" 
- **Touch Navigation**: Touch the "Next" button to navigate from Screen 1 to Screen 2
- **LVGL Integration**: Uses LVGL for modern GUI with touch support
- **GT911 Touch Controller**: Integrated touch input handling
- **Bluetooth Beacon**: Automatically starts advertising as "ESP32S3BOX3" on boot
- **Dual Functionality**: LVGL touch demo runs simultaneously with Bluetooth beacon

## Building and Running

```bash
cd samples/boards/espressif/lcd_lvgl_touch
west build -b esp32s3_box3/esp32s3/procpu
west flash
```

## Hardware Requirements

- ESP32-S3-BOX-3 development board
- 320x240 LCD display (built-in)
- GT911 capacitive touch controller (built-in)
- Bluetooth LE capability (built-in ESP32-S3)

## Expected Behavior

1. **Boot**: Device initializes Bluetooth beacon and shows Screen 1
2. **Bluetooth**: Device starts advertising as "ESP32S3BOX3" beacon
3. **Touch**: Touch the "Next" button to navigate to Screen 2
4. **Screen 2**: Shows "Hello World!" text with green background
5. **Background**: Bluetooth beacon continues running while using the touch interface

## Technical Details

### Display Configuration
- Resolution: 320x240 pixels
- Color depth: 16-bit RGB565
- LVGL integration with automatic display driver

### Touch Configuration  
- GT911 capacitive touch controller
- I2C1 bus at address 0x5D
- Interrupt-driven touch detection on GPIO3
- Integrated with Zephyr input subsystem

### Bluetooth Configuration
- Bluetooth LE beacon functionality
- Device name: "ESP32S3BOX3"
- Non-connectable advertising mode
- Starts automatically on boot
- Runs in background alongside LVGL

### Memory Configuration
- LVGL memory pool: 32KB (optimized)
- Heap size: 64KB (optimized)
- Optimized for dual LVGL + Bluetooth operation

## Code Structure

- `main.c`: Main application with LVGL setup, screen management, and Bluetooth initialization
- `bt.c`: Bluetooth beacon initialization and advertising setup
- `bt.h`: Bluetooth function declarations
- `create_screen1()`: Creates welcome screen with Next button
- `create_screen2()`: Creates second screen with different message
- `input_callback()`: Handles GT911 touch events and navigation
- `next_btn_event_cb()`: LVGL button click handler
- `init_bluetooth()`: Initializes and starts Bluetooth beacon

## Customization

You can easily modify:
- Text messages in `create_screen1()` and `create_screen2()`
- Colors by changing `lv_color_make()` values
- Button size and position
- Bluetooth device name in `bt.c`
- Bluetooth advertising data
- Add more screens and navigation logic

## Bluetooth Beacon Details

The device advertises as a Bluetooth LE beacon with:
- **Name**: ESP32S3BOX3
- **Type**: Non-connectable beacon
- **UUID**: Device Information Service (0x180A)
- **Flags**: General discoverable, BR/EDR not supported

You can scan for the beacon using any Bluetooth scanner app on your phone or computer.
