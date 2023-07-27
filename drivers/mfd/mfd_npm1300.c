/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_npm1300

#include <errno.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/mfd/npm1300.h>

#define TIME_BASE 0x07U
#define MAIN_BASE 0x00U
#define SHIP_BASE 0x0BU
#define GPIO_BASE 0x06U

#define TIME_OFFSET_LOAD  0x03U
#define TIME_OFFSET_TIMER 0x08U

#define MAIN_OFFSET_RESET    0x01U
#define MAIN_OFFSET_SET      0x00U
#define MAIN_OFFSET_CLR      0x01U
#define MAIN_OFFSET_INTENSET 0x02U
#define MAIN_OFFSET_INTENCLR 0x03U

#define SHIP_OFFSET_HIBERNATE 0x00U

#define GPIO_OFFSET_MODE 0x00U

#define TIMER_PRESCALER_MS 16U
#define TIMER_MAX          0xFFFFFFU

#define MAIN_SIZE 0x26U

#define GPIO_MODE_GPOIRQ 5

struct mfd_npm1300_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec host_int_gpios;
	uint8_t pmic_int_pin;
};

struct mfd_npm1300_data {
	struct k_mutex mutex;
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_work work;
	sys_slist_t callbacks;
};

struct event_reg_t {
	uint8_t offset;
	uint8_t mask;
};

static const struct event_reg_t event_reg[NPM1300_EVENT_MAX] = {
	[NPM1300_EVENT_CHG_COMPLETED] = {0x0AU, 0x10U},
	[NPM1300_EVENT_CHG_ERROR] = {0x0AU, 0x20U},
	[NPM1300_EVENT_BATTERY_DETECTED] = {0x0EU, 0x01U},
	[NPM1300_EVENT_BATTERY_REMOVED] = {0x0EU, 0x02U},
	[NPM1300_EVENT_SHIPHOLD_PRESS] = {0x12U, 0x01U},
	[NPM1300_EVENT_WATCHDOG_WARN] = {0x12U, 0x08U},
	[NPM1300_EVENT_VBUS_DETECTED] = {0x16U, 0x01U},
	[NPM1300_EVENT_VBUS_REMOVED] = {0x16U, 0x02U}};

static void gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct mfd_npm1300_data *data = CONTAINER_OF(cb, struct mfd_npm1300_data, gpio_cb);

	k_work_submit(&data->work);
}

static void work_callback(struct k_work *work)
{
	struct mfd_npm1300_data *data = CONTAINER_OF(work, struct mfd_npm1300_data, work);
	uint8_t buf[MAIN_SIZE];
	int ret;

	/* Read all MAIN registers into temporary buffer */
	ret = mfd_npm1300_reg_read_burst(data->dev, MAIN_BASE, 0U, buf, sizeof(buf));
	if (ret < 0) {
		return;
	}

	for (int i = 0; i < NPM1300_EVENT_MAX; i++) {
		int offset = event_reg[i].offset + MAIN_OFFSET_CLR;

		if ((buf[offset] & event_reg[i].mask) != 0U) {
			gpio_fire_callbacks(&data->callbacks, data->dev, BIT(i));

			ret = mfd_npm1300_reg_write(data->dev, MAIN_BASE, offset,
						    event_reg[i].mask);
			if (ret < 0) {
				return;
			}
		}
	}
}

