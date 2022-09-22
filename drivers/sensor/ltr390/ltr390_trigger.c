/*
 * Copyright (c) 2022 Luke Holt
 *
 * SPDX-Livense-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "ltr390.h"

LOG_MODULE_DECLARE(LTR390, CONFIG_SENSOR_LOG_LEVEL);


/**
 * @brief Converts the raw bytes acquired from the sensor to an ambient light
 *        value in lux.
 *
 * @param cfg Device config structure
 * @param val Container to write the converted value to
 * @param als_raw Buffer containing the raw bytes in LE
 * @return int errno
 */
static inline int ltr390_als_value_to_bytes(
	const struct ltr390_config *cfg,
	const struct sensor_value *val,
	void *als_raw)
{
	uint8_t *bytes = (uint8_t *)als_raw;
	double l;
	uint32_t unsigned_l;

	l = (int)val->val1;
	l += ((int)val->val2) / 1000000;

	if (l < 0) {
		LOG_ERR("Threshold value cannot be negative");
		return -ENOTSUP;
	}

	switch (cfg->resolution) {
	case LTR390_RESOLUTION_20BIT:
		l *= 4;
		break;
	case LTR390_RESOLUTION_19BIT:
		l *= 2;
		break;
	case LTR390_RESOLUTION_18BIT:
		l *= 1;
		break;
	case LTR390_RESOLUTION_17BIT:
		l *= 0.5;
		break;
	case LTR390_RESOLUTION_16BIT:
		l *= 0.25;
		break;
	case LTR390_RESOLUTION_13BIT:
		l *= 0.125;
		break;
	}

	switch (cfg->gain) {
	case LTR390_GAIN_1:
		l *= 1;
		break;
	case LTR390_GAIN_3:
		l *= 3;
		break;
	case LTR390_GAIN_6:
		l *= 6;
		break;
	case LTR390_GAIN_9:
		l *= 9;
		break;
	case LTR390_GAIN_18:
		l *= 18;
		break;
	}

	l /= 0.6;

	unsigned_l = (int)l;

	sys_put_le24(unsigned_l, bytes);

	return 0;
}


/**
 * @brief Converts the raw bytes acquired from the sensor to an UV index value.
 *
 * @param cfg Device config structure
 * @param val Container to write the converted value to
 * @param uvs_raw Buffer containing the raw bytes in LE
 * @return int errno
 */
static inline int ltr390_uvs_value_to_bytes(
	const struct ltr390_config *cfg,
	const struct sensor_value *val,
	void *uvs_raw)
{
	uint8_t *bytes = (uint8_t *)uvs_raw;
	uint32_t unsigned_u;
	double u;

	u = (int)val->val1;
	u += ((int)val->val2) / 1000000;

	if (u < 0) {
		LOG_ERR("Threshold value cannot be negative");
		return -ENOTSUP;
	}

	u *= 2300;

	unsigned_u = (int)u;

	sys_put_le24(unsigned_u, bytes);

	return 0;
}


int ltr390_attr_set(const struct device *dev,
					enum sensor_channel chan,
					enum sensor_attribute attr,
					const struct sensor_value *val)
{
	const struct ltr390_config *cfg = dev->config;
	uint8_t thres_value_bytes[3], thres_registers[3];
	int rc;

	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		rc = ltr390_als_value_to_bytes(cfg, val, thres_value_bytes);
		break;
	case SENSOR_CHAN_UVI:
		rc = ltr390_uvs_value_to_bytes(cfg, val, thres_value_bytes);
		break;
	default:
		return -ENOTSUP;
	}
	if (rc < 0) {
		return 0;
	}

	if (!cfg->int_gpio.port) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		thres_registers[0] = LTR390_THRES_LO_0;
		thres_registers[1] = LTR390_THRES_LO_1;
		thres_registers[2] = LTR390_THRES_LO_2;
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		thres_registers[0] = LTR390_THRES_UP_0;
		thres_registers[1] = LTR390_THRES_UP_1;
		thres_registers[2] = LTR390_THRES_UP_2;
		break;
	default:
		return -EINVAL;
	}

	rc = ltr390_write_register(cfg, thres_registers[0], thres_value_bytes[0]);
	if (rc < 0) {
		return 0;
	}

	rc = ltr390_write_register(cfg, thres_registers[1], thres_value_bytes[1]);
	if (rc < 0) {
		return 0;
	}

	rc = ltr390_write_register(cfg, thres_registers[2], thres_value_bytes[2]);
	if (rc < 0) {
		return 0;
	}

	return 0;
}


