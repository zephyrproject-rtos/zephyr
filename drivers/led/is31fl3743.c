/*
 * Copyright 2022-2023 Daniel DeGrasse <daniel@degrasse.com>
 * Copyright 2025-2026 Framework Computer Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT issi_is31fl3743

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/led/is31fl3743.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(is31fl3743, CONFIG_LED_LOG_LEVEL);

/* IS31FL3743A register definitions */
#define CMD_SEL_REG     0xFD /* Command/page selection reg */
#define CMD_SEL_PWM     0x0  /* PWM Register */
#define CMD_SEL_SCALING 0x1  /* Scaling Register */
#define CMD_SEL_FUNC    0x2  /* Function Register */

#define CMD_LOCK_REG    0xFE /* Command selection lock reg  */
#define CMD_LOCK_UNLOCK 0xC5 /* Command sel unlock value  */

/* IS31FL3743A page specific register definitions */

/* Page 2: Function configuration page  */
#define CONF_REG           0x00 /* Configuration register */
#define CONF_REG_SSD_MASK  0x1  /* Software shutdown mask  */
#define CONF_REG_SSD_SHIFT 0x0  /* Software shutdown shift */
#define CONF_REG_SWS_SHIFT 0x4  /* SWx Setting shift */

#define GLOBAL_CURRENT_CTRL_REG 0x01 /* Global current control register */

/* Sync bits are in the Spread Spectrum Register (0x25) on the 3743A  */
#define SPREAD_SPECTRUM_REG 0x25
#define SSP_REG_SYNC_SHIFT  0x6  /* Sync mode shift  */
#define SSP_REG_SYNC_MASK   0xC0 /* Sync mode mask  */

/* Reset all registers to POR state  */
#define RESET_REG   0x2F
#define RESET_MAGIC 0x4F

/* Matrix Layout definition: 11 SW x 18 CS */
#define IS31FL3743_ROW_COUNT 11
#define IS31FL3743_COL_COUNT 18
#define IS31FL3743_MAX_LED   (IS31FL3743_ROW_COUNT * IS31FL3743_COL_COUNT)

/* PWM/Scaling registers start at 0x01, not 0x00 */
#define IS31FL3743_REG_OFFSET 0x01

struct is31fl3743_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec sdb;
	uint8_t current_limit;
	uint8_t current_sources;
	uint8_t sync;
};

struct is31fl3743_data {
	/* Active configuration page */
	uint32_t selected_page;
	/* Scratch buffer, used for bulk controller writes (Max LED + 1 for addr) */
	uint8_t scratch_buf[IS31FL3743_MAX_LED + 1];
};

/* Selects target register page for IS31FL3743. After setting the
 * target page, all I2C writes will use the selected page until the selected
 * page is changed.
 */
static int is31fl3743_select_page(const struct device *dev, uint8_t page)
{
	const struct is31fl3743_config *config = dev->config;
	struct is31fl3743_data *data = dev->data;
	int ret = 0U;

	if (data->selected_page == page) {
		return 0;
	}

	/* Unlock page selection register */
	ret = i2c_reg_write_byte_dt(&config->bus, CMD_LOCK_REG, CMD_LOCK_UNLOCK);
	if (ret < 0) {
		LOG_ERR("Could not unlock page selection register");
		return ret;
	}

	/* Write to function select to select active page */
	ret = i2c_reg_write_byte_dt(&config->bus, CMD_SEL_REG, page);
	if (ret < 0) {
		LOG_ERR("Could not select active page");
		return ret;
	}
	data->selected_page = page;

	return ret;
}

static int is31fl3743_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct is31fl3743_config *config = dev->config;
	int ret;
	uint8_t led_brightness = (uint8_t)(((uint32_t)value * 255) / LED_BRIGHTNESS_MAX);

	if (led >= IS31FL3743_MAX_LED) {
		return -EINVAL;
	}

	/* Configure PWM mode  */
	ret = is31fl3743_select_page(dev, CMD_SEL_PWM);
	if (ret < 0) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&config->bus, led + IS31FL3743_REG_OFFSET, led_brightness);
}

static int is31fl3743_led_on(const struct device *dev, uint32_t led)
{
	return is31fl3743_led_set_brightness(dev, led, 100);
}

static int is31fl3743_led_off(const struct device *dev, uint32_t led)
{
	return is31fl3743_led_set_brightness(dev, led, 0);
}

static int is31fl3743_led_write_channels(const struct device *dev, uint32_t start_channel,
					 uint32_t num_channels, const uint8_t *buf)
{
	const struct is31fl3743_config *config = dev->config;
	struct is31fl3743_data *data = dev->data;
	int ret = 0U;

	if ((start_channel + num_channels) > IS31FL3743_MAX_LED) {
		return -EINVAL;
	}

	/* Set PWM registers as first byte of each transfer. */
	data->scratch_buf[0] = start_channel + IS31FL3743_REG_OFFSET;
	memcpy((data->scratch_buf + 1), buf, num_channels);

	/* Write LED PWM states */
	ret = is31fl3743_select_page(dev, CMD_SEL_PWM);
	if (ret < 0) {
		return ret;
	}
	LOG_HEXDUMP_DBG(data->scratch_buf, (num_channels + 1), "PWM states");

	return i2c_write_dt(&config->bus, data->scratch_buf, num_channels + 1);
}

