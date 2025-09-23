/*
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl345

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include "adxl345.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ADXL345, CONFIG_SENSOR_LOG_LEVEL);

static int adxl345_set_int_pad_state(const struct device *dev, uint8_t pad,
				     bool en)
{
	const struct adxl345_dev_config *cfg = dev->config;
	int state = en ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	/* in case of neither INT_1 nor INT_2 being defined */
	if (!cfg->gpio_int1.port && !cfg->gpio_int2.port) {
		return -ENOTSUP;
	}

	if (pad == 1) {
		return gpio_pin_interrupt_configure_dt(&cfg->gpio_int1, state);
	} else if (pad == 2) {
		return gpio_pin_interrupt_configure_dt(&cfg->gpio_int2, state);
	}

	/* pad may be -1, e.g. if no INT line defined in DT */
	return -EINVAL;
}

int adxl345_set_gpios_en(const struct device *dev, bool en)
{
	const struct adxl345_dev_config *cfg = dev->config;

	return adxl345_set_int_pad_state(dev, cfg->drdy_pad, en);
}

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD) || defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
static void adxl345_thread_cb(const struct device *dev)
{
	struct adxl345_dev_data *drv_data = dev->data;
	uint8_t status;
	int ret;

	ret = adxl345_get_status(dev, &status);
	__ASSERT(ret == 0, "Interrupt configuration failed");

	if (FIELD_GET(ADXL345_INT_DATA_RDY, status)) {
		if (drv_data->drdy_handler) {
			/*
			 * A handler needs to flush FIFO, i.e. fetch and get
			 * samples to get new events
			 */
			drv_data->drdy_handler(dev, drv_data->drdy_trigger);
		}
	}

	if (FIELD_GET(ADXL345_INT_WATERMARK, status)) {
		if (drv_data->wm_handler) {
			/*
			 * A handler needs to implement fetch, then get FIFO
			 * entries according to configured watermark in order
			 * to obtain new sensor events
			 */
			drv_data->wm_handler(dev, drv_data->wm_trigger);
		}
	}

	/* handle FIFO: overrun */
	if (FIELD_GET(ADXL345_INT_OVERRUN, status)) {
		if (drv_data->overrun_handler) {
			/*
			 * A handler may handle read outs, the fallback flushes
			 * the fifo and interrupt status register
			 */
			drv_data->overrun_handler(dev, drv_data->overrun_trigger);
		}

		/*
		 * If overrun handling is enabled, reset status register and
		 * fifo here, if not handled before in any way
		 */
		adxl345_raw_flush_fifo(dev);
	}

	if (drv_data->act_trigger && FIELD_GET(ADXL345_INT_ACT, status)) {
		if (drv_data->act_handler) {
			drv_data->act_handler(dev, drv_data->act_trigger);
		}
	}

	ret = adxl345_set_gpios_en(dev, true);
	__ASSERT(ret == 0, "Interrupt configuration failed");
}
#endif

static void adxl345_int1_gpio_callback(const struct device *dev,
				       struct gpio_callback *cb,
				       uint32_t pins)
{
	struct adxl345_dev_data *drv_data =
		CONTAINER_OF(cb, struct adxl345_dev_data, int1_cb);

	ARG_UNUSED(pins);

	adxl345_set_int_pad_state(dev, 1, false);

#if defined(CONFIG_ADXL345_STREAM)
	adxl345_stream_irq_handler(drv_data->dev);
#endif

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

static void adxl345_int2_gpio_callback(const struct device *dev,
				       struct gpio_callback *cb,
				       uint32_t pins)
{
	struct adxl345_dev_data *drv_data =
		CONTAINER_OF(cb, struct adxl345_dev_data, int2_cb);

	ARG_UNUSED(pins);

	adxl345_set_int_pad_state(dev, 2, false);

#if defined(CONFIG_ADXL345_STREAM)
	adxl345_stream_irq_handler(drv_data->dev);
#endif

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
	k_sem_give(&drv_data->gpio_sem);
#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&drv_data->work);
#endif
}

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
static void adxl345_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct adxl345_dev_data *drv_data = p1;

	while (true) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		adxl345_thread_cb(drv_data->dev);
	}
}

#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
static void adxl345_work_cb(struct k_work *work)
{
	struct adxl345_dev_data *drv_data =
		CONTAINER_OF(work, struct adxl345_dev_data, work);

	adxl345_thread_cb(drv_data->dev);
}
#endif

