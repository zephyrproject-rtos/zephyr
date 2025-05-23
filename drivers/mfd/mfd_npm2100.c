/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm2100

#include <errno.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/mfd/npm2100.h>

#define EVENTS_SET              0x00U
#define EVENTS_CLR              0x05U
#define INTEN_SET               0x0AU
#define GPIO_CONFIG             0x80U
#define GPIO_USAGE              0x83U
#define TIMER_TASKS_START       0xB0U
#define TIMER_CONFIG            0xB3U
#define TIMER_TARGET            0xB4U
#define TIMER_STATUS            0xB7U
#define SHPHLD_WAKEUP           0xC1U
#define SHPHLD_SHPHLD           0xC2U
#define HIBERNATE_TASKS_HIBER   0xC8U
#define HIBERNATE_TASKS_HIBERPT 0xC9U
#define RESET_TASKS_RESET       0xD0U
#define RESET_BUTTON            0xD2U
#define RESET_PIN               0xD3U
#define RESET_WRITESTICKY       0xDBU
#define RESET_STROBESTICKY      0xDCU

#define SHPHLD_RESISTOR_MASK     0x03U
#define SHPHLD_RESISTOR_PULLUP   0x00U
#define SHPHLD_RESISTOR_NONE     0x01U
#define SHPHLD_RESISTOR_PULLDOWN 0x02U
#define SHPHLD_CURR_MASK         0x0CU
#define SHPHLD_PULL_ENABLE       0x10U

#define WAKEUP_EDGE_FALLING    0x00U
#define WAKEUP_EDGE_RISING     0x01U
#define WAKEUP_HIBERNATE_PIN   0x00U
#define WAKEUP_HIBERNATE_NOPIN 0x02U

#define TIMER_CONFIG_WKUP 3U

#define TIMER_STATUS_IDLE 0U

#define TIMER_PRESCALER_MUL 64ULL
#define TIMER_PRESCALER_DIV 1000ULL
#define TIMER_MAX           0xFFFFFFU

#define EVENTS_SIZE 5U

#define GPIO_USAGE_INTLO      0x01U
#define GPIO_USAGE_INTHI      0x02U
#define GPIO_CONFIG_OUTPUT    0x02U
#define GPIO_CONFIG_OPENDRAIN 0x04U
#define GPIO_CONFIG_PULLUP    0x10U

#define RESET_STICKY_PWRBUT 0x04U

#define SHPHLD_LONGPRESS_SHIP    0
#define SHPHLD_LONGPRESS_DISABLE 1
#define SHPHLD_LONGPRESS_RESET   2

struct mfd_npm2100_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec host_int_gpios;
	gpio_flags_t host_int_flags;
	gpio_pin_t pmic_int_pin;
	gpio_flags_t pmic_int_flags;
	gpio_flags_t shiphold_flags;
	uint8_t shiphold_longpress;
	uint8_t shiphold_current;
	uint8_t shiphold_hibernate_wakeup;
};

struct mfd_npm2100_data {
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_work work;
	sys_slist_t callbacks;
};

struct event_reg_t {
	uint8_t offset;
	uint8_t mask;
};

