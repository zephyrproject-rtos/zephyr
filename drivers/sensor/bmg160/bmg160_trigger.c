/* Bosch BMG160 gyro driver, trigger implementation
 *
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * http://ae-bst.resource.bosch.com/media/_tech/media/datasheets/BST-BMG160-DS000-09.pdf
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

#include "bmg160.h"

extern struct bmg160_device_data bmg160_data;

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(BMG160, CONFIG_SENSOR_LOG_LEVEL);

static inline int setup_int(const struct device *dev,
			      bool enable)
{
	const struct bmg160_device_config *cfg = dev->config;

	return gpio_pin_interrupt_configure_dt(&cfg->int_gpio,
					       enable ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE);
}

static void bmg160_gpio_callback(const struct device *port,
				 struct gpio_callback *cb,
				 uint32_t pin)
{
	struct bmg160_device_data *bmg160 =
		CONTAINER_OF(cb, struct bmg160_device_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pin);

#if defined(CONFIG_BMG160_TRIGGER_OWN_THREAD)
	k_sem_give(&bmg160->trig_sem);
#elif defined(CONFIG_BMG160_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&bmg160->work);
#endif
}

static int bmg160_anymotion_set(const struct device *dev,
				const struct sensor_trigger *trig,
				sensor_trigger_handler_t handler)
{
	struct bmg160_device_data *bmg160 = dev->data;
	uint8_t anymotion_en = 0U;

	if (handler) {
		anymotion_en = BMG160_ANY_EN_X |
			       BMG160_ANY_EN_Y |
			       BMG160_ANY_EN_Z;
	}

	if (bmg160_update_byte(dev, BMG160_REG_ANY_EN,
			       BMG160_ANY_EN_MASK, anymotion_en) < 0) {
		return -EIO;
	}

	bmg160->anymotion_handler = handler;
	bmg160->anymotion_trig = trig;

	return 0;
}

static int bmg160_drdy_set(const struct device *dev,
			   const struct sensor_trigger *trig,
			   sensor_trigger_handler_t handler)
{
	struct bmg160_device_data *bmg160 = dev->data;

	if (bmg160_update_byte(dev, BMG160_REG_INT_EN0,
			       BMG160_DATA_EN,
			       handler ? BMG160_DATA_EN : 0) < 0) {
		return -EIO;
	}

	bmg160->drdy_handler = handler;
	bmg160->drdy_trig = trig;

	return 0;
}

int bmg160_slope_config(const struct device *dev, enum sensor_attribute attr,
			const struct sensor_value *val)
{
	struct bmg160_device_data *bmg160 = dev->data;

	if (attr == SENSOR_ATTR_SLOPE_TH) {
		uint16_t any_th_dps, range_dps;
		uint8_t any_th_reg_val;

		any_th_dps = sensor_rad_to_degrees(val);
		range_dps = BMG160_SCALE_TO_RANGE(bmg160->scale);
		any_th_reg_val = any_th_dps * 2000U / range_dps;

		/* the maximum slope depends on selected range */
		if (any_th_dps > range_dps / 16U) {
			return -ENOTSUP;
		}

		return bmg160_write_byte(dev, BMG160_REG_THRES,
					 any_th_dps & BMG160_THRES_MASK);
	} else if (attr == SENSOR_ATTR_SLOPE_DUR) {
		/* slope duration can be 4, 8, 12 or 16 samples */
		if (val->val1 != 4 && val->val1 != 8 &&
		    val->val1 != 12 && val->val1 != 16) {
			return -ENOTSUP;
		}

		return bmg160_write_byte(dev, BMG160_REG_ANY_EN,
			   (val->val1 << BMG160_ANY_DURSAMPLE_POS) &
			    BMG160_ANY_DURSAMPLE_MASK);
	}

	return -ENOTSUP;
}

int bmg160_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	const struct bmg160_device_config *config = dev->config;

	if (!config->int_gpio.port) {
		return -ENOTSUP;
	}

	if (trig->type == SENSOR_TRIG_DELTA) {
		return bmg160_anymotion_set(dev, trig, handler);
	} else if (trig->type == SENSOR_TRIG_DATA_READY) {
		return bmg160_drdy_set(dev, trig, handler);
	}

	return -ENOTSUP;
}

