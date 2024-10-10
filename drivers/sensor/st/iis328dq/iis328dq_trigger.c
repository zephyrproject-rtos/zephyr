/* ST Microelectronics IIS328DQ 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis328dq.pdf
 */

#define DT_DRV_COMPAT st_iis328dq

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "iis328dq.h"

LOG_MODULE_DECLARE(IIS328DQ, CONFIG_SENSOR_LOG_LEVEL);

static int iis328dq_set_int_pad_state(const struct device *dev, uint8_t pad, bool enable)
{
	const struct iis328dq_config *cfg = dev->config;

	int state = enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

	if (pad == 1) {
		return gpio_pin_interrupt_configure_dt(&cfg->gpio_int1, state);
	} else if (pad == 2) {
		return gpio_pin_interrupt_configure_dt(&cfg->gpio_int2, state);
	} else {
		return -EINVAL;
	}
}

/**
 * iis328dq_enable_int - enable selected int pin to generate interrupt
 */
static int iis328dq_enable_int(const struct device *dev, const struct sensor_trigger *trig,
			       int enable)
{
	const struct iis328dq_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		if (cfg->drdy_pad == 1) {
			/* route DRDY to PAD1 */
			if (iis328dq_pin_int1_route_set(ctx, IIS328DQ_PAD1_DRDY) != 0) {
				return -EIO;
			}
		} else if (cfg->drdy_pad == 2) {
			/* route DRDY to PAD2 */
			if (iis328dq_pin_int2_route_set(ctx, IIS328DQ_PAD2_DRDY) != 0) {
				return -EIO;
			}
		} else {
			LOG_ERR("No interrupt pin configured for DRDY in devicetree");
			return -ENOTSUP;
		}
		return iis328dq_set_int_pad_state(dev, cfg->drdy_pad, enable);
#ifdef CONFIG_IIS328DQ_THRESHOLD
	case SENSOR_TRIG_THRESHOLD: {
		/* set up internal INT1 for lower thresholds */
		int1_on_th_conf_t int1_conf = {0};

		switch (trig->chan) {
		case SENSOR_CHAN_ACCEL_X:
			int1_conf.int1_xlie = 1;
			break;
		case SENSOR_CHAN_ACCEL_Y:
			int1_conf.int1_ylie = 1;
			break;
		case SENSOR_CHAN_ACCEL_Z:
			int1_conf.int1_zlie = 1;
			break;
		case SENSOR_CHAN_ACCEL_XYZ:
			int1_conf.int1_xlie = 1;
			int1_conf.int1_ylie = 1;
			int1_conf.int1_zlie = 1;
			break;
		default:
			LOG_ERR("Invalid sensor channel %d", trig->chan);
			return -EINVAL;
		}

		if (iis328dq_int1_on_threshold_conf_set(ctx, int1_conf) != 0) {
			return -EIO;
		}

		/* set up internal INT2 for uppper thresholds */
		int2_on_th_conf_t int2_conf = {0};

		int2_conf.int2_xhie = int1_conf.int1_xlie;
		int2_conf.int2_yhie = int1_conf.int1_ylie;
		int2_conf.int2_zhie = int1_conf.int1_zlie;
		if (iis328dq_int2_on_threshold_conf_set(ctx, int2_conf) != 0) {
			return -EIO;
		}

		if (cfg->threshold_pad == 1) {
			/* route both internal interrupts to PAD1 */
			if (iis328dq_pin_int1_route_set(ctx, IIS328DQ_PAD1_INT1_OR_INT2_SRC) != 0) {
				return -EIO;
			}
		} else if (cfg->threshold_pad == 2) {
			/* route both internal interrupts to PAD2 */
			if (iis328dq_pin_int2_route_set(ctx, IIS328DQ_PAD2_INT1_OR_INT2_SRC) != 0) {
				return -EIO;
			}
		} else {
			LOG_ERR("No interrupt pin configured for DRDY in devicetree");
			return -ENOTSUP;
		}
		return iis328dq_set_int_pad_state(dev, cfg->threshold_pad, enable);
	}
#endif /* CONFIG_IIS328DQ_THRESHOLD */
	default:
		LOG_ERR("Unsupported trigger interrupt route %d", trig->type);
		return -ENOTSUP;
	}

	return 0;
}