static const struct event_reg_t event_reg[NPM2100_EVENT_MAX] = {
	[NPM2100_EVENT_SYS_DIETEMP_WARN] = {0x00U, 0x01U},
	[NPM2100_EVENT_SYS_SHIPHOLD_FALL] = {0x00U, 0x02U},
	[NPM2100_EVENT_SYS_SHIPHOLD_RISE] = {0x00U, 0x04U},
	[NPM2100_EVENT_SYS_PGRESET_FALL] = {0x00U, 0x08U},
	[NPM2100_EVENT_SYS_PGRESET_RISE] = {0x00U, 0x10U},
	[NPM2100_EVENT_SYS_TIMER_EXPIRY] = {0x00U, 0x20U},
	[NPM2100_EVENT_ADC_VBAT_READY] = {0x01U, 0x01U},
	[NPM2100_EVENT_ADC_DIETEMP_READY] = {0x01U, 0x02U},
	[NPM2100_EVENT_ADC_DROOP_DETECT] = {0x01U, 0x04U},
	[NPM2100_EVENT_ADC_VOUT_READY] = {0x01U, 0x08U},
	[NPM2100_EVENT_GPIO0_FALL] = {0x02U, 0x01U},
	[NPM2100_EVENT_GPIO0_RISE] = {0x02U, 0x02U},
	[NPM2100_EVENT_GPIO1_FALL] = {0x02U, 0x04U},
	[NPM2100_EVENT_GPIO1_RISE] = {0x02U, 0x08U},
	[NPM2100_EVENT_BOOST_VBAT_WARN] = {0x03U, 0x01U},
	[NPM2100_EVENT_BOOST_VOUT_MIN] = {0x03U, 0x02U},
	[NPM2100_EVENT_BOOST_VOUT_WARN] = {0x03U, 0x04U},
	[NPM2100_EVENT_BOOST_VOUT_DPS] = {0x03U, 0x08U},
	[NPM2100_EVENT_BOOST_VOUT_OK] = {0x03U, 0x10U},
	[NPM2100_EVENT_LDOSW_OCP] = {0x04U, 0x01U},
	[NPM2100_EVENT_LDOSW_VINTFAIL] = {0x04U, 0x02U},
};

static void gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct mfd_npm2100_data *data = CONTAINER_OF(cb, struct mfd_npm2100_data, gpio_cb);
	const struct mfd_npm2100_config *config = data->dev->config;

	if (config->host_int_flags & GPIO_INT_LEVEL_ACTIVE) {
		/* When using level irq, disable until it can be cleared in the work callback */
		gpio_pin_interrupt_configure_dt(&config->host_int_gpios, GPIO_INT_DISABLE);
	}

	k_work_submit(&data->work);
}

static void work_callback(struct k_work *work)
{
	struct mfd_npm2100_data *data = CONTAINER_OF(work, struct mfd_npm2100_data, work);
	const struct mfd_npm2100_config *config = data->dev->config;
	uint8_t buf[EVENTS_SIZE + 1U] = {EVENTS_SET};
	int ret;

	/* Read MAIN SET registers into buffer, leaving space for register address */
	ret = i2c_write_read_dt(&config->i2c, &buf[0], 1U, &buf[1], EVENTS_SIZE);
	if (ret < 0) {
		k_work_submit(&data->work);
		goto enable_irq;
	}

	for (int i = 0; i < NPM2100_EVENT_MAX; i++) {
		if ((buf[event_reg[i].offset + 1U] & event_reg[i].mask) != 0U) {
			gpio_fire_callbacks(&data->callbacks, data->dev, BIT(i));
		}
	}

	/* Write read buffer back to clear registers to clear all processed events */
	buf[0] = EVENTS_CLR;
	ret = i2c_write_dt(&config->i2c, buf, EVENTS_SIZE + 1U);
	if (ret < 0) {
		k_work_submit(&data->work);
		goto enable_irq;
	}

	/* Resubmit handler to queue if interrupt is still active */
	if (gpio_pin_get_dt(&config->host_int_gpios) != 0) {
		k_work_submit(&data->work);
	}

enable_irq:

	if (config->host_int_flags & GPIO_INT_LEVEL_ACTIVE) {
		/* Re-enable irq */
		gpio_pin_interrupt_configure_dt(&config->host_int_gpios, config->host_int_flags);
	}
}

static int config_pmic_int(const struct device *dev)
{
	const struct mfd_npm2100_config *config = dev->config;
	uint8_t usage = GPIO_USAGE_INTHI;
	uint8_t gpio_config = GPIO_CONFIG_OUTPUT;

	if (config->pmic_int_flags & GPIO_ACTIVE_LOW) {
		usage = GPIO_USAGE_INTLO;
	}
	if ((config->pmic_int_flags & GPIO_SINGLE_ENDED) != 0U) {
		gpio_config |= GPIO_CONFIG_OPENDRAIN;
	}
	if (config->pmic_int_flags & GPIO_PULL_UP) {
		gpio_config |= GPIO_CONFIG_PULLUP;
	}

	/* Set specified PMIC pin to be interrupt output */
	int ret = i2c_reg_write_byte_dt(&config->i2c, GPIO_USAGE + config->pmic_int_pin, usage);

	if (ret < 0) {
		return ret;
	}

	/* Configure PMIC output pin */
	return i2c_reg_write_byte_dt(&config->i2c, GPIO_CONFIG + config->pmic_int_pin, gpio_config);
}