static int bmg160_handle_anymotion_int(const struct device *dev)
{
	struct bmg160_device_data *bmg160 = dev->data;

	if (bmg160->anymotion_handler) {
		bmg160->anymotion_handler(dev, bmg160->anymotion_trig);
	}

	return 0;
}

static int bmg160_handle_dataready_int(const struct device *dev)
{
	struct bmg160_device_data *bmg160 = dev->data;

	if (bmg160->drdy_handler) {
		bmg160->drdy_handler(dev, bmg160->drdy_trig);
	}

	return 0;
}

static void bmg160_handle_int(const struct device *dev)
{
	uint8_t status_int[4];

	if (bmg160_read(dev, BMG160_REG_INT_STATUS0, status_int, 4) < 0) {
		return;
	}

	if (status_int[0] & BMG160_ANY_INT) {
		bmg160_handle_anymotion_int(dev);
	} else {
		bmg160_handle_dataready_int(dev);
	}
}

#ifdef CONFIG_BMG160_TRIGGER_OWN_THREAD
static K_KERNEL_STACK_DEFINE(bmg160_thread_stack, CONFIG_BMG160_THREAD_STACK_SIZE);
static struct k_thread bmg160_thread;

static void bmg160_thread_main(struct bmg160_device_data *bmg160)
{
	while (true) {
		k_sem_take(&bmg160->trig_sem, K_FOREVER);

		bmg160_handle_int(bmg160->dev);
	}
}
#endif

#ifdef CONFIG_BMG160_TRIGGER_GLOBAL_THREAD
static void bmg160_work_cb(struct k_work *work)
{
	struct bmg160_device_data *bmg160 =
		CONTAINER_OF(work, struct bmg160_device_data, work);

	bmg160_handle_int(bmg160->dev);
}
#endif

int bmg160_trigger_init(const struct device *dev)
{
	const struct bmg160_device_config *cfg = dev->config;
	struct bmg160_device_data *bmg160 = dev->data;
	int ret;

	/* set INT1 pin to: push-pull, active low */
	if (bmg160_write_byte(dev, BMG160_REG_INT_EN1, 0) < 0) {
		LOG_DBG("Failed to select interrupt pins type.");
		return -EIO;
	}

	/* set interrupt mode to non-latched */
	if (bmg160_write_byte(dev, BMG160_REG_INT_RST_LATCH, 0) < 0) {
		LOG_DBG("Failed to set the interrupt mode.");
		return -EIO;
	}

	/* map anymotion and high rate interrupts to INT1 pin */
	if (bmg160_write_byte(dev, BMG160_REG_INT_MAP0,
			      BMG160_INT1_ANY | BMG160_INT1_HIGH) < 0) {
		LOG_DBG("Unable to map interrupts.");
		return -EIO;
	}

	/* map data ready, FIFO and FastOffset interrupts to INT1 pin */
	if (bmg160_write_byte(dev, BMG160_REG_INT_MAP1,
			      BMG160_INT1_DATA | BMG160_INT1_FIFO |
			      BMG160_INT1_FAST_OFFSET) < 0) {
		LOG_DBG("Unable to map interrupts.");
		return -EIO;
	}

	if (!device_is_ready(cfg->int_gpio.port)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	bmg160->dev = dev;

#if defined(CONFIG_BMG160_TRIGGER_OWN_THREAD)
	k_sem_init(&bmg160->trig_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&bmg160_thread, bmg160_thread_stack,
			CONFIG_BMG160_THREAD_STACK_SIZE,
			(k_thread_entry_t)bmg160_thread_main,
			bmg160, NULL, NULL,
			K_PRIO_COOP(CONFIG_BMG160_THREAD_PRIORITY), 0,
			K_NO_WAIT);

#elif defined(CONFIG_BMG160_TRIGGER_GLOBAL_THREAD)
	bmg160->work.handler = bmg160_work_cb;
#endif

	ret = gpio_pin_configure_dt(&cfg->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&bmg160->gpio_cb, bmg160_gpio_callback,
			   BIT(cfg->int_gpio.pin));

	ret = gpio_add_callback(cfg->int_gpio.port, &bmg160->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	return setup_int(dev, true);
}