static int is31fl3743_init(const struct device *dev)
{
	const struct is31fl3743_config *config = dev->config;
	struct is31fl3743_data *data = dev->data;
	int ret = 0U;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}
	if (config->sdb.port != NULL) {
		if (!gpio_is_ready_dt(&config->sdb)) {
			LOG_ERR("GPIO SDB pin not ready");
			return -ENODEV;
		}
		/* Set SDB pin high to exit hardware shutdown */
		ret = gpio_pin_configure_dt(&config->sdb, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	ret = is31fl3743_select_page(dev, CMD_SEL_FUNC);
	if (ret < 0) {
		return ret;
	}
	/*
	 * Write reset reg to reset all registers to POR state,
	 * in case we are booting from a warm reset.
	 */
	ret = i2c_reg_write_byte_dt(&config->bus, RESET_REG, RESET_MAGIC);
	if (ret < 0) {
		return ret;
	}

	k_busy_wait(10 * USEC_PER_MSEC);

	/* Select function page again after reset */
	ret = is31fl3743_select_page(dev, CMD_SEL_FUNC);
	if (ret < 0) {
		return ret;
	}

	/* Set global current control (GCC) register based off devicetree value */
	ret = i2c_reg_write_byte_dt(&config->bus, GLOBAL_CURRENT_CTRL_REG, config->current_limit);
	if (ret < 0) {
		return ret;
	}

	/* Exit software shutdown and configure SWx enablement */
	ret = i2c_reg_write_byte_dt(&config->bus, CONF_REG,
				    CONF_REG_SSD_MASK |
					    (config->current_sources << CONF_REG_SWS_SHIFT));
	if (ret < 0) {
		return ret;
	}

	/* Configure Sync (Spread Spectrum Register 0x25) */
	if (config->sync != 0) {
		ret = i2c_reg_update_byte_dt(&config->bus, SPREAD_SPECTRUM_REG, SSP_REG_SYNC_MASK,
					     (config->sync << SSP_REG_SYNC_SHIFT));
		if (ret < 0) {
			return ret;
		}
	}

	/* Initialize Scaling Registers (Page 1) to 0xFF.
	 * The 3743A default scaling is 0x00, meaning LEDs are off
	 * even if PWM is set. We must set them to max to allow PWM control.
	 */
	data->scratch_buf[0] = IS31FL3743_REG_OFFSET;
	memset(data->scratch_buf + 1, 0xFF, IS31FL3743_MAX_LED);

	ret = is31fl3743_select_page(dev, CMD_SEL_SCALING);
	if (ret < 0) {
		return ret;
	}

	return i2c_write_dt(&config->bus, data->scratch_buf, IS31FL3743_MAX_LED + 1);
}

/* Custom IS31FL3743 specific APIs */

/**
 * @brief Blanks IS31FL3743 LED display.
 *
 * When blank_en is set, the LED display will be disabled. This can be used for
 * flicker-free display updates, or power saving.
 */
int is31fl3743_blank(const struct device *dev, bool blank_en)
{
	const struct is31fl3743_config *config = dev->config;
	uint8_t val = blank_en ? 0 : CONF_REG_SSD_MASK;
	int ret;

	/* Configuration register is on Page 2  */
	ret = is31fl3743_select_page(dev, CMD_SEL_FUNC);
	if (ret < 0) {
		return ret;
	}

	return i2c_reg_update_byte_dt(&config->bus, CONF_REG, CONF_REG_SSD_MASK, val);
}

/**
 * @brief Sets led current limit
 */
int is31fl3743_current_limit(const struct device *dev, uint8_t limit)
{
	const struct is31fl3743_config *config = dev->config;
	int ret;

	/* GCC register is on Page 2 */
	ret = is31fl3743_select_page(dev, CMD_SEL_FUNC);
	if (ret < 0) {
		return ret;
	}

	/* Set global current control register */
	return i2c_reg_write_byte_dt(&config->bus, GLOBAL_CURRENT_CTRL_REG, limit);
}

static DEVICE_API(led, is31fl3743_api) = {
	.on = is31fl3743_led_on,
	.off = is31fl3743_led_off,
	.set_brightness = is31fl3743_led_set_brightness,
	.write_channels = is31fl3743_led_write_channels,
};

#define IS31FL3743_DEVICE(n)                                                                       \
	static const struct is31fl3743_config is31fl3743_config_##n = {                            \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.sdb = GPIO_DT_SPEC_INST_GET_OR(n, sdb_gpios, {}),                                 \
		.current_limit = DT_INST_PROP(n, current_limit),                                   \
		.current_sources = DT_INST_ENUM_IDX(n, current_sources),                           \
		.sync = DT_INST_ENUM_IDX(n, sync_mode),                                            \
	};                                                                                         \
                                                                                                   \
	static struct is31fl3743_data is31fl3743_data_##n = {                                      \
		.selected_page = CMD_SEL_PWM,                                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &is31fl3743_init, NULL, &is31fl3743_data_##n,                     \
			      &is31fl3743_config_##n, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,       \
			      &is31fl3743_api);

DT_INST_FOREACH_STATUS_OKAY(IS31FL3743_DEVICE)
