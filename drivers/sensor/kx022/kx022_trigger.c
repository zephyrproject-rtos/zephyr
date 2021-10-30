/* Kionix KX022 3-axis accelerometer driver
 *
 * Copyright (c) 2021 G-Technologies Sdn. Bhd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <device.h>
#include <drivers/i2c.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include "kx022.h"

LOG_MODULE_DECLARE(KX022, CONFIG_SENSOR_LOG_LEVEL);

static void kx022_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct kx022_data *data = CONTAINER_OF(cb, struct kx022_data, gpio_cb);
	const struct kx022_config *cfg = data->dev->config;

	ARG_UNUSED(pins);
	gpio_pin_interrupt_configure(data->gpio, cfg->irq_pin, GPIO_INT_DISABLE);
#if defined(CONFIG_KX022_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_KX022_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void kx022_handle_motion_int(const struct device *dev)
{
	struct kx022_data *data = dev->data;

	if (data->motion_handler != NULL) {
		data->motion_handler(dev, &data->motion_trigger);
	}
}

static void kx022_handle_drdy_int(const struct device *dev)
{
	struct kx022_data *data = dev->data;

	if (data->drdy_handler != NULL) {
		data->drdy_handler(dev, &data->drdy_trigger);
	}
}
static void kx022_handle_slope_int(const struct device *dev)
{
	struct kx022_data *data = dev->data;

	if (data->slope_handler != NULL) {
		data->slope_handler(dev, &data->slope_trigger);
	}
}

static void kx022_handle_int(const struct device *dev)
{
	struct kx022_data *data = dev->data;
	const struct kx022_config *cfg = dev->config;
	uint8_t status, clr;

	if (data->hw_tf->read_reg(data, KX022_REG_INS2, &status) < 0) {
		return;
	}

	if (status & 0x02) {
		kx022_handle_motion_int(dev);
	} else if (status & 0x10) {
		kx022_handle_drdy_int(dev);
		k_msleep(50);

	} else if (status & 0x01) {
		kx022_handle_slope_int(dev);
	}
	data->hw_tf->read_reg(data, KX022_REG_INT_REL, &clr);
	gpio_pin_interrupt_configure(data->gpio, cfg->irq_pin, GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_KX022_TRIGGER_OWN_THREAD
static void kx022_thread(struct kx022_data *data)
{
	while (1) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		kx022_handle_int(data->dev);
	}
}
#endif

#ifdef CONFIG_KX022_TRIGGER_GLOBAL_THREAD
static void kx022_work_cb(struct k_work *work)
{
	struct kx022_data *data = CONTAINER_OF(work, struct kx022_data, work);

	kx022_handle_int(data->dev);
}
#endif

/**************************************************************
 * FUNCTION : kx022_trigger_init
 * use to initialize trigger function
 *
 * ************************************************************/
int kx022_trigger_init(const struct device *dev)
{
	struct kx022_data *data = dev->data;
	const struct kx022_config *cfg = dev->config;

	/* setup data ready gpio interrupt */
	data->gpio = device_get_binding(cfg->irq_port);
	if (data->gpio == NULL) {
		LOG_ERR("Cannot get pointer to %s device.", cfg->irq_port);
		return -EINVAL;
	}

	gpio_pin_configure(data->gpio, cfg->irq_pin, GPIO_INPUT | cfg->irq_flags);

	gpio_init_callback(&data->gpio_cb, kx022_gpio_callback, BIT(cfg->irq_pin));

	if (gpio_add_callback(data->gpio, &data->gpio_cb) < 0) {
		LOG_ERR("Could not set gpio callback.");
		return -EIO;
	}
	data->dev = dev;

	kx022_mode(dev, KX022_STANDY_MODE);

	if (IS_ENABLED(CONFIG_KX022_INT1_PIN)) {
		data->hw_tf->update_reg(data, KX022_REG_INC1, KX022_MAKS_INC1_INT_EN,
					KX022_INT1_EN);
	} else {
		data->hw_tf->update_reg(data, KX022_REG_INC5, KX022_MAKS_INT2_EN, KX022_INT2_EN);
	}

	kx022_mode(dev, KX022_OPERATING_MODE);

#if defined(CONFIG_KX022_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, UINT_MAX);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_KX022_THREAD_STACK_SIZE,
			(k_thread_entry_t)kx022_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_KX022_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_KX022_TRIGGER_GLOBAL_THREAD)
	data->work.handler = kx022_work_cb;

#endif

	gpio_pin_interrupt_configure(data->gpio, cfg->irq_pin, GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}

void kx022_motion_setup(const struct device *dev, sensor_trigger_handler_t handler)
{
	struct kx022_data *data = dev->data;

	data->motion_handler = handler;

	/* Enable Motion detection */
	data->hw_tf->update_reg(data, KX022_REG_CNTL1, KX022_MASK_CNTL1_WUFE, KX022_CNTL1_WUFE);

	data->hw_tf->update_reg(data, KX022_REG_ODCNTL, KX022_MASK_ODCNTL_OSA, KX022_ODCNTL_50HZ);

	data->hw_tf->write_reg(data, KX022_REG_INC2, KX022_DEFAULT_INC2);

	data->hw_tf->write_reg(data, KX022_REG_WUFC, KX022_WUFC_DUR);
	data->hw_tf->write_reg(data, KX022_REG_ATH, KX022_ATH_THS);

	if (IS_ENABLED(CONFIG_KX022_INT1_PIN)) {
		data->hw_tf->update_reg(data, KX022_REG_INC4, KX022_MAKS_INC4_WUFI1,
					KX022_INC4_WUFI1);
	} else {
		data->hw_tf->update_reg(data, KX022_REG_INC6, KX022_MAKS_INC6_WUFI2,
					KX022_MAKS_INC6_WUFI2);
	}
}

