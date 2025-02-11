/*
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Display text strings on HD44780 based 20x4 LCD controller
 * using GPIO for parallel interface on Arduino Due.
 *
 * Datasheet: http://lcd-linux.sourceforge.net/pdfdocs/hd44780.pdf
 *
 * LCD Wiring
 * ----------
 *
 * The wiring for the LCD is as follows:
 * 1 : GND
 * 2 : 5V
 * 3 : Contrast (0-5V)*
 * 4 : RS (Register Select)
 * 5 : R/W (Read Write)       - GROUND THIS PIN
 * 6 : Enable or Strobe
 * 7 : Data Bit 0             - NOT USED
 * 8 : Data Bit 1             - NOT USED
 * 9 : Data Bit 2             - NOT USED
 * 10: Data Bit 3             - NOT USED
 * 11: Data Bit 4
 * 12: Data Bit 5
 * 13: Data Bit 6
 * 14: Data Bit 7
 * 15: LCD Backlight +5V**
 * 16: LCD Backlight GND
 *
 *
 * Arduino Due
 * -----------
 *
 * On Arduino Due:
 * 1. IO_3 is PC28
 * 2. IO_5 is PC25
 * 3. IO_6 is PC24
 * 4. IO_7 is PC23
 * 5. IO_8 is PC22
 * 6. IO_9 is PC21
 *
 * The gpio_atmel_sam3 driver is being used.
 *
 * This sample app display text strings per line & page wise.
 *
 * Every 3 second you should see this repeatedly
 * on display:
 * "
 *     *********************
 *     Arduino Due
 *     20x4 LCD Display
 *     *********************
 *
 *      ------------------
 *     - Zephyr Rocks!
 *     - My super RTOS
 *      ------------------
 *
 *     --------HOME--------
 *     I am home!
 *
 *	--------------------
 */

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>

#if defined(CONFIG_SOC_SAM3X8E)
#define GPIO_NODE DT_NODELABEL(pioc)
#else
#error "Unsupported GPIO driver"
#endif

#if defined(CONFIG_SOC_SAM3X8E)
/* Define GPIO OUT to LCD */
#define GPIO_PIN_PC12_D0		12	/* PC12 - pin 51 */
#define GPIO_PIN_PC13_D1		13	/* PC13 - pin 50 */
#define GPIO_PIN_PC14_D2		14	/* PC14 - pin 49 */
#define GPIO_PIN_PC15_D3		15	/* PC15 - pin 48 */
#define GPIO_PIN_PC24_D4		24	/* PC24 - pin 6 */
#define GPIO_PIN_PC23_D5		23	/* PC23 - pin 7 */
#define GPIO_PIN_PC22_D6		22	/* PC22 - pin 8 */
#define GPIO_PIN_PC21_D7		21	/* PC21 - pin 9 */
#define GPIO_PIN_PC28_RS		28	/* PC28 - pin 3 */
#define GPIO_PIN_PC25_E			25	/* PC25 - pin 5 */
#define GPIO_NAME			"GPIO_"
#endif

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

/* Define some device constants */
#define LCD_WIDTH			20	/* Max char per line */
#define HIGH				1
#define LOW				0
/* in millisecond */
#define	ENABLE_DELAY			10


#define GPIO_PIN_WR(dev, pin, bit)						\
	do {									\
		if (gpio_pin_set_raw((dev), (pin), (bit))) {			\
			printk("Err set " GPIO_NAME "%d! %x\n", (pin), (bit));	\
		}								\
	} while (0)								\


#define GPIO_PIN_CFG(dev, pin, dir)						\
	do {									\
		if (gpio_pin_configure((dev), (pin), (dir))) {			\
			printk("Err cfg " GPIO_NAME "%d! %x\n", (pin), (dir));	\
		}								\
	} while (0)


struct pi_lcd_data {
	uint8_t	disp_func;	/* Display Function */
	uint8_t	disp_cntl;	/* Display Control */
	uint8_t disp_mode;	/* Display Mode */
	uint8_t	cfg_rows;
	uint8_t	row_offsets[4];
};

