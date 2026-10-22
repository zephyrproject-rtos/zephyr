/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "max30009.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(MAX30009);

#if defined(CONFIG_MAX30009_TRIGGER_OWN_THREAD) || defined(CONFIG_MAX30009_TRIGGER_GLOBAL_THREAD)
static void max30009_thread_cb(const struct device *dev)
{
	const struct max30009_dev_config *cfg = dev->config;
	struct max30009_data *data = dev->data;
	struct max30009_trigger_data *trig = &data->trig;
	uint8_t status1, status2;
	int ret;

	/* Clear the status */
	ret = max30009_reg_read(dev, MAX30009_STATUS1, &status1);
	if (ret < 0) {
		LOG_ERR("Failed to read status register 1: %d", ret);
		return;
	}

	ret = max30009_reg_read(dev, MAX30009_STATUS2, &status2);
	if (ret < 0) {
		LOG_ERR("Failed to read status register 2: %d", ret);
		return;
	}

	if ((trig->drdy_handler != NULL) && (status1 & MAX30009_FIFO_DATA_RDY_MSK)) {
		trig->drdy_handler(dev, trig->drdy_trigger);
	}

	if ((trig->dc_loff_nl_handler != NULL) && (status2 & MAX30009_STATUS2_DC_LOFF_NL_MSK)) {
		trig->dc_loff_nl_handler(dev, trig->dc_loff_nl_trigger);
	}

	if ((trig->dc_loff_nh_handler != NULL) && (status2 & MAX30009_STATUS2_DC_LOFF_NH_MSK)) {
		trig->dc_loff_nh_handler(dev, trig->dc_loff_nh_trigger);
	}

	if ((trig->dc_loff_pl_handler != NULL) && (status2 & MAX30009_STATUS2_DC_LOFF_PL_MSK)) {
		trig->dc_loff_pl_handler(dev, trig->dc_loff_pl_trigger);
	}

	if ((trig->dc_loff_ph_handler != NULL) && (status2 & MAX30009_STATUS2_DC_LOFF_PH_MSK)) {
		trig->dc_loff_ph_handler(dev, trig->dc_loff_ph_trigger);
	}

	if ((trig->drv_oor_handler != NULL) && (status2 & MAX30009_STATUS2_DRV_OOR_MSK)) {
		trig->drv_oor_handler(dev, trig->drv_oor_trigger);
	}

	if ((trig->bioz_undr_handler != NULL) && (status2 & MAX30009_STATUS2_BIOZ_UNDR_MSK)) {
		trig->bioz_undr_handler(dev, trig->bioz_undr_trigger);
	}

	if ((trig->bioz_over_handler != NULL) && (status2 & MAX30009_STATUS2_BIOZ_OVER_MSK)) {
		trig->bioz_over_handler(dev, trig->bioz_over_trigger);
	}

	if ((trig->lon_handler != NULL) && (status2 & MAX30009_STATUS2_LON_MSK)) {
		trig->lon_handler(dev, trig->lon_trigger);
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_ERR("Failed to enable interrupt: %d", ret);
		return;
	}
}
#endif /* CONFIG_MAX30009_TRIGGER_OWN_THREAD || CONFIG_MAX30009_TRIGGER_GLOBAL_THREAD */

static void max30009_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	int ret;
	struct max30009_data *data = CONTAINER_OF(cb, struct max30009_data, gpio_cb);
	const struct max30009_dev_config *cfg = data->dev->config;

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret) {
		LOG_ERR("Failed to disable interrupt: %d", ret);
		return;
	}

#ifdef CONFIG_MAX30009_STREAM
	max30009_stream_irq_handler(data->dev);
#endif /* CONFIG_MAX30009_STREAM */
#ifdef CONFIG_MAX30009_TRIGGER_OWN_THREAD
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_MAX30009_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#if defined(CONFIG_MAX30009_TRIGGER_OWN_THREAD)
static void max30009_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	struct max30009_data *data = p1;

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		max30009_thread_cb(data->dev);
	}
}
#elif defined(CONFIG_MAX30009_TRIGGER_GLOBAL_THREAD)
static void max30009_work_cb(struct k_work *work)
{
	struct max30009_data *data = CONTAINER_OF(work, struct max30009_data, work);

	max30009_thread_cb(data->dev);
}
#endif /* CONFIG_MAX30009_TRIGGER_OWN_THREAD || CONFIG_MAX30009_TRIGGER_GLOBAL_THREAD */

