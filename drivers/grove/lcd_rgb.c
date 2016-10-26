/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <nanokernel.h>
#include <init.h>
#include <stdbool.h>

#include <i2c.h>
#include <display/grove_lcd.h>
#include <misc/util.h>

#define SYS_LOG_DOMAIN "Grove LCD"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_GROVE_LEVEL
#include <misc/sys_log.h>

#define SLEEP_IN_US(_x_)	((_x_) * 1000)

#define GROVE_LCD_DISPLAY_ADDR		(0x3E)
#define GROVE_RGB_BACKLIGHT_ADDR	(0x62)

struct command {
	uint8_t control;
	uint8_t data;
};

struct glcd_data {
	struct device *i2c;
	uint8_t input_set;
	uint8_t display_switch;
	uint8_t function;
};

struct glcd_driver {
	uint16_t	lcd_addr;
	uint16_t	rgb_addr;
};


#define ON	0x1
#define OFF	0x0

/********************************************
 *  LCD FUNCTIONS
 *******************************************/

/* GLCD_CMD_SCREEN_CLEAR has no options */
/* GLCD_CMD_CURSOR_RETURN has no options */

/* Defines for the GLCD_CMD_CURSOR_SHIFT */
#define GLCD_CS_DISPLAY_SHIFT		(1 << 3)
#define GLCD_CS_RIGHT_SHIFT		(1 << 2)

/* LCD Display Commands */
#define GLCD_CMD_SCREEN_CLEAR		(1 << 0)
#define GLCD_CMD_CURSOR_RETURN		(1 << 1)
#define GLCD_CMD_INPUT_SET		(1 << 2)
#define GLCD_CMD_DISPLAY_SWITCH		(1 << 3)
#define GLCD_CMD_CURSOR_SHIFT		(1 << 4)
#define GLCD_CMD_FUNCTION_SET		(1 << 5)
#define GLCD_CMD_SET_CGRAM_ADDR		(1 << 6)
#define GLCD_CMD_SET_DDRAM_ADDR		(1 << 7)


/********************************************
 *  RGB FUNCTIONS
 *******************************************/

#define REGISTER_POWER  0x08
#define REGISTER_R	0x04
#define REGISTER_G	0x03
#define REGISTER_B	0x02

static uint8_t color_define[][3] = {
	{ 255, 255, 255 },	/* white */
	{ 255, 0,   0   },      /* red */
	{ 0,   255, 0   },      /* green */
	{ 0,   0,   255 },      /* blue */
};


/********************************************
 *  PRIVATE FUNCTIONS
 *******************************************/
static void _rgb_reg_set(struct device * const i2c, uint8_t addr, uint8_t dta)
{
	uint8_t data[2] = { addr, dta };

	i2c_write(i2c, data, sizeof(data), GROVE_RGB_BACKLIGHT_ADDR);
}


static inline void _sleep(uint32_t sleep_in_ms)
{
	sys_thread_busy_wait(SLEEP_IN_US(sleep_in_ms));
}


/********************************************
 *  PUBLIC FUNCTIONS
 *******************************************/
void glcd_print(struct device *port, unsigned char *data, uint32_t size)
{
	const struct glcd_driver * const rom = (struct glcd_driver *)
						port->config->config_info;
	struct glcd_data *dev = port->driver_data;
	uint8_t buf[] = { GLCD_CMD_SET_CGRAM_ADDR, 0 };
	int i;

	for (i = 0; i < size; i++) {
		buf[1] = data[i];
		i2c_write(dev->i2c, buf, sizeof(buf), rom->lcd_addr);
	}
}


void glcd_cursor_pos_set(struct device *port, uint8_t col, uint8_t row)
{
	const struct glcd_driver * const rom = (struct glcd_driver *)
						port->config->config_info;
	struct glcd_data *dev = port->driver_data;

	unsigned char data[2];

	if (row == 0) {
		col |= 0x80;
	} else {
		col |= 0xC0;
	}

	data[0] = GLCD_CMD_SET_DDRAM_ADDR;
	data[1] = col;

	i2c_write(dev->i2c, data, 2, rom->lcd_addr);
}


void glcd_clear(struct device *port)
{
	const struct glcd_driver * const rom = (struct glcd_driver *)
						port->config->config_info;
	struct glcd_data *dev = port->driver_data;
	uint8_t clear[] = { 0, GLCD_CMD_SCREEN_CLEAR };

	i2c_write(dev->i2c, clear, sizeof(clear), rom->lcd_addr);
	SYS_LOG_DBG("clear, delay 20 ms");
	_sleep(20);
}


void glcd_display_state_set(struct device *port, uint8_t opt)
{
	const struct glcd_driver * const rom = (struct glcd_driver *)
						port->config->config_info;
	struct glcd_data *dev = port->driver_data;
	uint8_t data[] = { 0, 0 };

	dev->display_switch = opt;
	data[1] = (opt | GLCD_CMD_DISPLAY_SWITCH);

	i2c_write(dev->i2c, data, sizeof(data), rom->lcd_addr);

	SYS_LOG_DBG("set display_state options, delay 5 ms");
	_sleep(5);
}


uint8_t glcd_display_state_get(struct device *port)
{
	struct glcd_data *dev = port->driver_data;

	return dev->display_switch;
}