static int config_shphold(const struct device *dev)
{
	const struct mfd_npm2100_config *config = dev->config;
	uint8_t reg;
	int ret;

	if (config->shiphold_longpress != SHPHLD_LONGPRESS_SHIP) {
		ret = i2c_reg_write_byte_dt(&config->i2c, RESET_WRITESTICKY, RESET_STICKY_PWRBUT);
		if (ret < 0) {
			return ret;
		}

		ret = i2c_reg_write_byte_dt(&config->i2c, RESET_STROBESTICKY, 1U);
		if (ret < 0) {
			return ret;
		}

		if (config->shiphold_longpress == SHPHLD_LONGPRESS_RESET) {
			ret = i2c_reg_write_byte_dt(&config->i2c, RESET_BUTTON, 0U);
			if (ret < 0) {
				return ret;
			}

			ret = i2c_reg_write_byte_dt(&config->i2c, RESET_PIN, 1U);
			if (ret < 0) {
				return ret;
			}
		}
	}

	reg = config->shiphold_hibernate_wakeup ? WAKEUP_HIBERNATE_PIN : WAKEUP_HIBERNATE_NOPIN;
	if ((config->shiphold_flags & GPIO_ACTIVE_LOW) == 0U) {
		reg |= WAKEUP_EDGE_RISING;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, SHPHLD_WAKEUP, reg);
	if (ret < 0) {
		return ret;
	}

	if ((config->shiphold_flags & GPIO_PULL_UP) != 0U) {
		reg = SHPHLD_RESISTOR_PULLUP;
	} else if ((config->shiphold_flags & GPIO_PULL_DOWN) != 0U) {
		reg = SHPHLD_RESISTOR_PULLDOWN;
	} else {
		reg = SHPHLD_RESISTOR_NONE;
	}
	if (config->shiphold_current != 0U) {
		reg |= FIELD_PREP(SHPHLD_CURR_MASK, (config->shiphold_current - 1U));
		reg |= SHPHLD_PULL_ENABLE;
	}

	return i2c_reg_write_byte_dt(&config->i2c, SHPHLD_SHPHLD, reg);
}

static int mfd_npm2100_init(const struct device *dev)
{
	const struct mfd_npm2100_config *config = dev->config;
	struct mfd_npm2100_data *mfd_data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	mfd_data->dev = dev;

	ret = config_shphold(dev);
	if (ret < 0) {
		return ret;
	}

	if (config->host_int_gpios.port == NULL) {
		return 0;
	}

	ret = config_pmic_int(dev);
	if (ret < 0) {
		return ret;
	}

	/* Configure host interrupt GPIO */
	if (!gpio_is_ready_dt(&config->host_int_gpios)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->host_int_gpios, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&mfd_data->gpio_cb, gpio_callback, BIT(config->host_int_gpios.pin));

	ret = gpio_add_callback_dt(&config->host_int_gpios, &mfd_data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	mfd_data->work.handler = work_callback;

	return gpio_pin_interrupt_configure_dt(&config->host_int_gpios, config->host_int_flags);
}

int mfd_npm2100_set_timer(const struct device *dev, uint32_t time_ms,
			  enum mfd_npm2100_timer_mode mode)
{
	const struct mfd_npm2100_config *config = dev->config;
	uint8_t buff[4] = {TIMER_TARGET};
	uint32_t ticks = (uint32_t)DIV_ROUND_CLOSEST(((uint64_t)time_ms * TIMER_PRESCALER_MUL),
						     TIMER_PRESCALER_DIV);
	uint8_t timer_status;
	int ret;

	if (ticks > TIMER_MAX) {
		return -EINVAL;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, TIMER_STATUS, &timer_status);
	if (ret < 0) {
		return ret;
	}

	if (timer_status != TIMER_STATUS_IDLE) {
		return -EBUSY;
	}

	sys_put_be24(ticks, &buff[1]);

	ret = i2c_write_dt(&config->i2c, buff, sizeof(buff));
	if (ret < 0) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&config->i2c, TIMER_CONFIG, mode);
}

