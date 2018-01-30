/*
 * SPDX-License-Identifier: Apache-2.0
 */

/* Display text strings on HD44780 based 20x4 LCD controller using I2C.
 * This driver is based on lcd_hd44780 (GPIO) Zephyr driver.
 *
 * The PCF8574A is used as I2C interface to HD44780.
 *
 *
 * LCD Wiring with I2C converter
 * -----------------------------
 *
 * HD44780	PCF8574A
 * RS		P0
 * RW		P1
 * E		P2
 * K		P3
 * D4		P4
 * D5		P5
 * D6		P6
 * D7		P7
 *
 *
 * References:
 * [0] HD44780U (LCD-II) (Dot Matrix Liquid Crystal Display Controller/Driver)
 * [1] PCF8574; PCF8574A Remote 8-bit I/O expander for I2C-bus with interrupt
 *
 *
 * Every 3 seconds you should see this repeatedly on display.
 *
 *	----- TEST: 0 ------
 *	HD44780 using I2C
 *	20x4 LCD Display
 *	--------------------
 *
 *	  ---- TEST: 1 -----
 *	- Zephyr Project!
 *	- RTOS
 *	  ------------------
 *
 *	----- TEST: 2 ------
 *	I am home!
 *
 *	 -------------------
 */

#include <zephyr.h>
#include <string.h>
#include <misc/printk.h>

#include <device.h>
#include <i2c.h>

/* Commands */
#define LCD_CLEAR_DISPLAY		0x01
#define LCD_RETURN_HOME			0x02
#define LCD_ENTRY_MODE_SET		0x04
#define LCD_DISPLAY_CONTROL		0x08
#define LCD_CURSOR_SHIFT		0x10
#define LCD_FUNCTION_SET		0x20
#define LCD_SET_CGRAM_ADDR		0x40
#define LCD_SET_DDRAM_ADDR		0x80

/* Display entry mode */
#define LCD_ENTRY_RIGHT			0x00
#define LCD_ENTRY_LEFT			0x02
#define LCD_ENTRY_SHIFT_INCREMENT	0x01
#define LCD_ENTRY_SHIFT_DECREMENT	0x00

/* Display on/off control */
#define LCD_DISPLAY_ON			0x04
#define LCD_DISPLAY_OFF			0x00
#define LCD_CURSOR_ON			0x02
#define LCD_CURSOR_OFF			0x00
#define LCD_BLINK_ON			0x01
#define LCD_BLINK_OFF			0x00

/* Display/cursor shift */
#define LCD_DISPLAY_MOVE		0x08
#define LCD_CURSOR_MOVE			0x00
#define LCD_MOVE_RIGHT			0x04
#define LCD_MOVE_LEFT			0x00

/* Function set */
#define LCD_8BIT_MODE			0x10
#define LCD_4BIT_MODE			0x00
#define LCD_2_LINE			0x08
#define LCD_1_LINE			0x00
#define LCD_5x10_DOTS			0x04
#define LCD_5x8_DOTS			0x00

/* Backlight */
#define LCD_BACKLIGHT			0x08
#define LCD_NO_BACKLIGHT		0x00

#define LCD_NO_DELAY			0

/* Select operation  */
#define LCD_WRITE_IR			0x00
#define LCD_WRITE_DATA			0x01
#define LCD_READ_BUSY_FLAG		0x10
#define LCD_READ_INTERNAL		0x11

#define LCD_ENABLE_HIGH			0x04
#define LCD_ENABLE_LOW			0x00

/* This is the default address to PCF8574A, PFC8574 uses by default the 0x27
 * address.
 */
#define I2C_SLV_ADDR			0x3f

struct lcd_config {
	u8_t function;
	u8_t control;
	u8_t mode;
	u8_t backlight;
	u8_t row_offsets[4];
	u8_t rows;
	u8_t cols;
};

u8_t lcd_expander_write(struct device *i2c_dev, struct lcd_config *cfg,
		    volatile u8_t value)
{
	struct i2c_msg msg;
	u8_t data;

	data = value | cfg->backlight;

	msg.buf = &data;
	msg.len = 1;
	msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;

	return i2c_transfer(i2c_dev, &msg, 1, I2C_SLV_ADDR);
}

u8_t lcd_pulse_enable(struct device *i2c_dev, struct lcd_config *cfg,
		      u8_t value)
{
	u8_t r;

	/* En high */
	r = lcd_expander_write(i2c_dev, cfg, value | LCD_ENABLE_HIGH);
	if (r < 0)
		return r;
	k_busy_wait(1);

	/* En low */
	r = lcd_expander_write(i2c_dev, cfg, LCD_ENABLE_LOW);
	k_busy_wait(50);

	return r;
}

u8_t lcd_write_nibble(struct device *i2c_dev, struct lcd_config *cfg,
		      u8_t value, u16_t udelay)
{
	u8_t r;

	r = lcd_expander_write(i2c_dev, cfg, value);
	if (r < 0) {
		return r;
	}
	r = lcd_pulse_enable(i2c_dev, cfg, value);

	k_busy_wait(udelay);

	return r;
}