/**
 * iis328dq_trigger_set - link external trigger to event data ready
 */
int iis328dq_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	const struct iis328dq_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct iis328dq_data *iis328dq = dev->data;
	int16_t raw[3];
	int state = (handler != NULL) ? PROPERTY_ENABLE : PROPERTY_DISABLE;

	if (!cfg->gpio_int1.port && !cfg->gpio_int2.port) {
		/* no interrupts configured */
		return -ENOTSUP;
	}

	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		iis328dq->drdy_handler = handler;
		iis328dq->drdy_trig = trig;
		if (state) {
			/* dummy read: re-trigger interrupt */
			iis328dq_acceleration_raw_get(ctx, raw);
		}
		break;
#ifdef CONFIG_IIS328DQ_THRESHOLD
	case SENSOR_TRIG_THRESHOLD:
		iis328dq->threshold_handler = handler;
		iis328dq->threshold_trig = trig;
		break;
#endif /* CONFIG_IIS328DQ_THRESHOLD */
	default:
		LOG_ERR("Unsupported sensor trigger");
		return -ENOTSUP;
	}

	return iis328dq_enable_int(dev, trig, state);
}

/**
 * iis328dq_handle_interrupt - handle the drdy event
 * read data and call handler if registered any
 */
static void iis328dq_handle_interrupt(const struct device *dev)
{
	const struct iis328dq_config *cfg = dev->config;
	struct iis328dq_data *data = dev->data;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	iis328dq_status_reg_t status;
#ifdef CONFIG_IIS328DQ_THRESHOLD
	iis328dq_int1_src_t sources1;
	iis328dq_int2_src_t sources2;
#endif

	iis328dq_status_reg_get(ctx, &status);

	if (status.zyxda) {
		if (data->drdy_handler) {
			data->drdy_handler(dev, data->drdy_trig);
		}

		iis328dq_set_int_pad_state(dev, cfg->drdy_pad, true);
	}
#ifdef CONFIG_IIS328DQ_THRESHOLD
	iis328dq_int1_src_get(ctx, &sources1);
	iis328dq_int2_src_get(ctx, &sources2);
	if (sources1.ia || sources2.ia) {
		if (data->threshold_handler) {
			data->threshold_handler(dev, data->threshold_trig);
		}

		iis328dq_set_int_pad_state(dev, cfg->threshold_pad, true);
	}
#endif /* CONFIG_IIS328DQ_THRESHOLD */
}

static void iis328dq_int1_gpio_callback(const struct device *dev, struct gpio_callback *cb,
					uint32_t pins)
{
	struct iis328dq_data *iis328dq = CONTAINER_OF(cb, struct iis328dq_data, int1_cb);

	ARG_UNUSED(pins);

	iis328dq_set_int_pad_state(iis328dq->dev, 1, true);

#if defined(CONFIG_IIS328DQ_TRIGGER_OWN_THREAD)
	k_sem_give(&iis328dq->gpio_sem);
#elif defined(CONFIG_IIS328DQ_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&iis328dq->work);
#endif /* CONFIG_IIS328DQ_TRIGGER_OWN_THREAD */
}

static void iis328dq_int2_gpio_callback(const struct device *dev, struct gpio_callback *cb,
					uint32_t pins)
{
	struct iis328dq_data *iis328dq = CONTAINER_OF(cb, struct iis328dq_data, int2_cb);

	ARG_UNUSED(pins);

	iis328dq_set_int_pad_state(iis328dq->dev, 2, true);

#if defined(CONFIG_IIS328DQ_TRIGGER_OWN_THREAD)
	k_sem_give(&iis328dq->gpio_sem);
#elif defined(CONFIG_IIS328DQ_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&iis328dq->work);
#endif /* CONFIG_IIS328DQ_TRIGGER_OWN_THREAD */
}

