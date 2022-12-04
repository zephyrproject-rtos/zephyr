/*
 * Copyright 2023 Daniel DeGrasse <daniel@degrasse.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(tcn75a, CONFIG_SENSOR_LOG_LEVEL);

#include "tcn75a.h"

int tcn75a_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	const struct tcn75a_config *config = dev->config;
	struct tcn75a_data *data = dev->data;
	int ret;

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		return -ENOTSUP;
	}

	if ((trig->chan != SENSOR_CHAN_ALL) && (trig->chan != SENSOR_CHAN_AMBIENT_TEMP)) {
		return -ENOTSUP;
	}

	data->sensor_cb = handler;
	data->sensor_trig = trig;

	/* TCN75A starts in comparator mode by default, switch it to
	 * use interrupt mode.
	 */
	ret = i2c_reg_update_byte_dt(&config->i2c_spec, TCN75A_CONFIG_REG, TCN75A_CONFIG_INT_EN,
				     TCN75A_CONFIG_INT_EN);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->alert_gpios, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

int tcn75a_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	const struct tcn75a_config *config = dev->config;
	uint8_t tx_buf[3];

	if ((chan != SENSOR_CHAN_AMBIENT_TEMP) && (chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		tx_buf[0] = TCN75A_THYST_REG;
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		tx_buf[0] = TCN75A_TSET_REG;
		break;
	default:
		return -ENOTSUP;
	}

	/* Convert sensor val to fixed point */
	tx_buf[1] = (uint8_t)val->val1;
	tx_buf[2] = TCN75A_SENSOR_TO_FIXED_PT(val->val2);

	LOG_DBG("Writing 0x%02X to limit reg %s", *(uint16_t *)(tx_buf + 1),
		tx_buf[0] == TCN75A_THYST_REG ? "THYST" : "TSET");

	return i2c_write_dt(&config->i2c_spec, tx_buf, 3);
}

int tcn75a_attr_get(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		    struct sensor_value *val)
{
	const struct tcn75a_config *config = dev->config;
	uint8_t config_reg;
	uint8_t rx_buf[2];
	uint16_t limit, temp_lsb;
	int ret;

	if ((chan != SENSOR_CHAN_AMBIENT_TEMP) && (chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		config_reg = TCN75A_THYST_REG;
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		config_reg = TCN75A_TSET_REG;
		break;
	default:
		return -ENOTSUP;
	}

	ret = i2c_write_read_dt(&config->i2c_spec, &config_reg, 1, rx_buf, 2);
	if (ret < 0) {
		return ret;
	}

	limit = sys_get_be16(rx_buf);

	LOG_DBG("Read 0x%02X from %s", limit, config_reg == TCN75A_THYST_REG ? "THYST" : "TSET");

	/* Convert fixed point to sensor value  */
	val->val1 = limit >> TCN75A_TEMP_MSB_POS;
	temp_lsb = (limit & TCN75A_TEMP_LSB_MASK);
	val->val2 = TCN75A_FIXED_PT_TO_SENSOR(temp_lsb);

	return ret;
}

static void tcn75a_handle_int(const struct device *dev)
{
	struct tcn75a_data *data = dev->data;
	/* Note that once the temperature rises
	 * above T_SET, the sensor will not trigger another interrupt until
	 * it falls below T_HYST (or vice versa for falling below T_HYST).
	 *
	 * Reading from any register will de-assert the interrupt.
	 */

	if (data->sensor_cb) {
		data->sensor_cb(dev, data->sensor_trig);
	}
}

static void tcn75a_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				 uint32_t pin_mask)
{
	struct tcn75a_data *data = CONTAINER_OF(cb, struct tcn75a_data, gpio_cb);
	const struct tcn75a_config *config = data->dev->config;

	if ((pin_mask & BIT(config->alert_gpios.pin)) == 0U) {
		return;
	}

#if defined(CONFIG_TCN75A_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_TCN75A_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#ifdef CONFIG_TCN75A_TRIGGER_OWN_THREAD
static void tcn75a_thread_main(struct tcn75a_data *data)
{
	while (true) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		tcn75a_handle_int(data->dev);
	}
}
#endif

#ifdef CONFIG_TCN75A_TRIGGER_GLOBAL_THREAD
static void tcn75a_work_handler(struct k_work *work)
{
	struct tcn75a_data *data = CONTAINER_OF(work, struct tcn75a_data, work);

	tcn75a_handle_int(data->dev);
}
#endif

int tcn75a_trigger_init(const struct device *dev)
{
	const struct tcn75a_config *config = dev->config;
	struct tcn75a_data *data = dev->data;
	int ret;

	/* Save config pointer */
	data->dev = dev;

	if (!gpio_is_ready_dt(&config->alert_gpios)) {
		LOG_ERR("alert GPIO device is not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->alert_gpios, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, tcn75a_gpio_callback, BIT(config->alert_gpios.pin));

	ret = gpio_add_callback(config->alert_gpios.port, &data->gpio_cb);

#if defined(CONFIG_TCN75A_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack, CONFIG_TCN75A_THREAD_STACK_SIZE,
			(k_thread_entry_t)tcn75a_thread_main, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_TCN75A_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_TCN75A_TRIGGER_GLOBAL_THREAD)
	data->work.handler = tcn75a_work_handler;
#endif

	return ret;
}
