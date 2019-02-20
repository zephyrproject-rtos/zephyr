/* ST Microelectronics LIS2DW12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2dw12.pdf
 */

#include <kernel.h>
#include <sensor.h>
#include <gpio.h>
#include <logging/log.h>

#include "lis2dw12.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_DECLARE(LIS2DW12);

/**
 * lis2dw12_enable_drdy - enable selected int pin to generate DRDY interrupt
 */
static int lis2dw12_enable_drdy(struct device *dev, int enable)
{
	const struct lis2dw12_device_config *cfg = dev->config->config_info;
	struct lis2dw12_data *lis2dw12 = dev->driver_data;

	/* set interrupt */
	if (cfg->int_pin == 1)
		return lis2dw12->hw_tf->update_reg(lis2dw12,
			 LIS2DW12_CTRL4_ADDR,
			 LIS2DW12_INT1_DRDY,
			 enable);

	return lis2dw12->hw_tf->update_reg(lis2dw12,
		 LIS2DW12_CTRL5_ADDR,
		 LIS2DW12_INT2_DRDY,
		 enable);
}

#ifdef CONFIG_LIS2DW12_TAP
/*
 * lis2dw12_enable_tap - enable single or double tap event
 * (odr >= 400Hz)
 */
static int lis2dw12_enable_tap(struct device *dev, u8_t dtap, u8_t enable)
{
	u8_t tmp;
	u8_t int_dur, tap_int;
	struct lis2dw12_data *lis2dw12 = dev->driver_data;

	if (dtap) {
		int_dur = 0x7f; /* latency = 7, shock = 3, quiet = 3 */
		tap_int = LIS2DW12_INT1_DTAP;
	} else {
		int_dur = 0x06; /* latency = 0, shock = 2, quiet = 1 */
		tap_int = LIS2DW12_INT1_STAP;
	}

	/* enable LOW noise */
	if (lis2dw12->hw_tf->update_reg(lis2dw12, LIS2DW12_CTRL6_ADDR,
				    LIS2DW12_LOW_NOISE_MASK, 1)) {
		LOG_ERR("enable low noise error");
		return -EIO;
	}

	/* Set tap threshold for X axis. */
	if (lis2dw12->hw_tf->update_reg(lis2dw12, LIS2DW12_TAP_THS_X,
					LIS2DW12_TAP_THS_MASK,
					lis2dw12->tap_ths)) {
		LOG_ERR("set x-threshold error");
		return -EIO;
	}

	/* Set tap threshold for Y axis and TAP priority Z-Y-X. */
	tmp = (0xE0 | ((lis2dw12->tap_ths) & (LIS2DW12_TAP_THS_MASK)));
	if (lis2dw12->hw_tf->write_reg(lis2dw12, LIS2DW12_TAP_THS_Y, tmp)) {
		LOG_ERR("set y-threshold error");
		return -EIO;
	}

	/* Set tap threshold for Z axis and tap detection on X, Y, Z-axis */
	tmp = (0xE0 | ((lis2dw12->tap_ths) & (LIS2DW12_TAP_THS_MASK)));
	if (lis2dw12->hw_tf->write_reg(lis2dw12, LIS2DW12_TAP_THS_Z, tmp)) {
		LOG_ERR("set z-threshold error");
		return -EIO;
	}

	/* Set interrupt duration params (latency/quiet/shock time windows */
	if (lis2dw12->hw_tf->write_reg(lis2dw12, LIS2DW12_INT_DUR, int_dur)) {
		LOG_ERR("set int duration error");
		return -EIO;
	}

	/* Enable single-tap or double-tap event */
	if (lis2dw12->hw_tf->update_reg(lis2dw12, LIS2DW12_WAKE_UP_THS,
					LIS2DW12_DTAP_MASK, dtap)) {
		LOG_ERR("enable single/double tap event error");
		return -EIO;
	}

	/* Enable single-tap or double-tap interrupt */
	if (lis2dw12->hw_tf->update_reg(lis2dw12, LIS2DW12_CTRL4_ADDR,
					tap_int, enable)) {
		LOG_ERR("enable single/double tap int error");
		return -EIO;
	}

	/* Enable algo interrupts */
	if (lis2dw12->hw_tf->update_reg(lis2dw12, LIS2DW12_CTRL7_ADDR,
					LIS2DW12_INTS_ENABLED, enable)) {
		LOG_ERR("enable algo int error");
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_LIS2DW12_TAP */

/**
 * lis2dw12_trigger_set - link external trigger to event data ready
 */
int lis2dw12_trigger_set(struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct lis2dw12_data *lis2dw12 = dev->driver_data;
	u8_t raw[6];

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		lis2dw12->handler_drdy = handler;
		if (handler) {
			/* dummy read: re-trigger interrupt */
			lis2dw12->hw_tf->read_data(lis2dw12,
					    LIS2DW12_OUT_X_L_ADDR, raw,
					    sizeof(raw));
			return lis2dw12_enable_drdy(dev, LIS2DW12_EN_BIT);
		} else {
			return lis2dw12_enable_drdy(dev, LIS2DW12_DIS_BIT);
		}
	}
#ifdef CONFIG_LIS2DW12_TAP
	else if (trig->type == SENSOR_TRIG_TAP) {
		lis2dw12->handler_tap = handler;

		return lis2dw12_enable_tap(dev, 0, (handler) ? 1 : 0);
	} else if (trig->type == SENSOR_TRIG_DOUBLE_TAP) {
		lis2dw12->handler_tap = handler;

		return lis2dw12_enable_tap(dev, 1, (handler) ? 1 : 0);
	}
#endif /* CONFIG_LIS2DW12_TAP */

	return -ENOTSUP;
}

/**
 * lis2dw12_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lis2dw12_handle_interrupt(void *arg)
{
	u8_t status;
	struct device *dev = arg;
	struct lis2dw12_data *lis2dw12 = dev->driver_data;
	const struct lis2dw12_device_config *cfg = dev->config->config_info;
	struct sensor_trigger trigger;

	if (lis2dw12->hw_tf->read_reg(lis2dw12,
				      LIS2DW12_STATUS_REG,
				      &status) < 0) {
		LOG_ERR("status reading error");
	}

	if ((status & LIS2DW12_STS_DRDY) &&
	    (lis2dw12->handler_drdy != NULL)) {
		trigger.type = SENSOR_TRIG_DATA_READY;
		lis2dw12->handler_drdy(dev, &trigger);
	}

	if ((status & LIS2DW12_STS_STAP) &&
	    (lis2dw12->handler_tap != NULL)) {
		trigger.type = SENSOR_TRIG_TAP;
		lis2dw12->hw_tf->read_reg(lis2dw12, LIS2DW12_TAP_SRC, &status);
		lis2dw12->handler_tap(dev, &trigger);
	}

	if ((status & LIS2DW12_STS_DTAP) &&
	    (lis2dw12->handler_tap != NULL)) {
		trigger.type = SENSOR_TRIG_DOUBLE_TAP;
		lis2dw12->hw_tf->read_reg(lis2dw12, LIS2DW12_TAP_SRC, &status);
		lis2dw12->handler_tap(dev, &trigger);
	}

	gpio_pin_enable_callback(lis2dw12->gpio, cfg->int_gpio_pin);
}

static void lis2dw12_gpio_callback(struct device *dev,
				    struct gpio_callback *cb, u32_t pins)
{
	struct lis2dw12_data *lis2dw12 =
		CONTAINER_OF(cb, struct lis2dw12_data, gpio_cb);
	const struct lis2dw12_device_config *cfg = dev->config->config_info;

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, cfg->int_gpio_pin);

#if defined(CONFIG_LIS2DW12_TRIGGER_OWN_THREAD)
	k_sem_give(&lis2dw12->gpio_sem);
#elif defined(CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lis2dw12->work);
#endif /* CONFIG_LIS2DW12_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_LIS2DW12_TRIGGER_OWN_THREAD
static void lis2dw12_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct lis2dw12_data *lis2dw12 = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&lis2dw12->gpio_sem, K_FOREVER);
		lis2dw12_handle_interrupt(dev);
	}
}
#endif /* CONFIG_LIS2DW12_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD
static void lis2dw12_work_cb(struct k_work *work)
{
	struct lis2dw12_data *lis2dw12 =
		CONTAINER_OF(work, struct lis2dw12_data, work);

	lis2dw12_handle_interrupt(lis2dw12->dev);
}
#endif /* CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD */

int lis2dw12_init_interrupt(struct device *dev)
{
	struct lis2dw12_data *lis2dw12 = dev->driver_data;
	const struct lis2dw12_device_config *cfg = dev->config->config_info;
	int ret;

	/* setup data ready gpio interrupt (INT1 or INT2) */
	lis2dw12->gpio = device_get_binding(cfg->int_gpio_port);
	if (lis2dw12->gpio == NULL) {
		LOG_DBG("Cannot get pointer to %s device",
			    cfg->int_gpio_port);
		return -EINVAL;
	}

#if defined(CONFIG_LIS2DW12_TRIGGER_OWN_THREAD)
	k_sem_init(&lis2dw12->gpio_sem, 0, UINT_MAX);

	k_thread_create(&lis2dw12->thread, lis2dw12->thread_stack,
		       CONFIG_LIS2DW12_THREAD_STACK_SIZE,
		       (k_thread_entry_t)lis2dw12_thread, dev,
		       0, NULL, K_PRIO_COOP(CONFIG_LIS2DW12_THREAD_PRIORITY),
		       0, 0);
#elif defined(CONFIG_LIS2DW12_TRIGGER_GLOBAL_THREAD)
	lis2dw12->work.handler = lis2dw12_work_cb;
	lis2dw12->dev = dev;
#endif /* CONFIG_LIS2DW12_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure(lis2dw12->gpio, cfg->int_gpio_pin,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&lis2dw12->gpio_cb,
			   lis2dw12_gpio_callback,
			   BIT(cfg->int_gpio_pin));

	if (gpio_add_callback(lis2dw12->gpio, &lis2dw12->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* enable interrupt on int1/int2 in lir mode */
	if (lis2dw12->hw_tf->update_reg(lis2dw12, LIS2DW12_CTRL3_ADDR,
					LIS2DW12_LIR_MASK, 1)) {
		return -EIO;
	}

	return gpio_pin_enable_callback(lis2dw12->gpio, cfg->int_gpio_pin);
}