int max30009_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct max30009_dev_config *cfg = dev->config;
	struct max30009_data *data = dev->data;
	uint8_t int_mask;
	uint8_t reg_addr;
	uint8_t int_en;
	int ret;

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret) {
		LOG_ERR("Failed to disable interrupt: %d", ret);
		return ret;
	}

	switch ((int)trig->type) {
	case SENSOR_TRIG_DATA_READY:
		/** Enabling DRDY means not using FIFO interrupts as both
		 * are served by reading data-register: two clients can't be
		 * served simultaneously. The mask covers both A_FULL and
		 * FIFO_DATA_RDY: reg_update places int_en at the lowest masked
		 * bit (FIFO_DATA_RDY) and clears the rest, so enabling DRDY also
		 * disables A_FULL.
		 */
		int_mask = MAX30009_INTERRUPT_ENABLE1_A_FULL_EN_MSK |
			   MAX30009_INTERRUPT_ENABLE1_FIFO_DATA_RDY_EN_MSK;
		data->trig.drdy_handler = handler;
		data->trig.drdy_trigger = trig;
		reg_addr = MAX30009_INTERRUPT_ENABLE1;
		break;
	case SENSOR_TRIG_MAX30009_DC_LOFF_NL:
		int_mask = MAX30009_INTERRUPT_ENABLE2_DC_LOFF_NL_EN_MSK;
		data->trig.dc_loff_nl_handler = handler;
		data->trig.dc_loff_nl_trigger = trig;
		reg_addr = MAX30009_INTERRUPT_ENABLE2;
		break;
	case SENSOR_TRIG_MAX30009_DC_LOFF_NH:
		int_mask = MAX30009_INTERRUPT_ENABLE2_DC_LOFF_NH_EN_MSK;
		data->trig.dc_loff_nh_handler = handler;
		data->trig.dc_loff_nh_trigger = trig;
		reg_addr = MAX30009_INTERRUPT_ENABLE2;
		break;
	case SENSOR_TRIG_MAX30009_DC_LOFF_PL:
		int_mask = MAX30009_INTERRUPT_ENABLE2_DC_LOFF_PL_EN_MSK;
		data->trig.dc_loff_pl_handler = handler;
		data->trig.dc_loff_pl_trigger = trig;
		reg_addr = MAX30009_INTERRUPT_ENABLE2;
		break;
	case SENSOR_TRIG_MAX30009_DC_LOFF_PH:
		int_mask = MAX30009_INTERRUPT_ENABLE2_DC_LOFF_PH_EN_MSK;
		data->trig.dc_loff_ph_handler = handler;
		data->trig.dc_loff_ph_trigger = trig;
		reg_addr = MAX30009_INTERRUPT_ENABLE2;
		break;
	case SENSOR_TRIG_MAX30009_DRV_OOR:
		int_mask = MAX30009_INTERRUPT_ENABLE2_DRV_OOR_EN_MSK;
		data->trig.drv_oor_handler = handler;
		data->trig.drv_oor_trigger = trig;
		reg_addr = MAX30009_INTERRUPT_ENABLE2;
		break;
	case SENSOR_TRIG_MAX30009_BIOZ_UNDR:
		int_mask = MAX30009_INTERRUPT_ENABLE2_BIOZ_UNDR_EN_MSK;
		data->trig.bioz_undr_handler = handler;
		data->trig.bioz_undr_trigger = trig;
		reg_addr = MAX30009_INTERRUPT_ENABLE2;
		break;
	case SENSOR_TRIG_MAX30009_BIOZ_OVER:
		int_mask = MAX30009_INTERRUPT_ENABLE2_BIOZ_OVER_EN_MSK;
		data->trig.bioz_over_handler = handler;
		data->trig.bioz_over_trigger = trig;
		reg_addr = MAX30009_INTERRUPT_ENABLE2;
		break;
	case SENSOR_TRIG_MAX30009_LON:
		int_mask = MAX30009_INTERRUPT_ENABLE2_LON_EN_MSK;
		data->trig.lon_handler = handler;
		data->trig.lon_trigger = trig;
		reg_addr = MAX30009_INTERRUPT_ENABLE2;
		break;
	default:
		LOG_ERR("Unsupported trigger type: %d", trig->type);
		return -ENOTSUP;
	}

	if (handler == NULL) {
		int_en = 0U;
	} else {
		int_en = 1U;
	}

	ret = max30009_reg_update(dev, reg_addr, int_mask, int_en);
	if (ret) {
		LOG_ERR("Failed to update interrupt enable register: %d", ret);
		return ret;
	}
	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_ERR("Failed to enable interrupt: %d", ret);
		return ret;
	}
	return 0;
}

int max30009_init_interrupt(const struct device *dev)
{
	int ret;
	const struct max30009_dev_config *cfg = dev->config;
	struct max30009_data *data = dev->data;

	if (!gpio_is_ready_dt(&cfg->interrupt_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->interrupt_gpio.port);
		return -ENODEV;
	}

	/* Configure Interrupt Pin */
	ret = gpio_pin_configure_dt(&cfg->interrupt_gpio, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Failed to configure interrupt pin: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, max30009_gpio_callback, BIT(cfg->interrupt_gpio.pin));

	ret = gpio_add_callback(cfg->interrupt_gpio.port, &data->gpio_cb);
	if (ret != 0) {
		LOG_ERR("Failed to add GPIO callback: %d", ret);
		return ret;
	}

	data->dev = dev;

	ret = max30009_reg_update(dev, MAX30009_PIN_FUNC_CONFIG,
				  MAX30009_PIN_FUNC_CONFIG_INT_FCFG_MSK, 1);
	if (ret) {
		LOG_ERR("Failed to enable INT pin: %d", ret);
		return ret;
	}

	ret = max30009_reg_update(dev, MAX30009_OUTPUT_PIN_CONFIG,
				  MAX30009_OUTPUT_PIN_CONFIG_INT_OCFG_MSK, 2);
	if (ret) {
		LOG_ERR("Failed to configure INT output drive: %d", ret);
		return ret;
	}

#if defined(CONFIG_MAX30009_STREAM)
	ret = max30009_reg_update(dev, MAX30009_INTERRUPT_ENABLE1,
				  MAX30009_INTERRUPT_ENABLE1_A_FULL_EN_MSK, 1);
	if (ret) {
		LOG_ERR("Failed to enable FIFO Data Ready interrupt: %d", ret);
		return ret;
	}
#endif /* CONFIG_MAX30009_STREAM */

#if defined(CONFIG_MAX30009_TRIGGER_OWN_THREAD)
	k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack, CONFIG_MAX30009_THREAD_STACK_SIZE,
			max30009_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_MAX30009_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&data->thread, dev->name);
#elif defined(CONFIG_MAX30009_TRIGGER_GLOBAL_THREAD)
	data->work.handler = max30009_work_cb;
#endif
	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_ERR("Failed to enable interrupt: %d", ret);
		return ret;
	}
	LOG_DBG("MAX30009 interrupt initialized");
	return 0;
}