int adxl345_trigger_set(const struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct adxl345_dev_config *cfg = dev->config;
	struct adxl345_dev_data *drv_data = dev->data;
	int ret;

	if (!cfg->gpio_int1.port && !cfg->gpio_int2.port) {
		/* might be in FIFO BYPASS mode */
		goto done;
	}

	/* generally turn off interrupts */
	ret = adxl345_set_gpios_en(dev, false);
	if (ret) {
		goto done;
	}

	if (!handler) {
		goto done;
	}

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		drv_data->drdy_handler = handler;
		drv_data->drdy_trigger = trig;

		ret = adxl345_reg_assign_bits(dev, ADXL345_INT_ENABLE_REG,
					      ADXL345_INT_DATA_RDY,
					      true);
		if (ret) {
			return ret;
		}
		break;
	case SENSOR_TRIG_FIFO_WATERMARK:
		drv_data->wm_handler = handler;
		drv_data->wm_trigger = trig;
		ret = adxl345_reg_assign_bits(dev, ADXL345_INT_ENABLE_REG,
					      ADXL345_INT_WATERMARK,
					      true);
		if (ret) {
			return ret;
		}
		break;
	case SENSOR_TRIG_FIFO_FULL:
		drv_data->overrun_handler = handler;
		drv_data->overrun_trigger = trig;
		ret = adxl345_reg_assign_bits(dev, ADXL345_INT_ENABLE_REG,
					      ADXL345_INT_OVERRUN,
					      true);
		if (ret) {
			return ret;
		}
		break;
	case SENSOR_TRIG_MOTION:
		drv_data->act_handler = handler;
		drv_data->act_trigger = trig;
		ret = adxl345_reg_write_byte(dev, ADXL345_ACT_INACT_CTL_REG,
					ADXL345_ACT_AC_DC | ADXL345_ACT_X_EN |
					ADXL345_ACT_Y_EN | ADXL345_ACT_Z_EN);
		if (ret) {
			return ret;
		}

		ret = adxl345_reg_assign_bits(dev, ADXL345_INT_ENABLE_REG,
					      ADXL345_INT_ACT,
					      true);
		if (ret) {
			return ret;
		}
		break;
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

done:
	ret = adxl345_set_gpios_en(dev, true);
	if (ret) {
		return ret;
	}

	return adxl345_raw_flush_fifo(dev);
}

int adxl345_init_interrupt(const struct device *dev)
{
	const struct adxl345_dev_config *cfg = dev->config;
	struct adxl345_dev_data *drv_data = dev->data;
	int ret;

	/* TRIGGER is set, but no INT line was defined in DT */
	if (!cfg->gpio_int1.port && !cfg->gpio_int2.port) {
		return -ENOTSUP;
	}

	if (cfg->gpio_int1.port) {
		if (!gpio_is_ready_dt(&cfg->gpio_int1)) {
			LOG_ERR("INT_1 line defined, but not ready");
			return -ENODEV;
		}
	}

	if (cfg->gpio_int2.port) {
		if (!gpio_is_ready_dt(&cfg->gpio_int2)) {
			LOG_ERR("INT_2 line defined, but not ready");
			return -ENODEV;
		}
	}

	drv_data->dev = dev;

#if defined(CONFIG_ADXL345_TRIGGER_OWN_THREAD)
	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack,
			CONFIG_ADXL345_THREAD_STACK_SIZE,
			adxl345_thread, drv_data,
			NULL, NULL, K_PRIO_COOP(CONFIG_ADXL345_THREAD_PRIORITY),
			0, K_NO_WAIT);

	k_thread_name_set(&drv_data->thread, dev->name);
#elif defined(CONFIG_ADXL345_TRIGGER_GLOBAL_THREAD)
	drv_data->work.handler = adxl345_work_cb;
#endif

	if (cfg->gpio_int1.port) {
		ret = gpio_pin_configure_dt(&cfg->gpio_int1, GPIO_INPUT);
		if (ret) {
			return ret;
		}

		gpio_init_callback(&drv_data->int1_cb, adxl345_int1_gpio_callback,
				   BIT(cfg->gpio_int1.pin));

		ret = gpio_add_callback(cfg->gpio_int1.port, &drv_data->int1_cb);
		if (ret) {
			LOG_ERR("Failed to set INT_1 gpio callback!");
			return -EIO;
		}
	}

	if (cfg->gpio_int2.port) {
		ret = gpio_pin_configure_dt(&cfg->gpio_int2, GPIO_INPUT);
		if (ret) {
			return ret;
		}

		gpio_init_callback(&drv_data->int2_cb, adxl345_int2_gpio_callback,
				   BIT(cfg->gpio_int2.pin));

		ret = gpio_add_callback(cfg->gpio_int2.port, &drv_data->int2_cb);
		if (ret) {
			LOG_ERR("Failed to set INT_2 gpio callback!");
			return -EIO;
		}
	}

	return 0;
}