void glcd_input_state_set(struct device *port, uint8_t opt)
{
	const struct glcd_driver * const rom = port->config->config_info;
	struct glcd_data *dev = port->driver_data;
	uint8_t data[] = { 0, 0 };

	dev->input_set = opt;
	data[1] = (opt | GLCD_CMD_INPUT_SET);

	i2c_write(dev->i2c, &dev->input_set, sizeof(dev->input_set),
		  rom->lcd_addr);
	SYS_LOG_DBG("set the input_set, no delay");
}


uint8_t glcd_input_state_get(struct device *port)
{
	struct glcd_data *dev = port->driver_data;

	return dev->input_set;
}


void glcd_color_select(struct device *port, uint8_t color)
{
	if (color > 3) {
		SYS_LOG_WRN("selected color is too high a value");
		return;
	}
	glcd_color_set(port, color_define[color][0],
			     color_define[color][1],
			     color_define[color][2]);
}


void glcd_color_set(struct device *port, uint8_t r, uint8_t g, uint8_t b)
{
	struct glcd_data * const dev = port->driver_data;

	_rgb_reg_set(dev->i2c, REGISTER_R, r);
	_rgb_reg_set(dev->i2c, REGISTER_G, g);
	_rgb_reg_set(dev->i2c, REGISTER_B, b);
}


void glcd_function_set(struct device *port, uint8_t opt)
{
	const struct glcd_driver * const rom = port->config->config_info;
	struct glcd_data *dev = port->driver_data;
	uint8_t data[] = { 0, 0 };

	dev->function = opt;
	data[1] = (opt | GLCD_CMD_FUNCTION_SET);

	i2c_write(dev->i2c, data, sizeof(data), rom->lcd_addr);

	SYS_LOG_DBG("set function options, delay 5 ms");
	_sleep(5);
}


uint8_t glcd_function_get(struct device *port)
{
	struct glcd_data *dev = port->driver_data;

	return dev->function;
}


int glcd_initialize(struct device *port)
{
	struct glcd_data *dev = port->driver_data;
	uint8_t cmd;

	SYS_LOG_DBG("initialize called");

	dev->input_set = 0;
	dev->display_switch = 0;
	dev->function = 0;

	/*
	 * First set up the device driver...
	 * we need a pointer to the I2C device we are bound to
	 */
	dev->i2c = device_get_binding(
			CONFIG_GROVE_LCD_RGB_I2C_MASTER_DEV_NAME);

	if (!dev->i2c) {
		return -EPERM;
	}

	/* Since device_get_binding() will not return any
	 * reference to a driver instance if port->driver_api
	 * is NULL and grove_lcd does not have any API struct,
	 * just populate it with some magic number
	 * so grove_lcd can be referenced.
	 *
	 * Since dev is probably still in registers.
	 * use that to minimize code addition.
	 */
	port->driver_api = (void *)dev;

	/*
	 * Initialization sequence from the data sheet:
	 * 1 - Power on
	 *   - Wait for more than 30 ms AFTER VDD rises to 4.5v
	 * 2 - Send FUNCTION set
	 *   - Wait for 39 us
	 * 3 - Send DISPLAY Control
	 *   - wait for 39 us
	 * 4 - send DISPLAY Clear
	 *   - wait for 1.5 ms
	 * 5 - send ENTRY Mode
	 * 6 - Initialization is done
	 */


	/*
	 * We're here!  Let's just make sure we've had enough time for the
	 * VDD to power on, so pause a little here, 30 ms min, so we go 50
	 */
	SYS_LOG_DBG("delay 50 ms while the VDD powers on");
	_sleep(50);

	/* Configure everything for the display function first */
	cmd = GLCD_CMD_FUNCTION_SET | GLCD_FS_ROWS_2;
	glcd_function_set(port, cmd);

	/* turn the display on - by default no cursor and no blinking */
	cmd = GLCD_DS_DISPLAY_ON | GLCD_DS_CURSOR_OFF | GLCD_DS_BLINK_OFF;

	glcd_display_state_set(port, cmd);

	/* Clear the screen */
	glcd_clear(port);

	/* Initialize to the default text direction for romance languages */
	cmd = GLCD_IS_ENTRY_LEFT | GLCD_IS_SHIFT_DECREMENT;

	glcd_input_state_set(port, cmd);

	/* Now power on the background RGB control */
	SYS_LOG_INF("configuring the RGB background");
	_rgb_reg_set(dev->i2c, 0x00, 0x00);
	_rgb_reg_set(dev->i2c, 0x01, 0x05);
	_rgb_reg_set(dev->i2c, 0x08, 0xAA);

	/* Now set the background color to white */
	SYS_LOG_DBG("background set to white");
	_rgb_reg_set(dev->i2c, REGISTER_R, color_define[GROVE_RGB_WHITE][0]);
	_rgb_reg_set(dev->i2c, REGISTER_G, color_define[GROVE_RGB_WHITE][1]);
	_rgb_reg_set(dev->i2c, REGISTER_B, color_define[GROVE_RGB_WHITE][2]);

	return 0;
}

static const struct glcd_driver grove_lcd_config = {
	.lcd_addr = GROVE_LCD_DISPLAY_ADDR,
	.rgb_addr = GROVE_RGB_BACKLIGHT_ADDR,
};

static struct glcd_data grove_lcd_driver = {
	.i2c = NULL,
	.input_set = 0,
	.display_switch = 0,
	.function = 0,
};

DEVICE_INIT(grove_lcd, GROVE_LCD_NAME, glcd_initialize,
			&grove_lcd_driver, &grove_lcd_config,
			NANOKERNEL, CONFIG_APPLICATION_INIT_PRIORITY);