int mfd_npm2100_start_timer(const struct device *dev)
{
	const struct mfd_npm2100_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->i2c, TIMER_TASKS_START, 1U);
}

int mfd_npm2100_reset(const struct device *dev)
{
	const struct mfd_npm2100_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->i2c, RESET_TASKS_RESET, 1U);
}

int mfd_npm2100_hibernate(const struct device *dev, uint32_t time_ms, bool pass_through)
{
	const struct mfd_npm2100_config *config = dev->config;
	int ret;

	if (time_ms > 0) {
		ret = mfd_npm2100_set_timer(dev, time_ms, NPM2100_TIMER_MODE_WAKEUP);
		if (ret < 0) {
			return ret;
		}

		ret = mfd_npm2100_start_timer(dev);
		if (ret < 0) {
			return ret;
		}
	}

	return i2c_reg_write_byte_dt(
		&config->i2c, pass_through ? HIBERNATE_TASKS_HIBERPT : HIBERNATE_TASKS_HIBER, 1U);
}

int mfd_npm2100_add_callback(const struct device *dev, struct gpio_callback *callback)
{
	const struct mfd_npm2100_config *config = dev->config;
	struct mfd_npm2100_data *data = dev->data;

	/* Enable interrupts for specified events */
	for (int i = 0; i < NPM2100_EVENT_MAX; i++) {
		if ((callback->pin_mask & BIT(i)) != 0U) {
			/* Clear pending interrupt */
			int ret = i2c_reg_write_byte_dt(
				&config->i2c, event_reg[i].offset + EVENTS_CLR, event_reg[i].mask);

			if (ret < 0) {
				return ret;
			}

			ret = i2c_reg_write_byte_dt(&config->i2c, event_reg[i].offset + INTEN_SET,
						    event_reg[i].mask);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return gpio_manage_callback(&data->callbacks, callback, true);
}

int mfd_npm2100_remove_callback(const struct device *dev, struct gpio_callback *callback)
{
	struct mfd_npm2100_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, false);
}

#define MFD_NPM2100_DEFINE(inst)                                                                   \
	static struct mfd_npm2100_data data##inst;                                                 \
                                                                                                   \
	static const struct mfd_npm2100_config config##inst = {                                    \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.host_int_gpios = GPIO_DT_SPEC_INST_GET_OR(inst, host_int_gpios, {0}),             \
		.host_int_flags = DT_INST_ENUM_IDX_OR(inst, host_int_type, 0) == 0                 \
					  ? GPIO_INT_EDGE_TO_ACTIVE                                \
					  : GPIO_INT_LEVEL_ACTIVE,                                 \
		.pmic_int_pin = DT_INST_PROP_OR(inst, pmic_int_pin, 0),                            \
		.pmic_int_flags = DT_INST_PROP_OR(inst, pmic_int_flags, 0),                        \
		.shiphold_flags =                                                                  \
			DT_INST_PROP_OR(inst, shiphold_flags, (GPIO_ACTIVE_LOW | GPIO_PULL_UP)),   \
		.shiphold_longpress = DT_INST_ENUM_IDX_OR(inst, shiphold_longpress, 0),            \
		.shiphold_current = DT_INST_ENUM_IDX_OR(inst, shiphold_current, 0),                \
		.shiphold_hibernate_wakeup = DT_INST_PROP_OR(inst, shiphold_hibernate_wakeup, 0),  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_npm2100_init, NULL, &data##inst, &config##inst,            \
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_NPM2100_DEFINE)
