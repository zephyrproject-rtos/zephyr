# ESP32-S3-BOX-3 LVGL Touch Demo

This sample demonstrates LVGL GUI with GT911 touch navigation on the ESP32-S3-BOX-3 board.

## Features

- **Screen 1**: Shows "Click Next for next screen" with a "Next" button
- **Screen 2**: Shows "Hello World!" 
- **Touch Navigation**: Touch the "Next" button to navigate from Screen 1 to Screen 2
- **LVGL Integration**: Uses LVGL for modern GUI with touch support
- **GT911 Touch Controller**: Integrated touch input handling

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

## Expected Behavior

2. **Touch**: Touch the "Next" button to navigate to Screen 2
3. **Screen 2**: Shows "Hello World" text with green background

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

### Memory Configuration
- LVGL memory pool: 64KB
- Heap size: 128KB
- Optimized for smooth GUI performance

## Code Structure

- `main.c`: Main application with LVGL setup and screen management
- `create_screen1()`: Creates welcome screen with Next button
- `create_screen2()`: Creates second screen with different message
- `input_callback()`: Handles GT911 touch events and navigation
- `next_btn_event_cb()`: LVGL button click handler

## Customization

You can easily modify:
- Text messages in `create_screen1()` and `create_screen2()`
- Colors by changing `lv_color_make()` values
- Button size and position
- Add more screens and navigation logic