/* Default Configuration - User can update */
struct pi_lcd_data lcd_data = {
	.disp_func = LCD_4BIT_MODE | LCD_1_LINE | LCD_5x8_DOTS,
	.disp_cntl = 0,
	.disp_mode = 0,
	.cfg_rows = 0,
	.row_offsets = {0x00, 0x00, 0x00, 0x00}
};

void _set_row_offsets(int8_t row0, int8_t row1, int8_t row2, int8_t row3)
{
	lcd_data.row_offsets[0] = row0;
	lcd_data.row_offsets[1] = row1;
	lcd_data.row_offsets[2] = row2;
	lcd_data.row_offsets[3] = row3;
}


void _pi_lcd_toggle_enable(const struct device *gpio_dev)
{
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC25_E, LOW);
	k_msleep(ENABLE_DELAY);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC25_E, HIGH);
	k_msleep(ENABLE_DELAY);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC25_E, LOW);
	k_msleep(ENABLE_DELAY);
}


void _pi_lcd_4bits_wr(const struct device *gpio_dev, uint8_t bits)
{
	/* High bits */
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC24_D4, LOW);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC23_D5, LOW);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC22_D6, LOW);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC21_D7, LOW);
	if ((bits & BIT(4)) == BIT(4)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC24_D4, HIGH);
	}
	if ((bits & BIT(5)) == BIT(5)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC23_D5, HIGH);
	}
	if ((bits & BIT(6)) == BIT(6)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC22_D6, HIGH);
	}
	if ((bits & BIT(7)) == BIT(7)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC21_D7, HIGH);
	}

	/* Toggle 'Enable' pin */
	_pi_lcd_toggle_enable(gpio_dev);

	/* Low bits */
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC24_D4, LOW);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC23_D5, LOW);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC22_D6, LOW);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC21_D7, LOW);
	if ((bits & BIT(0)) == BIT(0)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC24_D4, HIGH);
	}
	if ((bits & BIT(1)) == BIT(1)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC23_D5, HIGH);
	}
	if ((bits & BIT(2)) == BIT(2)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC22_D6, HIGH);
	}
	if ((bits & BIT(3)) == BIT(3)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC21_D7, HIGH);
	}

	/* Toggle 'Enable' pin */
	_pi_lcd_toggle_enable(gpio_dev);
}

void _pi_lcd_8bits_wr(const struct device *gpio_dev, uint8_t bits)
{
	/* High bits */
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC21_D7, LOW);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC22_D6, LOW);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC23_D5, LOW);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC24_D4, LOW);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC15_D3, LOW);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC14_D2, LOW);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC13_D1, LOW);
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC12_D0, LOW);

	/* Low bits */
	if ((bits & BIT(0)) == BIT(0)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC12_D0, HIGH);
	}
	if ((bits & BIT(1)) == BIT(1)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC13_D1, HIGH);
	}
	if ((bits & BIT(2)) == BIT(2)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC14_D2, HIGH);
	}
	if ((bits & BIT(3)) == BIT(3)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC15_D3, HIGH);
	}
	if ((bits & BIT(4)) == BIT(4)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC24_D4, HIGH);
	}
	if ((bits & BIT(5)) == BIT(5)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC23_D5, HIGH);
	}
	if ((bits & BIT(6)) == BIT(6)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC22_D6, HIGH);
	}
	if ((bits & BIT(7)) == BIT(7)) {
		GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC21_D7, HIGH);
	}

	/* Toggle 'Enable' pin */
	_pi_lcd_toggle_enable(gpio_dev);
}

void _pi_lcd_data(const struct device *gpio_dev, uint8_t bits)
{
	if (lcd_data.disp_func & LCD_8BIT_MODE) {
		_pi_lcd_8bits_wr(gpio_dev, bits);
	} else {
		_pi_lcd_4bits_wr(gpio_dev, bits);
	}
}

void _pi_lcd_command(const struct device *gpio_dev, uint8_t bits)
{
	/* mode = False for command */
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC28_RS, LOW);
	_pi_lcd_data(gpio_dev, bits);
}

