# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_FT800 ft8xx)
add_subdirectory_ifdef(CONFIG_GROVE_LCD_RGB grove_lcd_rgb)