u8_t lcd_command(struct device *i2c_dev, struct lcd_config *cfg, u8_t value,
		 u8_t operation)
{
	u8_t high = (value & 0xf0) | operation;
	u8_t low = ((value << 4) & 0xf0) | operation;
	u8_t ret;

	ret = lcd_write_nibble(i2c_dev, cfg, high, LCD_NO_DELAY);
	if (ret < 0)
		return ret;

	return lcd_write_nibble(i2c_dev, cfg, low, LCD_NO_DELAY);
}

/* User API */

void lcd_clear(struct device *i2c_dev, struct lcd_config *cfg)
{
	lcd_command(i2c_dev, cfg, LCD_CLEAR_DISPLAY, LCD_WRITE_IR);
	k_sleep(2);
}

void lcd_home(struct device *i2c_dev, struct lcd_config *cfg)
{
	lcd_command(i2c_dev, cfg, LCD_RETURN_HOME, LCD_WRITE_IR);
	k_sleep(2);
}

void lcd_display_on(struct device *i2c_dev, struct lcd_config *cfg)
{
	cfg->control |= LCD_DISPLAY_ON;
	lcd_command(i2c_dev, cfg, LCD_DISPLAY_CONTROL | cfg->control,
		    LCD_WRITE_IR);
}

void lcd_display_off(struct device *i2c_dev, struct lcd_config *cfg)
{
	cfg->control &= ~LCD_DISPLAY_ON;
	lcd_command(i2c_dev, cfg, LCD_DISPLAY_CONTROL | cfg->control,
		    LCD_WRITE_IR);
}

void lcd_cursor_on(struct device *i2c_dev, struct lcd_config *cfg)
{
	cfg->control |= LCD_CURSOR_ON;
	lcd_command(i2c_dev, cfg, LCD_DISPLAY_CONTROL | cfg->control,
		    LCD_WRITE_IR);
}

void lcd_cursor_off(struct device *i2c_dev, struct lcd_config *cfg)
{
	cfg->control &= ~LCD_CURSOR_ON;
	lcd_command(i2c_dev, cfg, LCD_DISPLAY_CONTROL | cfg->control,
		    LCD_WRITE_IR);
}

void lcd_blink_on(struct device *i2c_dev, struct lcd_config *cfg)
{
	cfg->control |= LCD_BLINK_ON;
	lcd_command(i2c_dev, cfg, LCD_DISPLAY_CONTROL | cfg->control,
		    LCD_WRITE_IR);
}

void lcd_blink_off(struct device *i2c_dev, struct lcd_config *cfg)
{
	cfg->control &= ~LCD_BLINK_ON;
	lcd_command(i2c_dev, cfg, LCD_DISPLAY_CONTROL | cfg->control,
		    LCD_WRITE_IR);
}

/* Scroll the display left without changing the RAM */
void lcd_scroll_left(struct device *i2c_dev, struct lcd_config *cfg)
{
	lcd_command(i2c_dev, cfg, LCD_CURSOR_SHIFT | LCD_DISPLAY_MOVE |
		    LCD_MOVE_LEFT, LCD_WRITE_IR);
}

/* Scroll the display right without changing the RAM */
void lcd_scroll_right(struct device *i2c_dev, struct lcd_config *cfg)
{
	lcd_command(i2c_dev, cfg, LCD_CURSOR_SHIFT | LCD_DISPLAY_MOVE |
		    LCD_MOVE_RIGHT, LCD_WRITE_IR);
}

/* Text flows from left to right */
void lcd_left_to_right(struct device *i2c_dev, struct lcd_config *cfg)
{
	cfg->mode |= LCD_ENTRY_LEFT;
	lcd_command(i2c_dev, cfg, LCD_ENTRY_MODE_SET | cfg->mode, LCD_WRITE_IR);
}

/* Text flows from right to left */
void lcd_right_to_left(struct device *i2c_dev, struct lcd_config *cfg)
{
	cfg->mode &= ~LCD_ENTRY_LEFT;
	lcd_command(i2c_dev, cfg, LCD_ENTRY_MODE_SET | cfg->mode, LCD_WRITE_IR);
}

/* Right justify text from the cursor location */
void lcd_auto_scroll_right(struct device *i2c_dev, struct lcd_config *cfg)
{
	cfg->mode |= LCD_ENTRY_SHIFT_INCREMENT;
	lcd_command(i2c_dev, cfg, LCD_ENTRY_MODE_SET | cfg->mode, LCD_WRITE_IR);
}

/* Left justify text from the cursor location */
void lcd_auto_scroll_left(struct device *i2c_dev, struct lcd_config *cfg)
{
	cfg->mode &= ~LCD_ENTRY_SHIFT_INCREMENT;
	lcd_command(i2c_dev, cfg, LCD_ENTRY_MODE_SET | cfg->mode, LCD_WRITE_IR);
}

void lcd_print(struct device *i2c_dev, struct lcd_config *cfg, char *msg)
{
	u8_t i;
	u8_t len = strlen(msg);

	if (len > cfg->cols) {
		printk("String to long! len: %d string: %s\n", len, msg);
	}

	for (i = 0; i < len; i++) {
		lcd_command(i2c_dev, cfg, msg[i], LCD_WRITE_DATA);
	}
}