void _pi_lcd_write(const struct device *gpio_dev, uint8_t bits)
{
	/* mode = True for character */
	GPIO_PIN_WR(gpio_dev, GPIO_PIN_PC28_RS, HIGH);
	_pi_lcd_data(gpio_dev, bits);
}


/*************************
 * USER can use these APIs
 *************************/
/** Home */
void pi_lcd_home(const struct device *gpio_dev)
{
	_pi_lcd_command(gpio_dev, LCD_RETURN_HOME);
	k_sleep(K_MSEC(2));			/* wait for 2ms */
}

/** Set cursor position */
void pi_lcd_set_cursor(const struct device *gpio_dev, uint8_t col,
		       uint8_t row)
{
	size_t max_lines;

	max_lines = ARRAY_SIZE(lcd_data.row_offsets);
	if (row >= max_lines) {
		row = max_lines - 1;	/* Count rows starting w/0 */
	}
	if (row >= lcd_data.cfg_rows) {
		row = lcd_data.cfg_rows - 1;    /* Count rows starting w/0 */
	}
	_pi_lcd_command(gpio_dev, (LCD_SET_DDRAM_ADDR | (col + lcd_data.row_offsets[row])));
}


/** Clear display */
void pi_lcd_clear(const struct device *gpio_dev)
{
	_pi_lcd_command(gpio_dev, LCD_CLEAR_DISPLAY);
	k_sleep(K_MSEC(2));			/* wait for 2ms */
}


/** Display ON */
void pi_lcd_display_on(const struct device *gpio_dev)
{
	lcd_data.disp_cntl |= LCD_DISPLAY_ON;
	_pi_lcd_command(gpio_dev,
			LCD_DISPLAY_CONTROL | lcd_data.disp_cntl);
}

/** Display OFF */
void pi_lcd_display_off(const struct device *gpio_dev)
{
	lcd_data.disp_cntl &= ~LCD_DISPLAY_ON;
	_pi_lcd_command(gpio_dev,
			LCD_DISPLAY_CONTROL | lcd_data.disp_cntl);
}


/** Turns cursor off */
void pi_lcd_cursor_off(const struct device *gpio_dev)
{
	lcd_data.disp_cntl &= ~LCD_CURSOR_ON;
	_pi_lcd_command(gpio_dev,
			LCD_DISPLAY_CONTROL | lcd_data.disp_cntl);
}

/** Turn cursor on */
void pi_lcd_cursor_on(const struct device *gpio_dev)
{
	lcd_data.disp_cntl |= LCD_CURSOR_ON;
	_pi_lcd_command(gpio_dev,
			LCD_DISPLAY_CONTROL | lcd_data.disp_cntl);
}


/** Turn off the blinking cursor */
void pi_lcd_blink_off(const struct device *gpio_dev)
{
	lcd_data.disp_cntl &= ~LCD_BLINK_ON;
	_pi_lcd_command(gpio_dev,
			LCD_DISPLAY_CONTROL | lcd_data.disp_cntl);
}

/** Turn on the blinking cursor */
void pi_lcd_blink_on(const struct device *gpio_dev)
{
	lcd_data.disp_cntl |= LCD_BLINK_ON;
	_pi_lcd_command(gpio_dev,
			LCD_DISPLAY_CONTROL | lcd_data.disp_cntl);
}

/** Scroll the display left without changing the RAM */
void pi_lcd_scroll_left(const struct device *gpio_dev)
{
	_pi_lcd_command(gpio_dev, LCD_CURSOR_SHIFT |
			LCD_DISPLAY_MOVE | LCD_MOVE_LEFT);
}

/** Scroll the display right without changing the RAM */
void pi_lcd_scroll_right(const struct device *gpio_dev)
{
	_pi_lcd_command(gpio_dev, LCD_CURSOR_SHIFT |
			LCD_DISPLAY_MOVE | LCD_MOVE_RIGHT);
}

/** Text that flows from left to right */
void pi_lcd_left_to_right(const struct device *gpio_dev)
{
	lcd_data.disp_mode |= LCD_ENTRY_LEFT;
	_pi_lcd_command(gpio_dev,
			LCD_ENTRY_MODE_SET | lcd_data.disp_cntl);
}

