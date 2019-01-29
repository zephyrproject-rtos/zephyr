/* ST Microelectronics LPS22HH pressure and temperature sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lps22hh.pdf
 */

#include <kernel.h>
#include <sensor.h>
#include <gpio.h>
#include <logging/log.h>

#include "lps22hh.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_DECLARE(LPS22HH);

/**
 * lps22hh_enable_int - enable selected int pin to generate interrupt
 */
static int lps22hh_enable_int(struct device *dev, int enable)
{
	struct lps22hh_data *lps22hh = dev->driver_data;

	/* set interrupt */
	return lps22hh->hw_tf->update_reg(lps22hh,
					  LPS22HH_REG_CTRL_REG3,
					  LPS22HH_INT1_DRDY,
					  enable);
}

/**
 * lps22hh_trigger_set - link external trigger to event data ready
 */
int lps22hh_trigger_set(struct device *dev,
			  const struct sensor_trigger *trig,
			  sensor_trigger_handler_t handler)
{
	struct lps22hh_data *lps22hh = dev->driver_data;
	u8_t raw[6];

	if (trig->chan == SENSOR_CHAN_ALL) {
		lps22hh->handler_drdy = handler;
		if (handler) {
			/* dummy read: re-trigger interrupt */
			lps22hh->hw_tf->read_data(lps22hh,
						  LPS22HH_REG_PRESS_OUT_XL,
						  raw, sizeof(raw));
			return lps22hh_enable_int(dev, LPS22HH_EN_BIT);
		} else {
			return lps22hh_enable_int(dev, LPS22HH_DIS_BIT);
		}
	}

	return -ENOTSUP;
}

/**
 * lps22hh_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void lps22hh_handle_interrupt(void *arg)
{
	struct device *dev = arg;
	struct lps22hh_data *lps22hh = dev->driver_data;
	struct sensor_trigger drdy_trigger = {
		.type = SENSOR_TRIG_DATA_READY,
	};

	if (lps22hh->handler_drdy != NULL) {
		lps22hh->handler_drdy(dev, &drdy_trigger);
	}

	gpio_pin_enable_callback(lps22hh->gpio, DT_ST_LPS22HH_0_DRDY_GPIOS_PIN);
}

static void lps22hh_gpio_callback(struct device *dev,
				  struct gpio_callback *cb, u32_t pins)
{
	struct lps22hh_data *lps22hh =
		CONTAINER_OF(cb, struct lps22hh_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, DT_ST_LPS22HH_0_DRDY_GPIOS_PIN);

#if defined(CONFIG_LPS22HH_TRIGGER_OWN_THREAD)
	k_sem_give(&lps22hh->gpio_sem);
#elif defined(CONFIG_LPS22HH_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&lps22hh->work);
#endif /* CONFIG_LPS22HH_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_LPS22HH_TRIGGER_OWN_THREAD
static void lps22hh_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct lps22hh_data *lps22hh = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&lps22hh->gpio_sem, K_FOREVER);
		lps22hh_handle_interrupt(dev);
	}
}
#endif /* CONFIG_LPS22HH_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LPS22HH_TRIGGER_GLOBAL_THREAD
static void lps22hh_work_cb(struct k_work *work)
{
	struct lps22hh_data *lps22hh =
		CONTAINER_OF(work, struct lps22hh_data, work);

	lps22hh_handle_interrupt(lps22hh->dev);
}
#endif /* CONFIG_LPS22HH_TRIGGER_GLOBAL_THREAD */

int lps22hh_init_interrupt(struct device *dev)
{
	struct lps22hh_data *lps22hh = dev->driver_data;
	int ret;

	/* setup data ready gpio interrupt */
	lps22hh->gpio = device_get_binding(DT_ST_LPS22HH_0_DRDY_GPIOS_CONTROLLER);
	if (lps22hh->gpio == NULL) {
		LOG_DBG("Cannot get pointer to %s device",
			    DT_ST_LPS22HH_0_DRDY_GPIOS_CONTROLLER);
		return -EINVAL;
	}

#if defined(CONFIG_LPS22HH_TRIGGER_OWN_THREAD)
	k_sem_init(&lps22hh->gpio_sem, 0, UINT_MAX);

	k_thread_create(&lps22hh->thread, lps22hh->thread_stack,
		       CONFIG_LPS22HH_THREAD_STACK_SIZE,
		       (k_thread_entry_t)lps22hh_thread, dev,
		       0, NULL, K_PRIO_COOP(CONFIG_LPS22HH_THREAD_PRIORITY),
		       0, 0);
#elif defined(CONFIG_LPS22HH_TRIGGER_GLOBAL_THREAD)
	lps22hh->work.handler = lps22hh_work_cb;
	lps22hh->dev = dev;
#endif /* CONFIG_LPS22HH_TRIGGER_OWN_THREAD */

	ret = gpio_pin_configure(lps22hh->gpio, DT_ST_LPS22HH_0_DRDY_GPIOS_PIN,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);
	if (ret < 0) {
		LOG_DBG("Could not configure gpio");
		return ret;
	}

	gpio_init_callback(&lps22hh->gpio_cb,
			   lps22hh_gpio_callback,
			   BIT(DT_ST_LPS22HH_0_DRDY_GPIOS_PIN));

	if (gpio_add_callback(lps22hh->gpio, &lps22hh->gpio_cb) < 0) {
		LOG_DBG("Could not set gpio callback");
		return -EIO;
	}

	/* configure interrupt active high */
	if (lps22hh->hw_tf->update_reg(lps22hh, LPS22HH_REG_CTRL_REG2,
					LPS22HH_INT_H_L, LPS22HH_DIS_BIT)) {
		return -EIO;
	}

	/* enable interrupt in pulse mode */
	if (lps22hh->hw_tf->update_reg(lps22hh, LPS22HH_REG_INTERRUPT_CFG,
					LPS22HH_LIR_MASK, LPS22HH_DIS_BIT)) {
		return -EIO;
	}

	return gpio_pin_enable_callback(lps22hh->gpio,
					DT_ST_LPS22HH_0_DRDY_GPIOS_PIN);
}
