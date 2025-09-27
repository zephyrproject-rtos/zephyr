/*
 * Copyright 2021 The Chromium OS Authors
 * Copyright 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ina230.h"
#include "ina2xx_common.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ina230.h>
#include <zephyr/dt-bindings/sensor/ina230.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(INA230, CONFIG_SENSOR_LOG_LEVEL);

static void ina230_gpio_callback(const struct device *port, struct gpio_callback *cb, uint32_t pin)
{
	struct ina230_data *ina230 = CONTAINER_OF(cb, struct ina230_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pin);

#if defined(CONFIG_INA230_TRIGGER_OWN_THREAD)
	k_sem_give(&ina230->sem);
#elif defined(CONFIG_INA230_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&ina230->work);
#endif
}

static void ina230_handle_interrupts(const struct device *dev)
{
	const struct ina230_config *config = dev->config;
	struct ina230_data *data = dev->data;
	uint16_t reg;

	if (ina2xx_reg_read_16(&config->common.bus, INA230_REG_MASK, &reg) < 0) {
		return;
	}

	if (data->handler_alert && reg & INA230_ALERT_FUNCTION_FLAG) {
		data->handler_alert(dev, data->trig_alert);
	}

	if (data->handler_cnvr && reg & INA230_CONVERSION_READY_FLAG) {
		data->handler_cnvr(dev, data->trig_cnvr);
	}
}

#ifdef CONFIG_INA230_TRIGGER_GLOBAL_THREAD
static void ina230_work_handler(struct k_work *work)
{
	struct ina230_data *data = CONTAINER_OF(work, struct ina230_data, work);

	ina230_handle_interrupts(data->dev);
}
#endif

#ifdef CONFIG_INA230_TRIGGER_OWN_THREAD
static void ina230_thread_main(void *p1, void *p2, void *p3)
{
	struct ina230_data *data = p1;

	while (1) {
		k_sem_take(&data->sem, K_FOREVER);
		ina230_handle_interrupts(data->dev);
	}
}
#endif

static int get_enable_flag_from_trigger(const struct sensor_trigger *trig, uint16_t *bit)
{
	switch (trig->chan) {
	case SENSOR_CHAN_VSHUNT:
		*bit = INA230_SHUNT_VOLTAGE_OVER;
		break;

	case SENSOR_CHAN_VOLTAGE:
		*bit = INA230_BUS_VOLTAGE_OVER;
		break;

	case SENSOR_CHAN_POWER:
		*bit = INA230_OVER_LIMIT_POWER;
		if ((enum sensor_trigger_type_ina230)trig->type == SENSOR_TRIG_INA230_UNDER) {
			return -ENOTSUP;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
};

int ina230_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	enum sensor_trigger_type_ina230 type = (enum sensor_trigger_type_ina230)trig->type;
	const struct ina230_config *config = dev->config;
	struct ina230_data *ina230 = dev->data;
	uint16_t new_val, mask, flag;
	int ret = 0;

	if (trig->type != SENSOR_TRIG_DATA_READY && type != SENSOR_TRIG_INA230_OVER &&
	    type != SENSOR_TRIG_INA230_UNDER) {
		return -ENOTSUP;
	}

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		ina230->handler_cnvr = handler;
		ina230->trig_cnvr = trig;
		mask = INA230_CONVERSION_READY;
		flag = INA230_CONVERSION_READY;
	} else {
		ret = get_enable_flag_from_trigger(trig, &flag);
		if (ret) {
			goto exit;
		}

		ina230->handler_alert = handler;
		ina230->trig_alert = trig;
		mask = INA230_SHUNT_VOLTAGE_OVER | INA230_SHUNT_VOLTAGE_UNDER |
		       INA230_BUS_VOLTAGE_OVER | INA230_BUS_VOLTAGE_UNDER | INA230_OVER_LIMIT_POWER;

		if (type == SENSOR_TRIG_INA230_UNDER) {
			flag >>= 1;
		}
	}

	new_val = (ina230->mask & ~mask) | (!!handler * flag);

	if (new_val == ina230->mask) {
		goto exit;
	}

	ret = ina2xx_reg_write(&config->common.bus, INA230_REG_MASK, new_val);
	if (ret) {
		LOG_ERR("Failed to configure alert");
		goto exit;
	}

	ina230->mask = new_val;

exit:
	return ret;
}

int ina230_trigger_mode_init(const struct device *dev)
{
	const struct ina230_config *config = dev->config;
	struct ina230_data *ina230 = dev->data;
	int ret;

	/* setup alert gpio interrupt */
	if (!gpio_is_ready_dt(&config->alert_gpio)) {
		LOG_ERR("Alert GPIO device not ready");
		return -ENODEV;
	}

	ina230->dev = dev;

	ret = gpio_pin_configure_dt(&config->alert_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gpio");
		return ret;
	}

#if defined(CONFIG_INA230_TRIGGER_OWN_THREAD)
	k_sem_init(&ina230->sem, 0, K_SEM_MAX_LIMIT);

	k_tid_t tid =
		k_thread_create(&ina230->thread, ina230->thread_stack,
				CONFIG_INA230_THREAD_STACK_SIZE, ina230_thread_main, ina230, NULL,
				NULL, K_PRIO_COOP(CONFIG_INA230_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(tid, "ina230-trigger");
#elif defined(CONFIG_INA230_TRIGGER_GLOBAL_THREAD)
	ina230->work.handler = ina230_work_handler;
#endif

	gpio_init_callback(&ina230->gpio_cb, ina230_gpio_callback, BIT(config->alert_gpio.pin));

	ret = gpio_add_callback(config->alert_gpio.port, &ina230->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback");
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(&config->alert_gpio, GPIO_INT_EDGE_TO_ACTIVE);
}