/** Text that flows from right to left */
void pi_lcd_right_to_left(const struct device *gpio_dev)
{
	lcd_data.disp_mode &= ~LCD_ENTRY_LEFT;
	_pi_lcd_command(gpio_dev,
			LCD_ENTRY_MODE_SET | lcd_data.disp_cntl);
}

/** Right justify text from the cursor location */
void pi_lcd_auto_scroll_right(const struct device *gpio_dev)
{
	lcd_data.disp_mode |= LCD_ENTRY_SHIFT_INCREMENT;
	_pi_lcd_command(gpio_dev,
			LCD_ENTRY_MODE_SET | lcd_data.disp_cntl);
}

/** Left justify text from the cursor location */
void pi_lcd_auto_scroll_left(const struct device *gpio_dev)
{
	lcd_data.disp_mode &= ~LCD_ENTRY_SHIFT_INCREMENT;
	_pi_lcd_command(gpio_dev,
			LCD_ENTRY_MODE_SET | lcd_data.disp_cntl);
}

void pi_lcd_string(const struct device *gpio_dev, char *msg)
{
	int i;
	int len = 0;
	uint8_t data;

	len = strlen(msg);
	if (len > LCD_WIDTH) {
		printk("Too long message! len %d %s\n", len, msg);
	}

	for (i = 0; i < len; i++) {
		data = msg[i];
		_pi_lcd_write(gpio_dev, data);
	}
}


/** LCD initialization function */
void pi_lcd_init(const struct device *gpio_dev, uint8_t cols, uint8_t rows,
		 uint8_t dotsize)
{
	if (rows > 1) {
		lcd_data.disp_func |= LCD_2_LINE;
	}
	lcd_data.cfg_rows = rows;

	_set_row_offsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);

	/* For 1 line displays, a 10 pixel high font looks OK */
	if ((dotsize != LCD_5x8_DOTS) && (rows == 1U)) {
		lcd_data.disp_func |= LCD_5x10_DOTS;
	}

	/* SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	 * according to datasheet, we need at least 40ms after power rises
	 * above 2.7V before sending commands. Arduino can turn on way
	 * before 4.5V so we'll wait 50
	 */
	k_sleep(K_MSEC(50));

	/* this is according to the hitachi HD44780 datasheet
	 * figure 23/24, pg 45/46 try to set 4/8 bits mode
	 */
	if (lcd_data.disp_func & LCD_8BIT_MODE) {
		/* 1st try */
		_pi_lcd_command(gpio_dev, 0x30);
		k_sleep(K_MSEC(5));			/* wait for 5ms */

		/* 2nd try */
		_pi_lcd_command(gpio_dev, 0x30);
		k_sleep(K_MSEC(5));			/* wait for 5ms */

		/* 3rd try */
		_pi_lcd_command(gpio_dev, 0x30);
		k_sleep(K_MSEC(1));			/* wait for 1ms */

		/* Set 4bit interface */
		_pi_lcd_command(gpio_dev, 0x30);
	} else {
		/* 1st try */
		_pi_lcd_command(gpio_dev, 0x03);
		k_sleep(K_MSEC(5));			/* wait for 5ms */

		/* 2nd try */
		_pi_lcd_command(gpio_dev, 0x03);
		k_sleep(K_MSEC(5));			/* wait for 5ms */

		/* 3rd try */
		_pi_lcd_command(gpio_dev, 0x03);
		k_sleep(K_MSEC(1));			/* wait for 1ms */

		/* Set 4bit interface */
		_pi_lcd_command(gpio_dev, 0x02);
	}

	/* finally, set # lines, font size, etc. */
	_pi_lcd_command(gpio_dev, (LCD_FUNCTION_SET | lcd_data.disp_func));

	/* turn the display on with no cursor or blinking default */
	lcd_data.disp_cntl = LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF;
	pi_lcd_display_on(gpio_dev);

	/* clear it off */
	pi_lcd_clear(gpio_dev);

	/* Initialize to default text direction */
	lcd_data.disp_mode = LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_DECREMENT;
	/* set the entry mode */
	_pi_lcd_command(gpio_dev, LCD_ENTRY_MODE_SET | lcd_data.disp_mode);
}

