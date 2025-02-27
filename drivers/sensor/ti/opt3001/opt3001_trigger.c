/*
 * Copyright (c) 2025 Bang & Olufsen A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_opt3001

#include "opt3001.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(opt3001, CONFIG_SENSOR_LOG_LEVEL);

static void opt3001_thread_cb(const struct device *dev)
{
	const struct opt3001_config *cfg = dev->config;
	struct opt3001_data *dat = dev->data;
	uint16_t reg_cfg;
	int ret;

	/* read result then clear interrupt by reading REG_CONFIG */
	ret = opt3001_reg_read(dat->dev, OPT3001_REG_RESULT, &dat->sample);
	if (ret) {
		LOG_ERR("Failed to read result, ret: %d", ret);
		return;
	}

	ret = opt3001_reg_read(dat->dev, OPT3001_REG_CONFIG, &reg_cfg);
	if (ret) {
		LOG_ERR("Failed to read config, ret: %d", ret);
		return;
	}

	ret = k_mutex_lock(&dat->handler_mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("Failed to lock trigger mutex, ret: %d", ret);
		return;
	}

	if (dat->handler) {
		dat->handler(dat->dev, dat->trigger);
	}

	/* recheck handler in case it was removed during the handler invocation */
	if (dat->handler) {
		/* enable interrupt again */
		gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	}

	ret = k_mutex_unlock(&dat->handler_mutex);
	if (ret) {
		LOG_ERR("Failed to unlock trigger mutex, ret: %d", ret);
	}
}

static void opt3001_isr(const struct device *dev, struct gpio_callback *gpio_cb, uint32_t pins)
{
	struct opt3001_data *dat = CONTAINER_OF(gpio_cb, struct opt3001_data, gpio_cb_int);
	const struct opt3001_config *cfg = dat->dev->config;

	ARG_UNUSED(pins);

	/* disable interrupt from gpio during handling */
	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

#if defined(CONFIG_OPT3001_TRIGGER_OWN_THREAD)
	k_sem_give(&dat->gpio_sem);
#elif defined(CONFIG_OPT3001_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&dat->work_int);
#endif
}

#ifdef CONFIG_OPT3001_TRIGGER_OWN_THREAD
static void opt3001_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct opt3001_data *dat = p1;

	while (1) {
		k_sem_take(&dat->gpio_sem, K_FOREVER);
		opt3001_thread_cb(dat->dev);
	}
}
#endif

#ifdef CONFIG_OPT3001_TRIGGER_GLOBAL_THREAD
static void opt3001_int_work(struct k_work *work)
{
	struct opt3001_data *dat = CONTAINER_OF(work, struct opt3001_data, work_int);

	opt3001_thread_cb(dat->dev);
}
#endif

int opt3001_init_interrupt(const struct device *dev)
{
	const struct opt3001_config *cfg = dev->config;
	struct opt3001_data *dat = dev->data;
	int ret = 0;

	/* setup interrupt if provided */
	if (!cfg->gpio_int.port) {
		LOG_DBG("int-gpios not provided, continuing without support for trigger");
		return 0;
	}

	dat->dev = dev;

	if (!gpio_is_ready_dt(&cfg->gpio_int)) {
		LOG_ERR("Interrupt gpio not ready");
		return -ENODEV;
	}

	ret = k_mutex_init(&dat->handler_mutex);
	if (ret) {
		LOG_ERR("Failed to init trigger mutex, ret: %d", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Failed to configure int-gpios, ret: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);
	if (ret) {
		LOG_ERR("Failed to enable interrupt on int-gpios, ret: %d", ret);
		return ret;
	}

	gpio_init_callback(&dat->gpio_cb_int, opt3001_isr, BIT(cfg->gpio_int.pin));

	ret = gpio_add_callback(cfg->gpio_int.port, &dat->gpio_cb_int);
	if (ret) {
		LOG_ERR("Failed to add callback to int-gpios, ret: %d", ret);
		return ret;
	}

#if defined(CONFIG_OPT3001_TRIGGER_OWN_THREAD)
	k_sem_init(&dat->gpio_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&dat->thread, dat->thread_stack, CONFIG_OPT3001_THREAD_STACK_SIZE,
			opt3001_thread, dat, NULL, NULL,
			K_PRIO_COOP(CONFIG_OPT3001_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&dat->thread, "opt3001_trigger");

#elif defined(CONFIG_OPT3001_TRIGGER_GLOBAL_THREAD)
	k_work_init(&dat->work_int, opt3001_int_work);
#endif

	return 0;
}

int opt3001_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct opt3001_config *cfg = dev->config;
	struct opt3001_data *dat = dev->data;
	uint16_t reg_cfg;
	int ret;

	/* only allow trigger on a device with int-gpios specified */
	if (!cfg->gpio_int.port) {
		return -ENOSYS;
	}

	if (trig->chan != SENSOR_CHAN_LIGHT) {
		return -EINVAL;
	}

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&dat->handler_mutex, K_FOREVER);
	if (ret) {
		LOG_ERR("Failed to lock trigger mutex, ret: %d", ret);
		return ret;
	}

	dat->trigger = trig;
	dat->handler = handler;

	ret = gpio_pin_interrupt_configure_dt(&cfg->gpio_int,
					      handler ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE);
	if (ret) {
		LOG_ERR("Failed to configure gpio interrupt, ret: %d", ret);
		return ret;
	}

	/* enable/disable interrupt on every conversion */
	ret = opt3001_reg_update(dev, OPT3001_REG_LOW_LIMIT, OPT3001_LIMIT_EXPONENT_MASK,
				 handler ? OPT3001_LIMIT_EXPONENT_MASK
					 : OPT3001_LIMIT_EXPONENT_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to enable interrupt on conversions, ret: %d", ret);
		return ret;
	}

	/* clear any asserted interrupt by reading REG_CONFIG */
	ret = opt3001_reg_read(dat->dev, OPT3001_REG_CONFIG, &reg_cfg);
	if (ret) {
		LOG_ERR("Failed to read config, ret: %d", ret);
		return ret;
	}

	ret = k_mutex_unlock(&dat->handler_mutex);
	if (ret) {
		LOG_ERR("Failed to unlock trigger mutex, ret: %d", ret);
		return ret;
	}

	return 0;
}
