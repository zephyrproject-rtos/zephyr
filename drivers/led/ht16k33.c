/*
 * Copyright (c) 2019 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief LED driver for the HT16K33 I2C LED driver with keyscan
 */

#include <i2c.h>
#include <led.h>
#include <zephyr.h>

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(ht16k33);

#include "led_context.h"

/* HT16K33 commands and options */
#define HT16K33_CMD_DISP_DATA_ADDR 0x00

#define HT16K33_CMD_SYSTEM_SETUP   0x20
#define HT16K33_OPT_S              BIT(0)

#define HT16K33_CMD_KEY_DATA_ADDR  0x40

#define HT16K33_CMD_INT_FLAG_ADDR  0x60

#define HT16K33_CMD_DISP_SETUP     0x80
#define HT16K33_OPT_D              BIT(0)
#define HT16K33_OPT_B0             BIT(1)
#define HT16K33_OPT_B1             BIT(2)
#define HT16K33_OPT_BLINK_OFF      0
#define HT16K33_OPT_BLINK_2HZ      HT16K33_OPT_B0
#define HT16K33_OPT_BLINK_1HZ      HT16K33_OPT_B1
#define HT16K33_OPT_BLINK_05HZ     (HT16K33_OPT_B1 | HT16K33_OPT_B0)

#define HT16K33_CMD_ROW_INT_SET    0xa0
#define HT16K33_OPT_ROW_INT        BIT(0)
#define HT16K33_OPT_ACT            BIT(1)
#define HT16K33_OPT_ROW            0
#define HT16K33_OPT_INT_LOW        HT16K33_OPT_ROW_INT
#define HT16K33_OPT_INT_HIGH       (HT16K33_OPT_ACT | HT16K33_OPT_ROW_INT)

#define HT16K33_CMD_DIMMING_SET    0xe0

/* HT16K33 size definitions */
#define HT16K33_DISP_ROWS          16
#define HT16K33_DISP_COLS          8
#define HT16K33_DISP_DATA_SIZE     HT16K33_DISP_ROWS
#define HT16K33_DISP_SEGMENTS      (HT16K33_DISP_ROWS * HT16K33_DISP_COLS)
#define HT16K33_DIMMING_LEVELS     16

struct ht16k33_cfg {
	char *i2c_dev_name;
	u16_t i2c_addr;
};

struct ht16k33_data {
	struct device *i2c;
	struct led_data dev_data;
	 /* Shadow buffer for the display data RAM */
	u8_t buffer[HT16K33_DISP_DATA_SIZE];
};

static int ht16k33_led_blink(struct device *dev, u32_t led,
			     u32_t delay_on, u32_t delay_off)
{
	/* The HT16K33 blinks all LEDs at the same frequency */
	ARG_UNUSED(led);

	const struct ht16k33_cfg *config = dev->config->config_info;
	struct ht16k33_data *data = dev->driver_data;
	struct led_data *dev_data = &data->dev_data;
	u32_t period;
	u8_t cmd;

	period = delay_on + delay_off;
	if (period < dev_data->min_period || period > dev_data->max_period) {
		return -EINVAL;
	}

	cmd = HT16K33_CMD_DISP_SETUP | HT16K33_OPT_D;
	if (delay_off == 0) {
		cmd |= HT16K33_OPT_BLINK_OFF;
	} else if (period > 1500)  {
		cmd |= HT16K33_OPT_BLINK_05HZ;
	} else if (period > 750)  {
		cmd |= HT16K33_OPT_BLINK_1HZ;
	} else {
		cmd |= HT16K33_OPT_BLINK_2HZ;
	}

	if (i2c_write(data->i2c, &cmd, 1, config->i2c_addr)) {
		LOG_ERR("Setting HT16K33 blink frequency failed");
		return -EIO;
	}

	return 0;
}

static int ht16k33_led_set_brightness(struct device *dev, u32_t led,
				      u8_t value)
{
	ARG_UNUSED(led);

	const struct ht16k33_cfg *config = dev->config->config_info;
	struct ht16k33_data *data = dev->driver_data;
	struct led_data *dev_data = &data->dev_data;
	u8_t dim;
	u8_t cmd;

	if (value < dev_data->min_brightness ||
	    value > dev_data->max_brightness) {
		return -EINVAL;
	}

	dim = (value * (HT16K33_DIMMING_LEVELS - 1)) / dev_data->max_brightness;
	cmd = HT16K33_CMD_DIMMING_SET | dim;

	if (i2c_write(data->i2c, &cmd, 1, config->i2c_addr)) {
		LOG_ERR("Setting HT16K33 brightness failed");
		return -EIO;
	}

	return 0;
}

