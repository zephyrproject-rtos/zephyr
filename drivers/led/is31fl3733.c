/*
 * Copyright 2022-2023 Daniel DeGrasse <daniel@degrasse.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT issi_is31fl3733

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>

#include <zephyr/drivers/led/is31fl3733.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(is31fl3733, CONFIG_LED_LOG_LEVEL);

/* IS31FL3733 register definitions */
#define CMD_SEL_REG		0xFD /* Command/page selection reg */
#define CMD_SEL_LED		0x0  /* LED configuration page */
#define CMD_SEL_PWM		0x1  /* PWM configuration page */
#define CMD_SEL_FUNC		0x3  /* Function configuration page */

#define CMD_LOCK_REG		0xFE /* Command selection lock reg */
#define CMD_LOCK_UNLOCK		0xC5 /* Command sel unlock value */

/* IS31FL3733 page specific register definitions */

/* Function configuration page */
#define CONF_REG		0x0 /* configuration register */
#define CONF_REG_SSD_MASK	0x1 /* Software shutdown mask */
#define CONF_REG_SSD_SHIFT	0x0 /* Software shutdown shift */
#define CONF_REG_SYNC_SHIFT	0x6 /* Sync mode shift */
#define CONF_REG_SYNC_MASK	0xC /* Sync mode mask */

#define GLOBAL_CURRENT_CTRL_REG 0x1 /* global current control register */

#define RESET_REG		0x11 /* Reset all registers to POR state */

/* Matrix Layout definitions */
#define IS31FL3733_ROW_COUNT 12
#define IS31FL3733_COL_COUNT 16
#define IS31FL3733_MAX_LED (IS31FL3733_ROW_COUNT * IS31FL3733_COL_COUNT)

/* Max brightness */
#define IS31FL3733_MAX_BRIGHTNESS 100

struct is31fl3733_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec sdb;
	uint8_t current_limit;
	uint8_t sync;
};

struct is31fl3733_data {
	/* Active configuration page */
	uint32_t selected_page;
	/* Scratch buffer, used for bulk controller writes */
	uint8_t scratch_buf[IS31FL3733_MAX_LED + 1];
	/* LED config reg state, IS31FL3733 conf reg is write only */
	uint8_t conf_reg;
};

/* Selects target register page for IS31FL3733. After setting the
 * target page, all I2C writes will use the selected page until the selected
 * page is changed.
 */
static int is31fl3733_select_page(const struct device *dev, uint8_t page)
{
	const struct is31fl3733_config *config = dev->config;
	struct is31fl3733_data *data = dev->data;
	int ret = 0U;

	if (data->selected_page == page) {
		/* No change necessary */
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

static int is31fl3733_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct is31fl3733_config *config = dev->config;
	int ret;
	uint8_t led_brightness = (uint8_t)(((uint32_t)value * 255) / 100);

	if (led >= IS31FL3733_MAX_LED) {
		return -EINVAL;
	}

	/* Configure PWM mode */
	ret = is31fl3733_select_page(dev, CMD_SEL_PWM);
	if (ret < 0) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&config->bus, led, led_brightness);
}

static int is31fl3733_led_on(const struct device *dev, uint32_t led)
{
	return is31fl3733_led_set_brightness(dev, led, IS31FL3733_MAX_BRIGHTNESS);
}

static int is31fl3733_led_off(const struct device *dev, uint32_t led)
{
	return is31fl3733_led_set_brightness(dev, led, 0);
}

static int is31fl3733_led_write_channels(const struct device *dev, uint32_t start_channel,
					 uint32_t num_channels, const uint8_t *buf)
{
	const struct is31fl3733_config *config = dev->config;
	struct is31fl3733_data *data = dev->data;
	int ret = 0U;
	uint8_t *pwm_start;

	if ((start_channel + num_channels) > IS31FL3733_MAX_LED) {
		return -EINVAL;
	}
	pwm_start = data->scratch_buf + start_channel;
	/* Set PWM and LED target registers as first byte of each transfer */
	*pwm_start = start_channel;
	memcpy((pwm_start + 1), buf, num_channels);

	/* Write LED PWM states */
	ret = is31fl3733_select_page(dev, CMD_SEL_PWM);
	if (ret < 0) {
		return ret;
	}
	LOG_HEXDUMP_DBG(pwm_start, (num_channels + 1), "PWM states");

	return i2c_write_dt(&config->bus, pwm_start, num_channels + 1);
}