void kx022_drdy_setup(const struct device *dev, sensor_trigger_handler_t handler)
{
	struct kx022_data *data = dev->data;

	data->drdy_handler = handler;

	data->hw_tf->update_reg(data, KX022_REG_CNTL1, KX022_MASK_CNTL1_DRDYE, KX022_CNTL1_DRDYE);
	data->hw_tf->update_reg(data, KX022_REG_ODCNTL, KX022_MASK_ODCNTL_OSA, KX022_ODCNTL_50HZ);

	if (IS_ENABLED(CONFIG_KX022_INT1_PIN)) {
		data->hw_tf->update_reg(data, KX022_REG_INC4, KX022_MAKS_INC4_DRDYI1,
					KX022_INC4_DRDYI1);
	} else {
		data->hw_tf->update_reg(data, KX022_REG_INC6, KX022_MAKS_INC6_DRDYI2,
					KX022_INC6_DRDYI2);
	}
}

void kx022_tilt_setup(const struct device *dev, sensor_trigger_handler_t handler)
{
	struct kx022_data *data = dev->data;

	data->slope_handler = handler;

	data->hw_tf->update_reg(data, KX022_REG_CNTL1, KX022_MASK_CNTL1_TPE, KX022_CNTL1_TPE_EN);

	k_msleep(50);
	data->hw_tf->write_reg(data, KX022_REG_CNTL2, KX022_CNTL_TILT_ALL_EN);
	k_msleep(50);

	data->hw_tf->update_reg(data, KX022_REG_CNTL3, KX022_MASK_CNTL3_OTP,
				(KX022_DEFAULT_TILT_ODR << KX022_OTP_SHIFT));
	k_msleep(50);

	data->hw_tf->write_reg(data, KX022_REG_TILT_TIMER, KX022_TILT_TIMES_DURATION);
	k_msleep(50);

	data->hw_tf->write_reg(data, KX022_REG_TILT_ANGLE_LL, KX022_DEF_TILT_ANGLE_LL);
	k_msleep(50);
	data->hw_tf->write_reg(data, KX022_REG_TILT_ANGLE_HL, KX022_DEF_TILT_ANGLE_HL);
	k_msleep(50);
	data->hw_tf->write_reg(data, KX022_REG_HYST_SET, KX022_DEF_HYST_SET);
	k_msleep(50);

	if (IS_ENABLED(CONFIG_KX022_INT1_PIN)) {
		data->hw_tf->update_reg(data, KX022_REG_INC4, KX022_MAKS_INC6_TPI2,
					KX022_INC4_TPI1);
	} else {
		data->hw_tf->update_reg(data, KX022_REG_INC6, KX022_MAKS_INC6_TPI2,
					KX022_INC6_TPI2);
	}

	k_msleep(100);
}

void kx022_clear_setup(const struct device *dev)
{
	struct kx022_data *data = dev->data;

	data->hw_tf->write_reg(data, KX022_REG_CNTL1, KX022_DEFAULT_CNTL1);
	k_msleep(50);
}
int kx022_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		      sensor_trigger_handler_t handler)
{
	struct kx022_data *data = dev->data;
	const struct kx022_config *cfg = dev->config;
	uint8_t buf[6];

	kx022_mode(dev, KX022_STANDY_MODE);

	switch (trig->type) {
	case SENSOR_TRIG_FREEFALL:
		kx022_clear_setup(dev);
		kx022_motion_setup(dev, handler);
		kx022_mode(dev, KX022_OPERATING_MODE);
		break;
	case SENSOR_TRIG_DATA_READY:
		kx022_clear_setup(dev);
		kx022_drdy_setup(dev, handler);
		kx022_mode(dev, KX022_OPERATING_MODE);
		break;
	case SENSOR_TRIG_ALL:
		kx022_drdy_setup(dev, handler);
		kx022_motion_setup(dev, handler);
		kx022_tilt_setup(dev, handler);
		kx022_mode(dev, KX022_OPERATING_MODE);
		break;
	case SENSOR_TRIG_NEAR_FAR:
		kx022_clear_setup(dev);
		kx022_tilt_setup(dev, handler);
		kx022_mode(dev, KX022_OPERATING_MODE);
		break;
	default:
		return -ENOTSUP;
	}
	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DELTA);

	gpio_pin_interrupt_configure(data->gpio, cfg->irq_pin, GPIO_INT_DISABLE);

	if (handler == NULL) {
		LOG_WRN("kx022: no handler");
		return 0;
	}

	/* re-trigger lost interrupt */
	if (data->hw_tf->read_data(data, KX022_REG_XOUT_L, buf, sizeof(buf)) < 0) {
		LOG_ERR("status reading error");
		return -EIO;
	}

	gpio_pin_interrupt_configure(data->gpio, cfg->irq_pin, GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}
