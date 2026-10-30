/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT raspberrypi_touchscreen_panel_regulator_v2

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rpi_panel_mcu, CONFIG_GPIO_LOG_LEVEL);

/* Register map */
#define REG_ID       0x01
#define REG_POWERON  0x02
#define REG_PWM      0x03

/* REG_POWERON bits */
#define LCD_RESET_BIT BIT(0)
#define CTP_RESET_BIT BIT(1)

/* REG_PWM bits */
#define PWM_BL_ENABLE  BIT(7)
#define PWM_BL_MASK    GENMASK(4, 0)

#define NUM_GPIOS 2

struct rpi_panel_mcu_config {
	/* Must be first */
	struct gpio_driver_config common;
	struct i2c_dt_spec i2c;
	uint8_t bl_default;
};

struct rpi_panel_mcu_data {
	/* Must be first */
	struct gpio_driver_data common;
	struct k_mutex lock;
	/* Shadow of REG_POWERON — both resets asserted at boot */
	uint8_t poweron;
};

/* ------------------------------------------------------------------ */

static int mcu_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct rpi_panel_mcu_config *cfg = dev->config;
	uint8_t buf[2] = {reg, val};

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

/* ------------------------------------------------------------------ */
/* GPIO controller API                                                  */
/* ------------------------------------------------------------------ */

static int rpi_panel_mcu_pin_configure(const struct device *dev,
				       gpio_pin_t pin, gpio_flags_t flags)
{
	if (pin >= NUM_GPIOS) {
		return -EINVAL;
	}
	if (flags & GPIO_INPUT) {
		return -ENOTSUP;
	}
	if (flags & GPIO_OUTPUT) {
		struct rpi_panel_mcu_data *data = dev->data;
		uint8_t val;
		int ret;

		/* Set initial output level from GPIO_OUTPUT_INIT_HIGH/_LOW */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			val = 1;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			val = 0;
		} else if (flags & GPIO_OUTPUT_INIT_LOGICAL) {
			/* GPIO_OUTPUT_INACTIVE with ACTIVE_LOW → logical 0 → hardware HIGH */
			val = (flags & GPIO_ACTIVE_LOW) ? 1 : 0;
		} else {
			return 0;
		}

		k_mutex_lock(&data->lock, K_FOREVER);
		if (val) {
			data->poweron |= BIT(pin);
		} else {
			data->poweron &= ~BIT(pin);
		}
		ret = mcu_write(dev, REG_POWERON, data->poweron);
		k_mutex_unlock(&data->lock);
		return ret;
	}
	return 0;
}

static int rpi_panel_mcu_port_get_raw(const struct device *dev,
				      gpio_port_value_t *value)
{
	struct rpi_panel_mcu_data *data = dev->data;

	*value = data->poweron;
	return 0;
}

static int rpi_panel_mcu_port_set_masked_raw(const struct device *dev,
					     gpio_port_pins_t mask,
					     gpio_port_value_t value)
{
	struct rpi_panel_mcu_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->poweron = (data->poweron & ~(uint8_t)mask) |
			((uint8_t)value & (uint8_t)mask);
	ret = mcu_write(dev, REG_POWERON, data->poweron);
	k_mutex_unlock(&data->lock);
	return ret;
}

static int rpi_panel_mcu_port_set_bits_raw(const struct device *dev,
					   gpio_port_pins_t pins)
{
	return rpi_panel_mcu_port_set_masked_raw(dev, pins, pins);
}

static int rpi_panel_mcu_port_clear_bits_raw(const struct device *dev,
					     gpio_port_pins_t pins)
{
	return rpi_panel_mcu_port_set_masked_raw(dev, pins, 0);
}

static int rpi_panel_mcu_port_toggle_bits(const struct device *dev,
					  gpio_port_pins_t pins)
{
	struct rpi_panel_mcu_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);
	data->poweron ^= (uint8_t)pins;
	ret = mcu_write(dev, REG_POWERON, data->poweron);
	k_mutex_unlock(&data->lock);
	return ret;
}

static int rpi_panel_mcu_pin_interrupt_configure(const struct device *dev,
						 gpio_pin_t pin,
						 enum gpio_int_mode mode,
						 enum gpio_int_trig trig)
{
	return -ENOTSUP;
}

static DEVICE_API(gpio, rpi_panel_mcu_gpio_api) = {
	.pin_configure           = rpi_panel_mcu_pin_configure,
	.port_get_raw            = rpi_panel_mcu_port_get_raw,
	.port_set_masked_raw     = rpi_panel_mcu_port_set_masked_raw,
	.port_set_bits_raw       = rpi_panel_mcu_port_set_bits_raw,
	.port_clear_bits_raw     = rpi_panel_mcu_port_clear_bits_raw,
	.port_toggle_bits        = rpi_panel_mcu_port_toggle_bits,
	.pin_interrupt_configure = rpi_panel_mcu_pin_interrupt_configure,
};

/* ------------------------------------------------------------------ */

static int rpi_panel_mcu_init(const struct device *dev)
{
	const struct rpi_panel_mcu_config *cfg = dev->config;
	struct rpi_panel_mcu_data *data = dev->data;
	int ret;

	k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Read firmware ID to verify this is the correct MCU */
	{
		uint8_t reg = REG_ID;
		uint8_t id  = 0;

		ret = i2c_write_read_dt(&cfg->i2c, &reg, 1, &id, 1);
		if (ret < 0) {
			LOG_ERR("Cannot read MCU REG_ID — wrong I2C bus or device? (%d)", ret);
			return ret;
		}
		LOG_INF("MCU REG_ID = 0x%02x (firmware version)", id);
	}

	data->poweron = 0x00;
	ret = mcu_write(dev, REG_POWERON, data->poweron);
	if (ret < 0) {
		LOG_ERR("Failed to enable panel power rail (%d)", ret);
		return ret;
	}
	k_msleep(50);

	/* Enable backlight at the configured default brightness */
	ret = mcu_write(dev, REG_PWM, PWM_BL_ENABLE | (cfg->bl_default & PWM_BL_MASK));
	if (ret < 0) {
		LOG_ERR("Failed to enable backlight (%d)", ret);
		return ret;
	}

	LOG_INF("RPi panel MCU ready, backlight %d/31", cfg->bl_default);
	return 0;
}

#define RPI_PANEL_MCU_DEFINE(n)                                               \
	static struct rpi_panel_mcu_data rpi_panel_mcu_data_##n;              \
	static const struct rpi_panel_mcu_config rpi_panel_mcu_cfg_##n = {   \
		.common = {                                                   \
			.port_pin_mask =                                      \
				GPIO_PORT_PIN_MASK_FROM_NGPIOS(NUM_GPIOS),    \
		},                                                            \
		.i2c        = I2C_DT_SPEC_INST_GET(n),                       \
		.bl_default = DT_INST_PROP_OR(n, bl_default_brightness, 16), \
	};                                                                    \
	DEVICE_DT_INST_DEFINE(n, rpi_panel_mcu_init, NULL,                   \
			      &rpi_panel_mcu_data_##n,                        \
			      &rpi_panel_mcu_cfg_##n,                         \
			      POST_KERNEL,                                    \
			      CONFIG_GPIO_RPI_PANEL_MCU_INIT_PRIORITY,        \
			      &rpi_panel_mcu_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(RPI_PANEL_MCU_DEFINE)