static int ht16k33_led_set_state(struct device *dev, u32_t led, bool on)
{
	const struct ht16k33_cfg *config = dev->config->config_info;
	struct ht16k33_data *data = dev->driver_data;
	u8_t cmd[2];
	u8_t addr;
	u8_t bit;

	if (led >= HT16K33_DISP_SEGMENTS) {
		return -EINVAL;
	}

	addr = led / HT16K33_DISP_COLS;
	bit = led % HT16K33_DISP_COLS;

	cmd[0] = HT16K33_CMD_DISP_DATA_ADDR | addr;
	if (on) {
		cmd[1] = data->buffer[addr] | BIT(bit);
	} else {
		cmd[1] = data->buffer[addr] & ~BIT(bit);
	}

	if (data->buffer[addr] == cmd[1]) {
		return 0;
	}

	if (i2c_write(data->i2c, cmd, sizeof(cmd), config->i2c_addr)) {
		LOG_ERR("Setting HT16K33 LED %s failed", on ? "on" : "off");
		return -EIO;
	}

	data->buffer[addr] = cmd[1];

	return 0;
}

static int ht16k33_led_on(struct device *dev, u32_t led)
{
	return ht16k33_led_set_state(dev, led, true);
}

static int ht16k33_led_off(struct device *dev, u32_t led)
{
	return ht16k33_led_set_state(dev, led, false);
}

static int ht16k33_init(struct device *dev)
{
	const struct ht16k33_cfg *config = dev->config->config_info;
	struct ht16k33_data *data = dev->driver_data;
	struct led_data *dev_data = &data->dev_data;
	u8_t cmd[1 + HT16K33_DISP_DATA_SIZE]; /* 1 byte command + data */
	int err;

	data->i2c = device_get_binding(config->i2c_dev_name);
	if (data->i2c == NULL) {
		LOG_ERR("Failed to get I2C device");
		return -EINVAL;
	}

	memset(&data->buffer, 0, sizeof(data->buffer));

	/* Hardware specific limits */
	dev_data->min_period = 0U;
	dev_data->max_period = 2000U;
	dev_data->min_brightness = 0U;
	dev_data->max_brightness = 100U;

	/* System oscillator on */
	cmd[0] = HT16K33_CMD_SYSTEM_SETUP | HT16K33_OPT_S;
	err = i2c_write(data->i2c, cmd, 1, config->i2c_addr);
	if (err) {
		LOG_ERR("Enabling HT16K33 system oscillator failed (err %d)",
			err);
		return -EIO;
	}

	/* Clear display RAM */
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = HT16K33_CMD_DISP_DATA_ADDR;
	err = i2c_write(data->i2c, cmd, sizeof(cmd), config->i2c_addr);
	if (err) {
		LOG_ERR("Clearing HT16K33 display RAM failed (err %d)", err);
		return -EIO;
	}

	/* Full brightness */
	cmd[0] = HT16K33_CMD_DIMMING_SET | 0x0f;
	err = i2c_write(data->i2c, cmd, 1, config->i2c_addr);
	if (err) {
		LOG_ERR("Setting HT16K33 brightness failed (err %d)", err);
		return -EIO;
	}

	/* Display on, blinking off */
	cmd[0] = HT16K33_CMD_DISP_SETUP | HT16K33_OPT_D | HT16K33_OPT_BLINK_OFF;
	err = i2c_write(data->i2c, cmd, 1, config->i2c_addr);
	if (err) {
		LOG_ERR("Enabling HT16K33 display failed (err %d)", err);
		return -EIO;
	}

	return 0;
}

static const struct led_driver_api ht16k33_leds_api = {
	.blink = ht16k33_led_blink,
	.set_brightness = ht16k33_led_set_brightness,
	.on = ht16k33_led_on,
	.off = ht16k33_led_off,
};

#define HT16K33_DEVICE(id)						\
	static const struct ht16k33_cfg ht16k33_##id##_cfg = {		\
		.i2c_dev_name = DT_HOLTEK_HT16K33_##id##_BUS_NAME,	\
		.i2c_addr     = DT_HOLTEK_HT16K33_##id##_BASE_ADDRESS,	\
	};								\
									\
static struct ht16k33_data ht16k33_##id##_data;				\
									\
DEVICE_AND_API_INIT(ht16k33_##id, DT_HOLTEK_HT16K33_##id##_LABEL,	\
		    &ht16k33_init, &ht16k33_##id##_data,		\
		    &ht16k33_##id##_cfg, POST_KERNEL,			\
		    CONFIG_LED_INIT_PRIORITY, &ht16k33_leds_api)

/* Support up to eight HT16K33 devices */

#ifdef DT_HOLTEK_HT16K33_0
HT16K33_DEVICE(0);
#endif

#ifdef DT_HOLTEK_HT16K33_1
HT16K33_DEVICE(1);
#endif

#ifdef DT_HOLTEK_HT16K33_2
HT16K33_DEVICE(2);
#endif

#ifdef DT_HOLTEK_HT16K33_3
HT16K33_DEVICE(3);
#endif

#ifdef DT_HOLTEK_HT16K33_4
HT16K33_DEVICE(4);
#endif

#ifdef DT_HOLTEK_HT16K33_5
HT16K33_DEVICE(5);
#endif

#ifdef DT_HOLTEK_HT16K33_6
HT16K33_DEVICE(6);
#endif

#ifdef DT_HOLTEK_HT16K33_7
HT16K33_DEVICE(7);
#endif