static inline void setup_int(const struct device *dev,
							 bool enable)
{
	const struct ltr390_config *cfg = dev->config;
	unsigned int flags = enable
		? GPIO_INT_EDGE_TO_ACTIVE
		: GPIO_INT_DISABLE;

	gpio_pin_interrupt_configure_dt(&cfg->int_gpio, flags);
}


static void handle_int(const struct device *dev)
{
	struct ltr390_data *data = dev->data;

	setup_int(dev, false);

#ifdef CONFIG_LTR390_TRIGGER_OWN_THREAD
	k_sem_give(&data->sem);
#else /* CONFIG_LTR390_TRIGGER_GLOBAL_THREAD */
	k_work_submit(&data->work);
#endif /* trigger type */
}


static void process_int(const struct device *dev)
{
	struct ltr390_data *data = dev->data;

	if (data->trigger_handler) {
		data->trigger_handler(dev, &data->trig);
	}

	if (data->trigger_handler) {
		setup_int(dev, true);
	}
}


int ltr390_trigger_set(const struct device *dev,
					   const struct sensor_trigger *trig,
					   sensor_trigger_handler_t handler)
{
	struct ltr390_data *data = dev->data;
	const struct ltr390_config *cfg = dev->config;
	int rc = 0;

	if (!cfg->int_gpio.port) {
		return -ENOTSUP;
	}

	setup_int(dev, false);

	data->trig = *trig;
	data->trigger_handler = handler;

	if (handler != NULL) {
		setup_int(dev, true);

		rc = gpio_pin_get_dt(&cfg->int_gpio);
		if (rc > 0) {
			handle_int(dev);
			rc = 0;
		}
	}

	/* Light or UVI */
	switch (trig->chan) {
	case SENSOR_CHAN_LIGHT:
		rc = ltr390_write_register(cfg, LTR390_INT_CFG,
			LTR390_IC_ALS_CHAN | LTR390_IC_INT_ENABLE);
		break;
	case SENSOR_CHAN_UVI:
		rc = ltr390_write_register(cfg, LTR390_INT_CFG,
			LTR390_IC_UVS_CHAN | LTR390_IC_INT_ENABLE);
		break;
	default:
		return -ENOTSUP;
	}

	return rc;
}


static void alert_cb(const struct device *dev,
					 struct gpio_callback *cb,
					 uint32_t pins)
{
	struct ltr390_data *data = CONTAINER_OF(cb, struct ltr390_data, alert_cb);

	ARG_UNUSED(pins);

	handle_int(data->dev);
}


#ifdef CONFIG_LTR390_TRIGGER_OWN_THREAD

static void ltr390_thread_main(struct ltr390_data *data)
{
	while (1) {
		k_sem_take(&data->sem, K_FOREVER);
		process_int(data->dev);
	}
}

static K_KERNEL_STACK_DEFINE(ltr390_thread_stack, CONFIG_LTR390_THREAD_STACK_SIZE);
static struct k_thread ltr390_thread;

#else /* CONFIG_LTR390_TRIGGER_GLOBAL_THREAD */

static void ltr390_gpio_thread_cb(struct k_work *work)
{
	struct ltr390_data *data = CONTAINER_OF(work, struct ltr390_data, work);

	process_int(data->dev);
}

#endif /* trigger type */


int ltr390_setup_interrupt(const struct device *dev)
{
	struct ltr390_data *data = dev->data;
	const struct ltr390_config *cfg = dev->config;
	int rc = 0;

	data->dev = dev;

#ifdef CONFIG_LTR390_TRIGGER_OWN_THREAD
	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&ltr390_thread,
					ltr390_thread_stack,
					CONFIG_LTR390_THREAD_STACK_SIZE,
					(k_thread_entry_t)ltr390_thread_main,
					data,
					NULL,
					NULL,
					K_PRIO_COOP(CONFIG_LTR390_THREAD_PRIORITY),
					0,
					K_NO_WAIT);
#else /* CONFIG_LTR390_TRIGGER_GLOBAL_THREAD */
	data->work.handler = ltr390_gpio_thread_cb;
#endif /* trigger type */

	if (!device_is_ready(cfg->int_gpio.port)) {
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