static int mfd_npm1300_init(const struct device *dev)
{
	const struct mfd_npm1300_config *config = dev->config;
	struct mfd_npm1300_data *mfd_data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	k_mutex_init(&mfd_data->mutex);

	mfd_data->dev = dev;

	if (config->host_int_gpios.port != NULL) {
		/* Set specified PMIC pin to be interrupt output */
		ret = mfd_npm1300_reg_write(dev, GPIO_BASE, GPIO_OFFSET_MODE + config->pmic_int_pin,
					    GPIO_MODE_GPOIRQ);
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

		gpio_init_callback(&mfd_data->gpio_cb, gpio_callback,
				   BIT(config->host_int_gpios.pin));

		ret = gpio_add_callback(config->host_int_gpios.port, &mfd_data->gpio_cb);
		if (ret < 0) {
			return ret;
		}

		mfd_data->work.handler = work_callback;

		ret = gpio_pin_interrupt_configure_dt(&config->host_int_gpios,
						      GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

int mfd_npm1300_reg_read_burst(const struct device *dev, uint8_t base, uint8_t offset, void *data,
			       size_t len)
{
	const struct mfd_npm1300_config *config = dev->config;
	uint8_t buff[] = {base, offset};

	return i2c_write_read_dt(&config->i2c, buff, sizeof(buff), data, len);
}

int mfd_npm1300_reg_read(const struct device *dev, uint8_t base, uint8_t offset, uint8_t *data)
{
	return mfd_npm1300_reg_read_burst(dev, base, offset, data, 1U);
}

int mfd_npm1300_reg_write(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data)
{
	const struct mfd_npm1300_config *config = dev->config;
	uint8_t buff[] = {base, offset, data};

	return i2c_write_dt(&config->i2c, buff, sizeof(buff));
}

int mfd_npm1300_reg_write2(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data1,
			   uint8_t data2)
{
	const struct mfd_npm1300_config *config = dev->config;
	uint8_t buff[] = {base, offset, data1, data2};

	return i2c_write_dt(&config->i2c, buff, sizeof(buff));
}

int mfd_npm1300_reg_update(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data,
			   uint8_t mask)
{
	struct mfd_npm1300_data *mfd_data = dev->data;
	uint8_t reg;
	int ret;

	k_mutex_lock(&mfd_data->mutex, K_FOREVER);

	ret = mfd_npm1300_reg_read(dev, base, offset, &reg);

	if (ret == 0) {
		reg = (reg & ~mask) | (data & mask);
		ret = mfd_npm1300_reg_write(dev, base, offset, reg);
	}

	k_mutex_unlock(&mfd_data->mutex);

	return ret;
}

int mfd_npm1300_set_timer(const struct device *dev, uint32_t time_ms)
{
	const struct mfd_npm1300_config *config = dev->config;
	uint8_t buff[5] = {TIME_BASE, TIME_OFFSET_TIMER};
	uint32_t ticks = time_ms / TIMER_PRESCALER_MS;

	if (ticks > TIMER_MAX) {
		return -EINVAL;
	}

	sys_put_be24(ticks, &buff[2]);

	int ret = i2c_write_dt(&config->i2c, buff, sizeof(buff));

	if (ret != 0) {
		return ret;
	}

	return mfd_npm1300_reg_write(dev, TIME_BASE, TIME_OFFSET_LOAD, 1U);
}

int mfd_npm1300_reset(const struct device *dev)
{
	return mfd_npm1300_reg_write(dev, MAIN_BASE, MAIN_OFFSET_RESET, 1U);
}

int mfd_npm1300_hibernate(const struct device *dev, uint32_t time_ms)
{
	int ret = mfd_npm1300_set_timer(dev, time_ms);

	if (ret != 0) {
		return ret;
	}

	return mfd_npm1300_reg_write(dev, SHIP_BASE, SHIP_OFFSET_HIBERNATE, 1U);
}

int mfd_npm1300_add_callback(const struct device *dev, struct gpio_callback *callback)
{
	struct mfd_npm1300_data *data = dev->data;

	/* Enable interrupts for specified events */
	for (int i = 0; i < NPM1300_EVENT_MAX; i++) {
		if ((callback->pin_mask & BIT(i)) != 0U) {
			/* Clear pending interrupt */
			int ret = mfd_npm1300_reg_write(data->dev, MAIN_BASE,
							event_reg[i].offset + MAIN_OFFSET_CLR,
							event_reg[i].mask);

			if (ret < 0) {
				return ret;
			}

			ret = mfd_npm1300_reg_write(data->dev, MAIN_BASE,
						    event_reg[i].offset + MAIN_OFFSET_INTENSET,
						    event_reg[i].mask);
			if (ret < 0) {
				return ret;
			}
		}
	}

	return gpio_manage_callback(&data->callbacks, callback, true);
}

int mfd_npm1300_remove_callback(const struct device *dev, struct gpio_callback *callback)
{
	struct mfd_npm1300_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, false);
}

#define MFD_NPM1300_DEFINE(inst)                                                                   \
	static struct mfd_npm1300_data data_##inst;                                                \
                                                                                                   \
	static const struct mfd_npm1300_config config##inst = {                                    \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.host_int_gpios = GPIO_DT_SPEC_INST_GET_OR(inst, host_int_gpios, {0}),             \
		.pmic_int_pin = DT_INST_PROP_OR(inst, pmic_int_pin, 0),                            \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_npm1300_init, NULL, &data_##inst, &config##inst,           \
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_NPM1300_DEFINE)
