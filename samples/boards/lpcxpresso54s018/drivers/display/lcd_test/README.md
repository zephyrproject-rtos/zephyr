# LCD Display Test Sample

This sample demonstrates the LCD controller (LCDC) driver for the LPC54S018
with an ST7701S-based 480x800 RGB LCD panel.

## Overview

The sample performs the following tests:
1. Draws a color pattern with 8 different colored blocks
2. Clears the screen to black
3. Draws a color gradient
4. Draws test rectangles in various colors

## Requirements

- LPC54S018 board with RGB LCD connected
- ST7701S-based LCD panel (480x800 resolution)
- Proper LCD connections as per the pinctrl configuration

## Building and Running

```bash
cd samples/drivers/display/lcd_test
west build -b lpcxpresso54s018
west flash
```

## Expected Output

You should see the following on the LCD display:
1. 8 color blocks (red, green, blue, yellow, cyan, magenta, white, black)
2. Black screen
3. Color gradient from blue to red horizontally, with green vertically
4. Test rectangles in different colors

Console output:
```
[00:00:00.000,000] <inf> lcd_test: LCD Test Application
[00:00:00.000,000] <inf> lcd_test: Display device found
[00:00:00.000,000] <inf> lcd_test: Display turned on
[00:00:00.000,000] <inf> lcd_test: Drawing color pattern...
```