#ifdef CONFIG_IIS328DQ_TRIGGER_OWN_THREAD
static void iis328dq_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct iis328dq_data *iis328dq = p1;

	while (1) {
		k_sem_take(&iis328dq->gpio_sem, K_FOREVER);
		iis328dq_handle_interrupt(iis328dq->dev);
	}
}
#endif /* CONFIG_IIS328DQ_TRIGGER_OWN_THREAD */

#ifdef CONFIG_IIS328DQ_TRIGGER_GLOBAL_THREAD
static void iis328dq_work_cb(struct k_work *work)
{
	struct iis328dq_data *iis328dq = CONTAINER_OF(work, struct iis328dq_data, work);

	iis328dq_handle_interrupt(iis328dq->dev);
}
#endif /* CONFIG_IIS328DQ_TRIGGER_GLOBAL_THREAD */

int iis328dq_init_interrupt(const struct device *dev)
{
	struct iis328dq_data *iis328dq = dev->data;
	const struct iis328dq_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int ret;

	if (!cfg->gpio_int1.port && !cfg->gpio_int2.port) {
		/* no interrupts configured, nothing to do */
		return 0;
	}

	/* setup data ready gpio interrupt (INT1 and INT2) */
	if (cfg->gpio_int1.port) {
		if (!gpio_is_ready_dt(&cfg->gpio_int1)) {
			LOG_ERR("INT_1 pin is not ready");
			return -EINVAL;
		}
	}
	if (cfg->gpio_int2.port) {
		if (!gpio_is_ready_dt(&cfg->gpio_int2)) {
			LOG_ERR("INT_2 pin is not ready");
			return -EINVAL;
		}
	}

#if defined(CONFIG_IIS328DQ_TRIGGER_OWN_THREAD)
	k_sem_init(&iis328dq->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&iis328dq->thread, iis328dq->thread_stack,
			CONFIG_IIS328DQ_THREAD_STACK_SIZE, iis328dq_thread, iis328dq, NULL, NULL,
			K_PRIO_COOP(CONFIG_IIS328DQ_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_IIS328DQ_TRIGGER_GLOBAL_THREAD)
	iis328dq->work.handler = iis328dq_work_cb;
#endif /* CONFIG_IIS328DQ_TRIGGER_OWN_THREAD */

	if (cfg->gpio_int1.port) {
		ret = gpio_pin_configure_dt(&cfg->gpio_int1, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure INT_1 gpio");
			return ret;
		}
		gpio_init_callback(&iis328dq->int1_cb, iis328dq_int1_gpio_callback,
				   BIT(cfg->gpio_int1.pin));
		if (gpio_add_callback(cfg->gpio_int1.port, &iis328dq->int1_cb) < 0) {
			LOG_ERR("Could not set INT1 callback");
			return -EIO;
		}
	}

	if (cfg->gpio_int2.port) {
		ret = gpio_pin_configure_dt(&cfg->gpio_int2, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure INT_2 gpio");
			return ret;
		}
		gpio_init_callback(&iis328dq->int2_cb, iis328dq_int2_gpio_callback,
				   BIT(cfg->gpio_int2.pin));
		if (gpio_add_callback(cfg->gpio_int2.port, &iis328dq->int2_cb) < 0) {
			LOG_ERR("Could not set INT2 callback");
			return -EIO;
		}
	}

	if (iis328dq_int1_notification_set(ctx, IIS328DQ_INT1_PULSED) != 0) {
		return -EIO;
	}

	if (iis328dq_int2_notification_set(ctx, IIS328DQ_INT2_PULSED) != 0) {
		return -EIO;
	}

	return 0;
}