int main(void)
{
	const struct device *const gpio_dev = DEVICE_DT_GET(GPIO_NODE);

	if (!device_is_ready(gpio_dev)) {
		printk("Device %s not ready!\n", gpio_dev->name);
		return 0;
	}

	/* Setup GPIO output */
	GPIO_PIN_CFG(gpio_dev, GPIO_PIN_PC25_E, GPIO_OUTPUT);
	GPIO_PIN_CFG(gpio_dev, GPIO_PIN_PC28_RS, GPIO_OUTPUT);
	GPIO_PIN_CFG(gpio_dev, GPIO_PIN_PC12_D0, GPIO_OUTPUT);
	GPIO_PIN_CFG(gpio_dev, GPIO_PIN_PC13_D1, GPIO_OUTPUT);
	GPIO_PIN_CFG(gpio_dev, GPIO_PIN_PC14_D2, GPIO_OUTPUT);
	GPIO_PIN_CFG(gpio_dev, GPIO_PIN_PC15_D3, GPIO_OUTPUT);
	GPIO_PIN_CFG(gpio_dev, GPIO_PIN_PC24_D4, GPIO_OUTPUT);
	GPIO_PIN_CFG(gpio_dev, GPIO_PIN_PC23_D5, GPIO_OUTPUT);
	GPIO_PIN_CFG(gpio_dev, GPIO_PIN_PC22_D6, GPIO_OUTPUT);
	GPIO_PIN_CFG(gpio_dev, GPIO_PIN_PC21_D7, GPIO_OUTPUT);

	printk("LCD Init\n");
	pi_lcd_init(gpio_dev, 20, 4, LCD_5x8_DOTS);

	/* Clear display */
	pi_lcd_clear(gpio_dev);

	while (1) {
		printk("Page 1: message\n");

		pi_lcd_string(gpio_dev, "********************");
		pi_lcd_set_cursor(gpio_dev, 0, 1);
		pi_lcd_string(gpio_dev, "Arduino Due");
		pi_lcd_right_to_left(gpio_dev);
		pi_lcd_set_cursor(gpio_dev, 15, 2);
		pi_lcd_string(gpio_dev, "yalpsiD DCL 4x02");
		pi_lcd_set_cursor(gpio_dev, 19, 3);
		pi_lcd_left_to_right(gpio_dev);
		pi_lcd_string(gpio_dev, "********************");
		k_msleep(MSEC_PER_SEC * 3U);

		/* Clear display */
		pi_lcd_clear(gpio_dev);

		printk("Page 2: message\n");

		pi_lcd_scroll_right(gpio_dev);
		pi_lcd_string(gpio_dev, "-------------------");
		pi_lcd_set_cursor(gpio_dev, 0, 1);
		pi_lcd_scroll_right(gpio_dev);
		pi_lcd_string(gpio_dev, "Zephyr Rocks!");
		pi_lcd_set_cursor(gpio_dev, 0, 2);
		pi_lcd_string(gpio_dev, "My super RTOS");
		pi_lcd_set_cursor(gpio_dev, 0, 3);
		pi_lcd_string(gpio_dev, "-------------------");
		k_msleep(MSEC_PER_SEC * 3U);

		/* Clear display */
		pi_lcd_clear(gpio_dev);

		printk("Page 3: message\n");

		pi_lcd_set_cursor(gpio_dev, 0, 3);
		pi_lcd_string(gpio_dev, "--------------------");
		pi_lcd_home(gpio_dev);
		pi_lcd_string(gpio_dev, "--------HOME--------");
		pi_lcd_set_cursor(gpio_dev, 0, 1);
		pi_lcd_string(gpio_dev, "I am home!");
		pi_lcd_set_cursor(gpio_dev, 0, 2);
		pi_lcd_string(gpio_dev, "");
		k_msleep(MSEC_PER_SEC * 3U);

		/* Clear display */
		pi_lcd_clear(gpio_dev);
	}
	return 0;
}
