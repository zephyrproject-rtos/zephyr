/* ST Microelectronics LIS2DS12 3-axis accelerometer driver
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lis2ds12.pdf
 */

#include <device.h>
#include <drivers/i2c.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <logging/log.h>
#include "lis2ds12.h"

LOG_MODULE_DECLARE(LIS2DS12, CONFIG_SENSOR_LOG_LEVEL);

static void lis2ds12_gpio_callback(const struct device *dev,
				   struct gpio_callback *cb, uint32_t pins)
{
	struct lis2ds12_data *data =
		CONTAINER_OF(cb, struct lis2ds12_data, gpio_cb);
	const struct lis2ds12_config *cfg = data->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure(data->gpio, cfg->irq_pin,
				     GPIO_INT_DISABLE);

#if defined(CONFIG_LIS2DS12_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_LIS2DS12_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void lis2ds12_handle_drdy_int(const struct device *dev)
{
	struct lis2ds12_data *data = dev->data;

	if (data->data_ready_handler != NULL) {
		data->data_ready_handler(dev, &data->data_ready_trigger);
	}
}

static void lis2ds12_handle_int(const struct device *dev)
{
	struct lis2ds12_data *data = dev->data;
	const struct lis2ds12_config *cfg = dev->config;
	uint8_t status;

	if (data->hw_tf->read_reg(data, LIS2DS12_REG_STATUS, &status) < 0) {
		LOG_ERR("status reading error");
		return;
	}

	if (status & LIS2DS12_INT_DRDY) {
		lis2ds12_handle_drdy_int(dev);
	}

	gpio_pin_interrupt_configure(data->gpio, cfg->irq_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);
}

#ifdef CONFIG_LIS2DS12_TRIGGER_OWN_THREAD
static void lis2ds12_thread(struct lis2ds12_data *data)
{
	while (1) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		lis2ds12_handle_int(data->dev);
	}
}
#endif

#ifdef CONFIG_LIS2DS12_TRIGGER_GLOBAL_THREAD
static void lis2ds12_work_cb(struct k_work *work)
{
	struct lis2ds12_data *data =
		CONTAINER_OF(work, struct lis2ds12_data, work);

	lis2ds12_handle_int(data->dev);
}
#endif

static int lis2ds12_init_interrupt(const struct device *dev)
{
	struct lis2ds12_data *data = dev->data;

	/* Enable latched mode */
	if (data->hw_tf->update_reg(data,
				    LIS2DS12_REG_CTRL3,
				    LIS2DS12_MASK_LIR,
				    (1 << LIS2DS12_SHIFT_LIR)) < 0) {
		LOG_ERR("Could not enable LIR mode.");
		return -EIO;
	}

	/* enable data-ready interrupt */
	if (data->hw_tf->update_reg(data,
				    LIS2DS12_REG_CTRL4,
				    LIS2DS12_MASK_INT1_DRDY,
				    (1 << LIS2DS12_SHIFT_INT1_DRDY)) < 0) {
		LOG_ERR("Could not enable data-ready interrupt.");
		return -EIO;
	}

	return 0;
}

int lis2ds12_trigger_init(const struct device *dev)
{
	struct lis2ds12_data *data = dev->data;
	const struct lis2ds12_config *cfg = dev->config;

	/* setup data ready gpio interrupt */
	data->gpio = device_get_binding(cfg->irq_port);
	if (data->gpio == NULL) {
		LOG_ERR("Cannot get pointer to %s device.", cfg->irq_port);
		return -EINVAL;
	}

	gpio_pin_configure(data->gpio, cfg->irq_pin,
			   GPIO_INPUT | cfg->irq_flags);

	gpio_init_callback(&data->gpio_cb,
			   lis2ds12_gpio_callback,
			   BIT(cfg->irq_pin));

	if (gpio_add_callback(data->gpio, &data->gpio_cb) < 0) {
		LOG_ERR("Could not set gpio callback.");
		return -EIO;
	}
	data->dev = dev;

#if defined(CONFIG_LIS2DS12_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, UINT_MAX);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_LIS2DS12_THREAD_STACK_SIZE,
			(k_thread_entry_t)lis2ds12_thread,
			data, NULL, NULL,
			K_PRIO_COOP(CONFIG_LIS2DS12_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_LIS2DS12_TRIGGER_GLOBAL_THREAD)
	data->work.handler = lis2ds12_work_cb;
#endif

	gpio_pin_interrupt_configure(data->gpio, cfg->irq_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}

int lis2ds12_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler)
{
	struct lis2ds12_data *data = dev->data;
	const struct lis2ds12_config *cfg = dev->config;
	uint8_t buf[6];

	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

	gpio_pin_interrupt_configure(data->gpio, cfg->irq_pin,
				     GPIO_INT_DISABLE);

	data->data_ready_handler = handler;
	if (handler == NULL) {
		LOG_WRN("lis2ds12: no handler");
		return 0;
	}

	/* re-trigger lost interrupt */
	if (data->hw_tf->read_data(data, LIS2DS12_REG_OUTX_L,
				   buf, sizeof(buf)) < 0) {
		LOG_ERR("status reading error");
		return -EIO;
	}

	data->data_ready_trigger = *trig;

	lis2ds12_init_interrupt(dev);
	gpio_pin_interrupt_configure(data->gpio, cfg->irq_pin,
				     GPIO_INT_EDGE_TO_ACTIVE);

	return 0;
}
