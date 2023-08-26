/* Bosch BMI160 inertial measurement unit driver, trigger implementation
 *
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>

#include "bmi160.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(BMI160, CONFIG_SENSOR_LOG_LEVEL);

static void bmi160_handle_anymotion(const struct device *dev)
{
	struct bmi160_data *data = dev->data;

	if (data->handler_anymotion) {
		data->handler_anymotion(dev, data->trig_anymotion);
	}
}

static void bmi160_handle_drdy(const struct device *dev, uint8_t status)
{
	struct bmi160_data *data = dev->data;

#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
	if (data->handler_drdy_acc && (status & BMI160_STATUS_ACC_DRDY)) {
		data->handler_drdy_acc(dev, data->trig_drdy_acc);
	}
#endif

#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
	if (data->handler_drdy_gyr && (status & BMI160_STATUS_GYR_DRDY)) {
		data->handler_drdy_gyr(dev, data->trig_drdy_gyr);
	}
#endif
}

static void bmi160_handle_interrupts(const struct device *dev)
{
	union {
		uint8_t raw[5];
		struct {
			uint8_t status;
			uint8_t int_status[4];
		};
	} buf;

	if (bmi160_read(dev, BMI160_REG_STATUS, buf.raw, sizeof(buf)) < 0) {
		return;
	}

	if ((buf.int_status[0] & BMI160_INT_STATUS0_ANYM) &&
	    (buf.int_status[2] & (BMI160_INT_STATUS2_ANYM_FIRST_X |
				  BMI160_INT_STATUS2_ANYM_FIRST_Y |
				  BMI160_INT_STATUS2_ANYM_FIRST_Z))) {
		bmi160_handle_anymotion(dev);
	}

	if (buf.int_status[1] & BMI160_INT_STATUS1_DRDY) {
		bmi160_handle_drdy(dev, buf.status);
	}

}

#ifdef CONFIG_BMI160_TRIGGER_OWN_THREAD
static K_KERNEL_STACK_DEFINE(bmi160_thread_stack, CONFIG_BMI160_THREAD_STACK_SIZE);
static struct k_thread bmi160_thread;

static void bmi160_thread_main(struct bmi160_data *data)
{
	while (1) {
		k_sem_take(&data->sem, K_FOREVER);
		bmi160_handle_interrupts(data->dev);
	}
}
#endif

#ifdef CONFIG_BMI160_TRIGGER_GLOBAL_THREAD
static void bmi160_work_handler(struct k_work *work)
{
	struct bmi160_data *data = CONTAINER_OF(work, struct bmi160_data, work);

	bmi160_handle_interrupts(data->dev);
}
#endif

extern struct bmi160_data bmi160_data;

static void bmi160_gpio_callback(const struct device *port,
				 struct gpio_callback *cb, uint32_t pin)
{
	struct bmi160_data *data =
		CONTAINER_OF(cb, struct bmi160_data, gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pin);

#if defined(CONFIG_BMI160_TRIGGER_OWN_THREAD)
	k_sem_give(&data->sem);
#elif defined(CONFIG_BMI160_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static int bmi160_trigger_drdy_set(const struct device *dev,
				   enum sensor_channel chan,
				   const struct sensor_trigger *trig,
				   sensor_trigger_handler_t handler)
{
	struct bmi160_data *data = dev->data;
	uint8_t drdy_en = 0U;

#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
	if (chan == SENSOR_CHAN_ACCEL_XYZ) {
		data->handler_drdy_acc = handler;
		data->trig_drdy_acc = trig;
	}

	if (data->handler_drdy_acc) {
		drdy_en = BMI160_INT_DRDY_EN;
	}
#endif

#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
	if (chan == SENSOR_CHAN_GYRO_XYZ) {
		data->handler_drdy_gyr = handler;
		data->trig_drdy_gyr = trig;
	}

	if (data->handler_drdy_gyr) {
		drdy_en = BMI160_INT_DRDY_EN;
	}
#endif

	if (bmi160_reg_update(dev, BMI160_REG_INT_EN1,
			      BMI160_INT_DRDY_EN, drdy_en) < 0) {
		return -EIO;
	}

	return 0;
}

#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
static int bmi160_trigger_anym_set(const struct device *dev,
				   const struct sensor_trigger *trig,
				   sensor_trigger_handler_t handler)
{
	struct bmi160_data *data = dev->data;
	uint8_t anym_en = 0U;

	data->handler_anymotion = handler;
	data->trig_anymotion = trig;

	if (handler) {
		anym_en = BMI160_INT_ANYM_X_EN |
			  BMI160_INT_ANYM_Y_EN |
			  BMI160_INT_ANYM_Z_EN;
	}

	if (bmi160_reg_update(dev, BMI160_REG_INT_EN0,
			      BMI160_INT_ANYM_MASK, anym_en) < 0) {
		return -EIO;
	}

	return 0;
}

static int bmi160_trigger_set_acc(const struct device *dev,
				  const struct sensor_trigger *trig,
				  sensor_trigger_handler_t handler)
{
	if (trig->type == SENSOR_TRIG_DATA_READY) {
		return bmi160_trigger_drdy_set(dev, trig->chan, trig, handler);
	} else if (trig->type == SENSOR_TRIG_DELTA) {
		return bmi160_trigger_anym_set(dev, trig, handler);
	}

	return -ENOTSUP;
}

int bmi160_acc_slope_config(const struct device *dev,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	uint8_t acc_range_g, reg_val;
	uint32_t slope_th_ums2;

	if (attr == SENSOR_ATTR_SLOPE_TH) {
		if (bmi160_byte_read(dev, BMI160_REG_ACC_RANGE, &reg_val) < 0) {
			return -EIO;
		}

		acc_range_g = bmi160_acc_reg_val_to_range(reg_val);

		slope_th_ums2 = val->val1 * 1000000 + val->val2;

		/* make sure the provided threshold does not exceed range / 2 */
		if (slope_th_ums2 > (acc_range_g / 2 * SENSOR_G)) {
			return -EINVAL;
		}

		reg_val = (slope_th_ums2 - 1) * 512U / (acc_range_g * SENSOR_G);

		if (bmi160_byte_write(dev, BMI160_REG_INT_MOTION1,
				      reg_val) < 0) {
			return -EIO;
		}
	} else { /* SENSOR_ATTR_SLOPE_DUR */
		/* slope duration is measured in number of samples */
		if (val->val1 < 1 || val->val1 > 4) {
			return -ENOTSUP;
		}

		if (bmi160_reg_field_update(dev, BMI160_REG_INT_MOTION0,
					    BMI160_ANYM_DUR_POS,
					    BMI160_ANYM_DUR_MASK,
					    val->val1) < 0) {
			return -EIO;
		}
	}

	return 0;
}
#endif

