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

static void lis2ds12_gpio_callback(struct device *dev,
				  struct gpio_callback *cb, u32_t pins)
{
	struct lis2ds12_data *data =
		CONTAINER_OF(cb, struct lis2ds12_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, DT_INST_0_ST_LIS2DS12_IRQ_GPIOS_PIN);

#if defined(CONFIG_LIS2DS12_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_LIS2DS12_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

static void lis2ds12_handle_drdy_int(struct device *dev)
{
	struct lis2ds12_data *data = dev->driver_data;

	if (data->data_ready_handler != NULL) {
		data->data_ready_handler(dev, &data->data_ready_trigger);
	}
}

static void lis2ds12_handle_int(void *arg)
{
	struct device *dev = arg;
	struct lis2ds12_data *data = dev->driver_data;
	u8_t status;

	if (data->hw_tf->read_reg(data, LIS2DS12_REG_STATUS, &status) < 0) {
		LOG_ERR("status reading error");
		return;
	}

	if (status & LIS2DS12_INT_DRDY) {
		lis2ds12_handle_drdy_int(dev);
	}

	gpio_pin_enable_callback(data->gpio, DT_INST_0_ST_LIS2DS12_IRQ_GPIOS_PIN);
}

#ifdef CONFIG_LIS2DS12_TRIGGER_OWN_THREAD
static void lis2ds12_thread(int dev_ptr, int unused)
{
	struct device *dev = INT_TO_POINTER(dev_ptr);
	struct lis2ds12_data *data = dev->driver_data;

	ARG_UNUSED(unused);

	while (1) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		lis2ds12_handle_int(dev);
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

static int lis2ds12_init_interrupt(struct device *dev)
{
	struct lis2ds12_data *data = dev->driver_data;

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

int lis2ds12_trigger_init(struct device *dev)
{
	struct lis2ds12_data *data = dev->driver_data;

	/* setup data ready gpio interrupt */
	data->gpio = device_get_binding(DT_INST_0_ST_LIS2DS12_IRQ_GPIOS_CONTROLLER);
	if (data->gpio == NULL) {
		LOG_ERR("Cannot get pointer to %s device.",
			    DT_INST_0_ST_LIS2DS12_IRQ_GPIOS_CONTROLLER);
		return -EINVAL;
	}

	gpio_pin_configure(data->gpio, DT_INST_0_ST_LIS2DS12_IRQ_GPIOS_PIN,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_HIGH | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&data->gpio_cb,
			   lis2ds12_gpio_callback,
			   BIT(DT_INST_0_ST_LIS2DS12_IRQ_GPIOS_PIN));

	if (gpio_add_callback(data->gpio, &data->gpio_cb) < 0) {
		LOG_ERR("Could not set gpio callback.");
		return -EIO;
	}

#if defined(CONFIG_LIS2DS12_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, UINT_MAX);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_LIS2DS12_THREAD_STACK_SIZE,
			(k_thread_entry_t)lis2ds12_thread, dev,
			0, NULL, K_PRIO_COOP(CONFIG_LIS2DS12_THREAD_PRIORITY),
			0, K_NO_WAIT);
#elif defined(CONFIG_LIS2DS12_TRIGGER_GLOBAL_THREAD)
	data->work.handler = lis2ds12_work_cb;
	data->dev = dev;
#endif

	gpio_pin_enable_callback(data->gpio, DT_INST_0_ST_LIS2DS12_IRQ_GPIOS_PIN);

	return 0;
}

int lis2ds12_trigger_set(struct device *dev,
			const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct lis2ds12_data *data = dev->driver_data;
	u8_t buf[6];

	__ASSERT_NO_MSG(trig->type == SENSOR_TRIG_DATA_READY);

	gpio_pin_disable_callback(data->gpio, DT_INST_0_ST_LIS2DS12_IRQ_GPIOS_PIN);

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
	gpio_pin_enable_callback(data->gpio, DT_INST_0_ST_LIS2DS12_IRQ_GPIOS_PIN);

	return 0;
}