static int is31fl3733_init(const struct device *dev)
{
	const struct is31fl3733_config *config = dev->config;
	struct is31fl3733_data *data = dev->data;
	int ret = 0U;
	uint8_t dummy;

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

	ret = is31fl3733_select_page(dev, CMD_SEL_FUNC);
	if (ret < 0) {
		return ret;
	}
	/*
	 * read reset reg to reset all registers to POR state,
	 * in case we are booting from a warm reset.
	 */
	ret = i2c_reg_read_byte_dt(&config->bus, RESET_REG, &dummy);
	if (ret < 0) {
		return ret;
	}

	/* Select function page after LED controller reset */
	ret = is31fl3733_select_page(dev, CMD_SEL_FUNC);
	if (ret < 0) {
		return ret;
	}
	/* Set global current control register based off devicetree value */
	ret = i2c_reg_write_byte_dt(&config->bus, GLOBAL_CURRENT_CTRL_REG,
				config->current_limit);
	if (ret < 0) {
		return ret;
	}
	/* As a final step, we exit software shutdown, disabling display
	 * blanking. We also set the LED controller sync mode here.
	 */
	data->conf_reg = (config->sync << CONF_REG_SYNC_SHIFT) | CONF_REG_SSD_MASK;
	ret = i2c_reg_write_byte_dt(&config->bus, CONF_REG, data->conf_reg);
	if (ret < 0) {
		return ret;
	}

	/* Enable all LEDs. We only control LED brightness in this driver. */
	data->scratch_buf[0] = 0x0;
	memset(data->scratch_buf + 1, 0xFF, (IS31FL3733_MAX_LED / 8));
	ret = is31fl3733_select_page(dev, CMD_SEL_LED);
	if (ret < 0) {
		return ret;
	}

	return i2c_write_dt(&config->bus, data->scratch_buf,
			(IS31FL3733_MAX_LED / 8) + 1);
}

/* Custom IS31FL3733 specific APIs */

/**
 * @brief Blanks IS31FL3733 LED display.
 *
 * When blank_en is set, the LED display will be disabled. This can be used for
 * flicker-free display updates, or power saving.
 *
 * @param dev: LED device structure
 * @param blank_en: should blanking be enabled
 * @return 0 on success, or negative value on error.
 */
int is31fl3733_blank(const struct device *dev, bool blank_en)
{
	const struct is31fl3733_config *config = dev->config;
	struct is31fl3733_data *data = dev->data;
	int ret;

	ret = is31fl3733_select_page(dev, CMD_SEL_FUNC);
	if (ret < 0) {
		return ret;
	}

	if (blank_en) {
		data->conf_reg &= ~CONF_REG_SSD_MASK;
	} else {
		data->conf_reg |= CONF_REG_SSD_MASK;
	}

	return i2c_reg_write_byte_dt(&config->bus, CONF_REG, data->conf_reg);
}

/**
 * @brief Sets led current limit
 *
 * Sets the current limit for the LED driver. This is a separate value
 * from per-led brightness, and applies to all LEDs.
 * This value sets the output current limit according
 * to the following formula: (840/R_ISET) * (limit/256)
 * See table 14 of the datasheet for additional details.
 * @param dev: LED device structure
 * @param limit: current limit to apply
 * @return 0 on success, or negative value on error.
 */
int is31fl3733_current_limit(const struct device *dev, uint8_t limit)
{
	const struct is31fl3733_config *config = dev->config;
	int ret;

	ret = is31fl3733_select_page(dev, CMD_SEL_FUNC);
	if (ret < 0) {
		return ret;
	}

	/* Set global current control register */
	return i2c_reg_write_byte_dt(&config->bus, GLOBAL_CURRENT_CTRL_REG, limit);
}

static DEVICE_API(led, is31fl3733_api) = {
	.on = is31fl3733_led_on,
	.off = is31fl3733_led_off,
	.set_brightness = is31fl3733_led_set_brightness,
	.write_channels = is31fl3733_led_write_channels,
};

#define IS31FL3733_DEVICE(n)                                                                       \
	static const struct is31fl3733_config is31fl3733_config_##n = {                            \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.sdb = GPIO_DT_SPEC_INST_GET_OR(n, sdb_gpios, {}),                                 \
		.current_limit = DT_INST_PROP(n, current_limit),                                   \
		.sync = DT_INST_ENUM_IDX(n, sync_mode),						   \
	};                                                                                         \
                                                                                                   \
	static struct is31fl3733_data is31fl3733_data_##n = {                                      \
		.selected_page = CMD_SEL_LED,                                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &is31fl3733_init, NULL, &is31fl3733_data_##n,                     \
			      &is31fl3733_config_##n, POST_KERNEL, CONFIG_LED_INIT_PRIORITY,       \
			      &is31fl3733_api);

DT_INST_FOREACH_STATUS_OKAY(IS31FL3733_DEVICE)