#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
static int bmi160_trigger_set_gyr(const struct device *dev,
				  const struct sensor_trigger *trig,
				  sensor_trigger_handler_t handler)
{
	if (trig->type == SENSOR_TRIG_DATA_READY) {
		return bmi160_trigger_drdy_set(dev, trig->chan, trig, handler);
	}

	return -ENOTSUP;
}
#endif

int bmi160_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
#if !defined(CONFIG_BMI160_ACCEL_PMU_SUSPEND)
	if (trig->chan == SENSOR_CHAN_ACCEL_XYZ) {
		return bmi160_trigger_set_acc(dev, trig, handler);
	}
#endif
#if !defined(CONFIG_BMI160_GYRO_PMU_SUSPEND)
	if (trig->chan == SENSOR_CHAN_GYRO_XYZ) {
		return bmi160_trigger_set_gyr(dev, trig, handler);
	}
#endif
	return -ENOTSUP;
}

int bmi160_trigger_mode_init(const struct device *dev)
{
	struct bmi160_data *data = dev->data;
	const struct bmi160_cfg *cfg = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&cfg->interrupt)) {
		LOG_DBG("GPIO port %s not ready", cfg->interrupt.port->name);
		return -EINVAL;
	}

	data->dev = dev;

#if defined(CONFIG_BMI160_TRIGGER_OWN_THREAD)
	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&bmi160_thread, bmi160_thread_stack,
			CONFIG_BMI160_THREAD_STACK_SIZE,
			(k_thread_entry_t)bmi160_thread_main,
			data, NULL, NULL,
			K_PRIO_COOP(CONFIG_BMI160_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_BMI160_TRIGGER_GLOBAL_THREAD)
	data->work.handler = bmi160_work_handler;
#endif

	/* map all interrupts to INT1 pin */
	if (bmi160_word_write(dev, BMI160_REG_INT_MAP0, 0xf0ff) < 0) {
		LOG_DBG("Failed to map interrupts.");
		return -EIO;
	}

	ret = gpio_pin_configure_dt(&cfg->interrupt, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb,
			   bmi160_gpio_callback,
			   BIT(cfg->interrupt.pin));

	ret = gpio_add_callback(cfg->interrupt.port, &data->gpio_cb);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->interrupt,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	return bmi160_byte_write(dev, BMI160_REG_INT_OUT_CTRL,
				 BMI160_INT1_OUT_EN | BMI160_INT1_EDGE_CTRL);
}
