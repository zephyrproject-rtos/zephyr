/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "icm20948.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(icm20948, CONFIG_SENSOR_LOG_LEVEL);

static int icm20948_enable_drdy_int(const struct device *dev, bool enable)
{
	uint8_t val = enable ? ICM20948_DRDY_EN_MASK : 0;

	return icm20948_write_reg(dev, ICM20948_REG_INT_ENABLE_1, val);
}

static int icm20948_enable_wom_int(const struct device *dev, bool enable)
{
	int ret;
	uint8_t val;

	/* Read current INT_ENABLE register */
	ret = icm20948_read_reg(dev, ICM20948_REG_INT_ENABLE, &val);
	if (ret < 0) {
		return ret;
	}

	if (enable) {
		val |= ICM20948_INT_ENABLE_WOM_EN_MASK;
	} else {
		val &= ~ICM20948_INT_ENABLE_WOM_EN_MASK;
	}

	return icm20948_write_reg(dev, ICM20948_REG_INT_ENABLE, val);
}

int icm20948_config_wom(const struct device *dev, uint8_t threshold_mg)
{
	int ret;

	/* Configure WOM threshold (Bank 2 register)
	 * LSB = 4mg, range 0-1020mg
	 * threshold_mg is in mg, divide by 4 to get register value
	 */
	uint8_t thr_val = threshold_mg / 4;

	if (thr_val > 255) {
		thr_val = 255;
	}

	ret = icm20948_write_reg(dev, ICM20948_REG_ACCEL_WOM_THR, thr_val);
	if (ret < 0) {
		LOG_ERR("Failed to set WOM threshold");
		return ret;
	}

	/* Enable WOM logic and set compare mode to previous sample */
	uint8_t ctrl = ICM20948_ACCEL_INTEL_CTRL_EN_MASK | ICM20948_ACCEL_INTEL_CTRL_MODE_PREVIOUS;

	ret = icm20948_write_reg(dev, ICM20948_REG_ACCEL_INTEL_CTRL, ctrl);
	if (ret < 0) {
		LOG_ERR("Failed to configure WOM control");
		return ret;
	}

	LOG_DBG("WOM configured: threshold=%d mg (reg=0x%02x)", threshold_mg, thr_val);
	return 0;
}

int icm20948_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct icm20948_data *data = dev->data;
	const struct icm20948_config *cfg = dev->config;
	int ret;

	if (trig->type != SENSOR_TRIG_DATA_READY && trig->type != SENSOR_TRIG_MOTION) {
		LOG_ERR("Unsupported trigger type: %d", trig->type);
		return -ENOTSUP;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_pin, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Failed to disable gpio interrupt.");
		return ret;
	}

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		data->drdy_handler = handler;
		data->drdy_trigger = trig;

		ret = icm20948_enable_drdy_int(dev, handler != NULL);
		if (ret < 0) {
			LOG_ERR("Failed to configure DRDY interrupt");
			return ret;
		}
	} else if (trig->type == SENSOR_TRIG_MOTION) {
		data->motion_handler = handler;
		data->motion_trigger = trig;

		ret = icm20948_enable_wom_int(dev, handler != NULL);
		if (ret < 0) {
			LOG_ERR("Failed to configure WOM interrupt");
			return ret;
		}
	} else {
		return -ENOTSUP;
	}

	/* Re-enable GPIO interrupt if any handler is active */
	if (data->drdy_handler != NULL || data->motion_handler != NULL) {
		ret = gpio_pin_interrupt_configure_dt(&cfg->int_pin, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to enable gpio interrupt.");
			return ret;
		}
	}

	return 0;
}

static void icm20948_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct icm20948_data *data = CONTAINER_OF(cb, struct icm20948_data, gpio_cb);
	const struct icm20948_config *cfg = data->dev->config;
	int ret;

	ARG_UNUSED(pins);

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_pin, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Disabling gpio interrupt failed with err: %d", ret);
		return;
	}

#if defined(CONFIG_ICM20948_TRIGGER_OWN_THREAD)
	k_sem_give(&data->gpio_sem);
#elif defined(CONFIG_ICM20948_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void icm20948_thread_cb(const struct device *dev)
{
	struct icm20948_data *data = dev->data;
	const struct icm20948_config *cfg = dev->config;
	uint8_t int_status = 0;
	uint8_t int_status_1 = 0;
	int ret;

	/* Read interrupt status registers to determine which interrupt fired */
	icm20948_read_reg(dev, ICM20948_REG_INT_STATUS, &int_status);
	icm20948_read_reg(dev, ICM20948_REG_INT_STATUS_1, &int_status_1);

	/* Check for Wake-on-Motion interrupt */
	if ((int_status & ICM20948_INT_STATUS_WOM) && data->motion_handler != NULL) {
		data->motion_handler(dev, data->motion_trigger);
	}

	/* Check for Data Ready interrupt */
	if ((int_status_1 & ICM20948_INT_STATUS_1_DRDY) && data->drdy_handler != NULL) {
		data->drdy_handler(dev, data->drdy_trigger);
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->int_pin, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Enabling gpio interrupt failed with err: %d", ret);
	}
}

#ifdef CONFIG_ICM20948_TRIGGER_OWN_THREAD
static void icm20948_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct icm20948_data *data = p1;

	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		icm20948_thread_cb(data->dev);
	}
}
#endif

#ifdef CONFIG_ICM20948_TRIGGER_GLOBAL_THREAD
static void icm20948_work_cb(struct k_work *work)
{
	struct icm20948_data *data = CONTAINER_OF(work, struct icm20948_data, work);

	icm20948_thread_cb(data->dev);
}
#endif

int icm20948_init_interrupt(const struct device *dev)
{
	struct icm20948_data *data = dev->data;
	const struct icm20948_config *cfg = dev->config;
	int ret;

	/* Setup GPIO interrupt pin */
	if (!gpio_is_ready_dt(&cfg->int_pin)) {
		LOG_ERR("Interrupt pin is not ready.");
		return -EIO;
	}

	data->dev = dev;

	ret = gpio_pin_configure_dt(&cfg->int_pin, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt pin.");
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, icm20948_gpio_callback, BIT(cfg->int_pin.pin));

	ret = gpio_add_callback(cfg->int_pin.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set gpio callback.");
		return ret;
	}

	/* Configure INT pin: active high, push-pull, pulse mode */
	ret = icm20948_write_reg(dev, ICM20948_REG_INT_PIN_CFG, 0x00);
	if (ret < 0) {
		LOG_ERR("Failed to configure INT pin.");
		return ret;
	}

#if defined(CONFIG_ICM20948_TRIGGER_OWN_THREAD)
	ret = k_sem_init(&data->gpio_sem, 0, K_SEM_MAX_LIMIT);
	if (ret < 0) {
		LOG_ERR("Failed to initialize semaphore");
		return ret;
	}

	k_thread_create(&data->thread, data->thread_stack, CONFIG_ICM20948_THREAD_STACK_SIZE,
			icm20948_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ICM20948_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ICM20948_TRIGGER_GLOBAL_THREAD)
	data->work.handler = icm20948_work_cb;
#endif

	LOG_DBG("Interrupt initialized on pin %d", cfg->int_pin.pin);
	return 0;
}
