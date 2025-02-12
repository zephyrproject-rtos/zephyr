/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include "mcp9808.h"

LOG_MODULE_DECLARE(MCP9808, CONFIG_SENSOR_LOG_LEVEL);

int mcp9808_attr_set(const struct device *dev, enum sensor_channel chan,
		     enum sensor_attribute attr,
		     const struct sensor_value *val)
{
	const struct mcp9808_config *cfg = dev->config;
	uint8_t reg_addr;
	int temp;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (!cfg->int_gpio.port) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		reg_addr = MCP9808_REG_LOWER_LIMIT;
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		reg_addr = MCP9808_REG_UPPER_LIMIT;
		break;
	default:
		return -EINVAL;
	}

	/* Convert temperature to a signed scaled value, then write
	 * the 12-bit 2s complement-plus-sign-bit register value.
	 */
	temp = val->val1 * MCP9808_TEMP_SCALE_CEL;
	temp += (MCP9808_TEMP_SCALE_CEL * val->val2) / 1000000;

	return mcp9808_reg_write_16bit(dev, reg_addr,
				 mcp9808_temp_reg_from_signed(temp));
}

static inline void setup_int(const struct device *dev,
			     bool enable)
{
	const struct mcp9808_config *cfg = dev->config;
	unsigned int flags = enable
		? GPIO_INT_EDGE_TO_ACTIVE
		: GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, flags);
}

static void handle_int(const struct device *dev)
{
	struct mcp9808_data *data = dev->data;

	setup_int(dev, false);

#if defined(CONFIG_MCP9808_TRIGGER_OWN_THREAD)
	k_sem_give(&data->sem);
#elif defined(CONFIG_MCP9808_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void process_int(const struct device *dev)
{
	struct mcp9808_data *data = dev->data;

	if (data->trigger_handler) {
		data->trigger_handler(dev, data->trig);
	}

	if (data->trigger_handler) {
		setup_int(dev, true);
	}
}

int mcp9808_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct mcp9808_data *data = dev->data;
	const struct mcp9808_config *cfg = dev->config;
	int rv = 0;

	if (!cfg->int_gpio.port) {
		return -ENOTSUP;
	}

	setup_int(dev, false);

	data->trig = trig;
	data->trigger_handler = handler;

	if (handler != NULL) {
		setup_int(dev, true);

		rv = gpio_pin_get_dt(&cfg->int_gpio);
		if (rv > 0) {
			handle_int(dev);
			rv = 0;
		}
	}

	return rv;
}

static void alert_cb(const struct device *dev, struct gpio_callback *cb,
		     uint32_t pins)
{
	struct mcp9808_data *data =
		CONTAINER_OF(cb, struct mcp9808_data, alert_cb);

	ARG_UNUSED(pins);

	handle_int(data->dev);
}

#ifdef CONFIG_MCP9808_TRIGGER_OWN_THREAD

static void mcp9808_thread_main(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct mcp9808_data *data = p1;

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);
		process_int(data->dev);
	}
}

static K_KERNEL_STACK_DEFINE(mcp9808_thread_stack, CONFIG_MCP9808_THREAD_STACK_SIZE);
static struct k_thread mcp9808_thread;
#else /* CONFIG_MCP9808_TRIGGER_GLOBAL_THREAD */

static void mcp9808_gpio_thread_cb(struct k_work *work)
{
	struct mcp9808_data *data =
		CONTAINER_OF(work, struct mcp9808_data, work);

	process_int(data->dev);
}

#endif /* CONFIG_MCP9808_TRIGGER_GLOBAL_THREAD */

int mcp9808_setup_interrupt(const struct device *dev)
{
	struct mcp9808_data *data = dev->data;
	const struct mcp9808_config *cfg = dev->config;
	int rc = mcp9808_reg_write_16bit(dev, MCP9808_REG_CRITICAL,
				   MCP9808_TEMP_ABS_MASK);
	if (rc == 0) {
		rc = mcp9808_reg_write_16bit(dev, MCP9808_REG_CONFIG,
				       MCP9808_CFG_ALERT_ENA);
	}

	data->dev = dev;

#ifdef CONFIG_MCP9808_TRIGGER_OWN_THREAD
	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&mcp9808_thread, mcp9808_thread_stack,
			CONFIG_MCP9808_THREAD_STACK_SIZE,
			mcp9808_thread_main, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_MCP9808_THREAD_PRIORITY),
			0, K_NO_WAIT);
#else /* CONFIG_MCP9808_TRIGGER_GLOBAL_THREAD */
	data->work.handler = mcp9808_gpio_thread_cb;
#endif /* trigger type */

	if (!gpio_is_ready_dt(&cfg->int_gpio)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	if (rc == 0) {
		rc = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INPUT);
	}

	if (rc == 0) {
		gpio_init_callback(&data->alert_cb, alert_cb, BIT(cfg->int_gpio.pin));

		rc = gpio_add_callback(cfg->int_gpio.port, &data->alert_cb);
	}

	return rc;
}
