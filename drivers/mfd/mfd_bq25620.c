/*
 * Copyright (c) 2026 Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_bq25620

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/bq25620.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>

#include "mfd_bq25620.h"

LOG_MODULE_REGISTER(mfd_bq25620, CONFIG_MFD_LOG_LEVEL);

#define BQ25620_INTERRUPT DT_ANY_INST_HAS_PROP_STATUS_OKAY(int_gpios)

struct mfd_bq25620_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec int_gpio;
};

struct mfd_bq25620_data {
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_work work;
	struct k_mutex lock;
	sys_slist_t callbacks;
};

#if BQ25620_INTERRUPT
static int mfd_bq25620_write_masks(const struct device *dev, uint32_t events)
{
	const struct mfd_bq25620_config *config = dev->config;
	uint8_t masks[3];

	/* A set mask bit suppresses the interrupt of the event */
	masks[0] = BQ25620_CHG_MASK_0_ALL & ~(uint8_t)events;
	masks[1] = BQ25620_CHG_MASK_1_ALL & ~(uint8_t)(events >> 8);
	masks[2] = BQ25620_FAULT_MASK_0_ALL & ~(uint8_t)(events >> 16);

	return i2c_burst_write_dt(&config->i2c, BQ25620_REG_CHG_MASK_0, masks, sizeof(masks));
}

static uint32_t mfd_bq25620_enabled_events(struct mfd_bq25620_data *data)
{
	struct mfd_bq25620_callback *cb;
	uint32_t events = 0U;

	SYS_SLIST_FOR_EACH_CONTAINER(&data->callbacks, cb, node) {
		events |= cb->events;
	}

	return events;
}

int mfd_bq25620_add_callback(const struct device *dev, struct mfd_bq25620_callback *cb)
{
	const struct mfd_bq25620_config *config = dev->config;
	struct mfd_bq25620_data *data = dev->data;
	int ret;

	if (config->int_gpio.port == NULL) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	(void)sys_slist_find_and_remove(&data->callbacks, &cb->node);
	sys_slist_append(&data->callbacks, &cb->node);

	ret = mfd_bq25620_write_masks(dev, mfd_bq25620_enabled_events(data));

	k_mutex_unlock(&data->lock);

	return ret;
}

int mfd_bq25620_remove_callback(const struct device *dev, struct mfd_bq25620_callback *cb)
{
	const struct mfd_bq25620_config *config = dev->config;
	struct mfd_bq25620_data *data = dev->data;
	int ret;

	if (config->int_gpio.port == NULL) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (!sys_slist_find_and_remove(&data->callbacks, &cb->node)) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	ret = mfd_bq25620_write_masks(dev, mfd_bq25620_enabled_events(data));

	k_mutex_unlock(&data->lock);

	return ret;
}

static void mfd_bq25620_work_handler(struct k_work *work)
{
	struct mfd_bq25620_data *data = CONTAINER_OF(work, struct mfd_bq25620_data, work);
	const struct mfd_bq25620_config *config = data->dev->config;
	struct mfd_bq25620_callback *cb;
	uint8_t flags[3];
	uint32_t events;
	int ret;

	/* The flag registers are cleared on read */
	ret = i2c_burst_read_dt(&config->i2c, BQ25620_REG_CHG_FLAG_0, flags, sizeof(flags));
	if (ret < 0) {
		LOG_ERR("Failed to read flags: %d", ret);
		return;
	}

	events = flags[0] | ((uint32_t)flags[1] << 8) | ((uint32_t)flags[2] << 16);

	k_mutex_lock(&data->lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&data->callbacks, cb, node) {
		if ((cb->events & events) != 0U) {
			cb->handler(data->dev, cb, cb->events & events);
		}
	}

	k_mutex_unlock(&data->lock);
}

static void mfd_bq25620_gpio_callback(const struct device *port, struct gpio_callback *gpio_cb,
				      gpio_port_pins_t pins)
{
	struct mfd_bq25620_data *data = CONTAINER_OF(gpio_cb, struct mfd_bq25620_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	(void)k_work_submit(&data->work);
}

static int mfd_bq25620_init_interrupt(const struct device *dev)
{
	const struct mfd_bq25620_config *config = dev->config;
	struct mfd_bq25620_data *data = dev->data;
	uint8_t flags[3];
	int ret;

	data->dev = dev;
	k_mutex_init(&data->lock);
	sys_slist_init(&data->callbacks);
	k_work_init(&data->work, mfd_bq25620_work_handler);

	if (config->int_gpio.port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("Interrupt GPIO not ready");
		return -ENODEV;
	}

	/* Discard events that happened before the initialization */
	ret = i2c_burst_read_dt(&config->i2c, BQ25620_REG_CHG_FLAG_0, flags, sizeof(flags));
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, mfd_bq25620_gpio_callback, BIT(config->int_gpio.pin));

	ret = gpio_add_callback_dt(&config->int_gpio, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}
#else
int mfd_bq25620_add_callback(const struct device *dev, struct mfd_bq25620_callback *cb)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);

	return -ENOTSUP;
}

int mfd_bq25620_remove_callback(const struct device *dev, struct mfd_bq25620_callback *cb)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);

	return -ENOTSUP;
}
#endif /* BQ25620_INTERRUPT */

static int mfd_bq25620_init(const struct device *dev)
{
	const struct mfd_bq25620_config *config = dev->config;
	uint8_t masks[3] = {
		BQ25620_CHG_MASK_0_ALL,
		BQ25620_CHG_MASK_1_ALL,
		BQ25620_FAULT_MASK_0_ALL,
	};
	uint8_t part_number;
	uint8_t val;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, BQ25620_REG_PART_INFO, &val);
	if (ret < 0) {
		LOG_ERR("Failed to read part information: %d", ret);
		return ret;
	}

	part_number = FIELD_GET(BQ25620_PART_INFO_PN, val);
	if (part_number != BQ25620_PN_BQ25620) {
		LOG_ERR("Unexpected part number: %u", part_number);
		return -ENODEV;
	}

	/*
	 * Disable the I2C watchdog, it would reset the configuration of the
	 * function drivers when the host does not service it.
	 */
	ret = i2c_reg_update_byte_dt(&config->i2c, BQ25620_REG_CHG_CTRL_1,
				     BQ25620_CHG_CTRL_1_WATCHDOG, 0);
	if (ret < 0) {
		return ret;
	}

	/* Events are unmasked when a function driver registers a callback */
	ret = i2c_burst_write_dt(&config->i2c, BQ25620_REG_CHG_MASK_0, masks, sizeof(masks));
	if (ret < 0) {
		return ret;
	}

#if BQ25620_INTERRUPT
	ret = mfd_bq25620_init_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("Interrupt setup failed: %d", ret);
		return ret;
	}
#endif

	return 0;
}

#define MFD_BQ25620_DEFINE(inst)                                                                   \
	static const struct mfd_bq25620_config mfd_bq25620_config_##inst = {                       \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                        \
	};                                                                                         \
                                                                                                   \
	static struct mfd_bq25620_data mfd_bq25620_data_##inst;                                    \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, mfd_bq25620_init, NULL, &mfd_bq25620_data_##inst,              \
			      &mfd_bq25620_config_##inst, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,   \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(MFD_BQ25620_DEFINE)