void lcd_set_cursor(struct device *i2c_dev, struct lcd_config *cfg, u8_t col,
		    u8_t row)
{
	if (row >= cfg->rows) {
		row = cfg->rows - 1;
	}
	lcd_command(i2c_dev, cfg, LCD_SET_DDRAM_ADDR |
		    (col + cfg->row_offsets[row]), LCD_WRITE_IR);
}

void lcd_init(struct device *i2c_dev, struct lcd_config *cfg, u8_t cols,
	      u8_t rows, u8_t dotsize)
{
	if (rows > 1) {
		cfg->function = LCD_4BIT_MODE | LCD_2_LINE;
	} else {
		cfg->function = LCD_4BIT_MODE | LCD_1_LINE;
	}
	cfg->rows = rows;
	cfg->cols = cols;

	cfg->row_offsets[0] = 0x00;
	cfg->row_offsets[1] = 0x40;
	cfg->row_offsets[2] = 0x00 + cols;
	cfg->row_offsets[3] = 0x40 + cols;

	if ((dotsize != LCD_5x8_DOTS) && (rows == 1)) {
		cfg->function |= LCD_5x10_DOTS;
	} else {
		cfg->function |= LCD_5x8_DOTS;
	}

	/* LCD 4 bit initialization according ref. [0] page 46.
	 * We are using 4 bits because the I/O Expander for I2C has only 8 bits
	 * and HD44780 has RS, R/W and Enable bits in addition to data.
	 */
	k_sleep(50);

	lcd_write_nibble(i2c_dev, cfg, 0x30, 4200);
	lcd_write_nibble(i2c_dev, cfg, 0x30, 110);
	lcd_write_nibble(i2c_dev, cfg, 0x30, LCD_NO_DELAY);
	lcd_write_nibble(i2c_dev, cfg, 0x20, LCD_NO_DELAY);

	cfg->control = LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF;
	cfg->mode = LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_DECREMENT;
	cfg->backlight = LCD_BACKLIGHT;

	lcd_clear(i2c_dev, cfg);
	lcd_command(i2c_dev, cfg, LCD_FUNCTION_SET | cfg->function,
		    LCD_WRITE_IR);
	lcd_command(i2c_dev, cfg, LCD_DISPLAY_CONTROL | cfg->control,
		    LCD_WRITE_IR);
	lcd_command(i2c_dev, cfg, LCD_ENTRY_MODE_SET | cfg->mode,
		    LCD_WRITE_IR);
}

void main(void)
{
	struct device *i2c_dev;
	struct lcd_config cfg;

	i2c_dev = device_get_binding("I2C_1");
	if (!i2c_dev) {
		return;
	}

	printk("LCD init\n");
	lcd_init(i2c_dev, &cfg, 20, 4, LCD_5x8_DOTS);

	while (1) {
		printk("TEST: 0\n");

		lcd_print(i2c_dev, &cfg, "----- TEST: 0 ------");
		lcd_set_cursor(i2c_dev, &cfg, 0, 1);
		lcd_print(i2c_dev, &cfg, "HD44780 using I2C");
		lcd_right_to_left(i2c_dev, &cfg);
		lcd_set_cursor(i2c_dev, &cfg, 15, 2);
		lcd_print(i2c_dev, &cfg, "yalpsiD DCL 4x02");
		lcd_set_cursor(i2c_dev, &cfg, 19, 3);
		lcd_print(i2c_dev, &cfg, "--------------------");
		k_sleep(MSEC_PER_SEC * 3);

		printk("TEST: 1\n");
		lcd_clear(i2c_dev, &cfg);

		lcd_scroll_right(i2c_dev, &cfg);
		lcd_print(i2c_dev, &cfg, "---- TEST: 1 ------");
		lcd_set_cursor(i2c_dev, &cfg, 0, 1);
		lcd_scroll_right(i2c_dev, &cfg);
		lcd_print(i2c_dev, &cfg, "Zephyr Project!");
		lcd_set_cursor(i2c_dev, &cfg, 0, 2);
		lcd_print(i2c_dev, &cfg, "RTOS");
		lcd_set_cursor(i2c_dev, &cfg, 0, 3);
		lcd_print(i2c_dev, &cfg, "-------------------");
		k_sleep(MSEC_PER_SEC * 3);

		printk("TEST: 2\n");
		lcd_clear(i2c_dev, &cfg);

		lcd_set_cursor(i2c_dev, &cfg, 0, 3);
		lcd_print(i2c_dev, &cfg, "--------------------");
		lcd_home(i2c_dev, &cfg);
		lcd_print(i2c_dev, &cfg, "----- TEST: 2 ------");
		lcd_set_cursor(i2c_dev, &cfg, 0, 1);
		lcd_print(i2c_dev, &cfg, "I am home!");
		lcd_set_cursor(i2c_dev, &cfg, 0, 3);
		lcd_print(i2c_dev, &cfg, " ");
		k_sleep(MSEC_PER_SEC * 3);

		lcd_clear(i2c_dev, &cfg);
	}
}
