/*
 * Copyright (c) 2022 Andrei-Edward Popa
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <sys/printk.h>
#include <drivers/i2c.h>

/**
 * @file Sample app using LCD 2004 through I2C.
 */

#define LCD_I2C_ADDR		0x27

#define TOGGLE_DELAY_US		600
#define MSGS_DELAY		2000

#define LCD_COMMAND		0x00
#define LCD_CHARACTER		0x01

#define MAX_LINES		4
#define MAX_CHARS		20

#define LCD_CLEARDISPLAY	0x01
#define LCD_ENTRYMODESET	0x04
#define LCD_DISPLAYCONTROL	0x08
#define LCD_FUNCTIONSET		0x20
#define LCD_ENTRYLEFT		0x02
#define LCD_BLINKON		0x01
#define LCD_DISPLAYON		0x04
#define LCD_4LINE		0x09
#define LCD_BACKLIGHT		0x08
#define LCD_ENABLEBIT		0x04

static inline uint32_t line_cursor_value(uint8_t line)
{
	return 0x80 + (0x14 * (line / 2)) + (0x40 * (line % 2));
}

static inline int i2c_write_byte(const struct device *dev, uint8_t value)
{
	return i2c_write(dev, &value, 1, LCD_I2C_ADDR);
}

static inline void lcd_toggle_enable(const struct device *dev, uint8_t value)
{
	k_usleep(TOGGLE_DELAY_US);
	i2c_write_byte(dev, value | LCD_ENABLEBIT);
	k_usleep(TOGGLE_DELAY_US);
	i2c_write_byte(dev, value & ~LCD_ENABLEBIT);
	k_usleep(TOGGLE_DELAY_US);
}

static inline void lcd_send_byte(const struct device *dev, uint8_t value, uint32_t mode)
{
	uint8_t low = mode | ((value << 4) & 0xF0) | LCD_BACKLIGHT;
	uint8_t high = mode | (value & 0xF0) | LCD_BACKLIGHT;

	i2c_write_byte(dev, high);
	lcd_toggle_enable(dev, high);
	i2c_write_byte(dev, low);
	lcd_toggle_enable(dev, low);
}

static inline void lcd_clear(const struct device *dev)
{
	lcd_send_byte(dev, LCD_CLEARDISPLAY, LCD_COMMAND);
}

static inline void lcd_set_cursor(const struct device *dev, uint8_t line, uint32_t position)
{
	uint32_t value = line_cursor_value(line) + position;

	lcd_send_byte(dev, value, LCD_COMMAND);
}

static inline inline void lcd_char(const struct device *dev, char ch)
{
	lcd_send_byte(dev, ch, LCD_CHARACTER);
}

static inline void lcd_string(const struct device *dev, const char *str)
{
	while (*str != '\0') {
		lcd_char(dev, *str);
		str++;
	}
}

static void lcd_init(const struct device *dev)
{
	lcd_send_byte(dev, 0x03, LCD_COMMAND);
	lcd_send_byte(dev, 0x03, LCD_COMMAND);
	lcd_send_byte(dev, 0x03, LCD_COMMAND);
	lcd_send_byte(dev, 0x02, LCD_COMMAND);

	lcd_send_byte(dev, LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
	lcd_send_byte(dev, LCD_FUNCTIONSET | LCD_4LINE, LCD_COMMAND);
	lcd_send_byte(dev, LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
	lcd_clear(dev);
}

int main(void)
{
	const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	uint32_t num_msgs, position;

	if (!device_is_ready(i2c_dev)) {
		printk("I2C: Device is not ready.\n");
		return 0;
	}

	lcd_init(i2c_dev);

	static const char * const messages[] = {
		"The Zephyr Project", "is a RTOS supporting",
		"multiple hardware", "architectures,",
		"optimized for", "resource constrained",
		"devices, built with", "security in mind."
	};

	num_msgs = ARRAY_SIZE(messages);

	while (1) {
		for (uint32_t index = 0; index < num_msgs; index += MAX_LINES) {
			for (uint8_t line = 0; line < MAX_LINES; line++) {
				position = MAX_CHARS / 2 - strlen(messages[index + line]) / 2;
				lcd_set_cursor(i2c_dev, line, position);
				lcd_string(i2c_dev, messages[index + line]);
			}
			k_msleep(MSGS_DELAY);
			lcd_clear(i2c_dev);
		}
	}

	return 0;
}
