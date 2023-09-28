/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_sedi_i2c

#include <stdint.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <sedi_driver_i2c.h>
#include <zephyr/pm/device.h>

#define I2C_SEDI_TIMEOUT_MS (1000)

struct i2c_sedi_context {
	DEVICE_MMIO_RAM;
	int sedi_device;
	struct k_sem *sem;
	struct k_mutex *mutex;
	int err;
	uint16_t addr_10bit;
};

struct i2c_sedi_config {
	DEVICE_MMIO_ROM;
	sedi_i2c_event_cb_t cb_sedi;
	void (*irq_config)(const struct device *dev);
};

static int i2c_sedi_api_configure(const struct device *dev, uint32_t dev_config)
{
	int ret;
	int speed = I2C_SPEED_GET(dev_config);
	int sedi_speed;
	struct i2c_sedi_context *const context = dev->data;

	context->addr_10bit = (dev_config & I2C_ADDR_10_BITS) ? SEDI_I2C_ADDRESS_10BIT : 0;

	if (speed == I2C_SPEED_STANDARD) {
		sedi_speed = SEDI_I2C_BUS_SPEED_STANDARD;
	} else if (speed == I2C_SPEED_FAST) {
		sedi_speed = SEDI_I2C_BUS_SPEED_FAST;
	} else if (speed == I2C_SPEED_FAST_PLUS) {
		sedi_speed = SEDI_I2C_BUS_SPEED_FAST_PLUS;
	} else if (speed == I2C_SPEED_HIGH) {
		sedi_speed = SEDI_I2C_BUS_SPEED_HIGH;
	} else {
		return -EINVAL;
	}

	k_mutex_lock(context->mutex, K_FOREVER);
	ret = sedi_i2c_control(context->sedi_device, SEDI_I2C_BUS_SPEED, sedi_speed);
	k_mutex_unlock(context->mutex);

	if (ret != 0) {
		return -EIO;
	} else {
		return 0;
	}
}

static int i2c_sedi_api_full_io(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
				uint16_t addr)
{
	int ret = 0;
	struct i2c_sedi_context *context = dev->data;

	if (!num_msgs) {
		return 0;
	}
	__ASSERT_NO_MSG(msgs);

	k_mutex_lock(context->mutex, K_FOREVER);
	pm_device_busy_set(dev);

	for (int i = 0; i < num_msgs; i++) {
		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
			ret = sedi_i2c_master_write_async(
				context->sedi_device, addr | context->addr_10bit, msgs[i].buf,
				msgs[i].len, (msgs[i].flags & I2C_MSG_STOP) == 0x0);
		} else {
			ret = sedi_i2c_master_read_async(
				context->sedi_device, addr | context->addr_10bit, msgs[i].buf,
				msgs[i].len, (msgs[i].flags & I2C_MSG_STOP) == 0x0);
		}
		if (ret != 0) {
			ret = -EIO;
			break;
		}

		ret = k_sem_take(context->sem, K_MSEC(I2C_SEDI_TIMEOUT_MS));
		if (ret != 0) {
			break;
		}

		if (context->err != 0) {
			ret = -EIO;
			break;
		}
	}

	if (ret != 0) {
		/* Abort current transfer */
		sedi_i2c_control(context->sedi_device, SEDI_I2C_ABORT_TRANSFER, 0);
		ret = -EIO;
	}
	pm_device_busy_clear(dev);
	k_mutex_unlock(context->mutex);

	return ret;
}

static const struct i2c_driver_api i2c_sedi_apis = {.configure = i2c_sedi_api_configure,
						    .transfer = i2c_sedi_api_full_io};

#ifdef CONFIG_PM_DEVICE

static int i2c_sedi_suspend_device(const struct device *dev)
{
	struct i2c_sedi_context *const context = dev->data;
	int ret;

	if (pm_device_is_busy(dev)) {
		return -EBUSY;
	}

	ret = sedi_i2c_set_power(context->sedi_device, SEDI_POWER_SUSPEND);

	if (ret != 0) {
		return -EIO;
	}

	return 0;
}

static int i2c_sedi_resume_device_from_suspend(const struct device *dev)
{
	struct i2c_sedi_context *const context = dev->data;
	int ret;

	ret = sedi_i2c_set_power(context->sedi_device, SEDI_POWER_FULL);
	if (ret != 0) {
		return -EIO;
	}

	return 0;
}

static int i2c_sedi_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = i2c_sedi_suspend_device(dev);
		break;
	case PM_DEVICE_ACTION_RESUME:
		ret = i2c_sedi_resume_device_from_suspend(dev);
		break;

	default:
		ret = -ENOTSUP;
	}

	return ret;
}

#endif /* CONFIG_PM_DEVICE */

static int i2c_sedi_init(const struct device *dev)
{
	int ret;
	const struct i2c_sedi_config *const config = dev->config;
	struct i2c_sedi_context *const context = dev->data;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ret = sedi_i2c_init(context->sedi_device, config->cb_sedi, DEVICE_MMIO_GET(dev));
	if (ret != 0) {
		return -EIO;
	}

	ret = sedi_i2c_set_power(context->sedi_device, SEDI_POWER_FULL);
	if (ret != 0) {
		return -EIO;
	}

	config->irq_config(dev);

	return 0;
}

static void i2c_sedi_isr(const struct device *dev)
{
	struct i2c_sedi_context *const context = dev->data;

	sedi_i2c_isr_handler(context->sedi_device);
}

#define I2C_SEDI_IRQ_FLAGS_SENSE0(n) 0
#define I2C_SEDI_IRQ_FLAGS_SENSE1(n) DT_INST_IRQ(n, sense)
#define I2C_SEDI_IRQ_FLAGS(n) _CONCAT(I2C_SEDI_IRQ_FLAGS_SENSE, DT_INST_IRQ_HAS_CELL(n, sense))(n)

#define I2C_DEVICE_INIT_SEDI(n)                                                                    \
	static K_SEM_DEFINE(i2c_sedi_sem_##n, 0, 1);                                               \
	static K_MUTEX_DEFINE(i2c_sedi_mutex_##n);                                                 \
	static struct i2c_sedi_context i2c_sedi_data_##n = {                                       \
		.sedi_device = DT_INST_PROP(n, peripheral_id),                                     \
		.sem = &i2c_sedi_sem_##n,                                                          \
		.mutex = &i2c_sedi_mutex_##n,                                                      \
	};                                                                                         \
	static void i2c_sedi_callback_##n(const uint32_t event)                                    \
	{                                                                                          \
		if (event == SEDI_I2C_EVENT_TRANSFER_DONE) {                                       \
			i2c_sedi_data_##n.err = 0;                                                 \
		} else {                                                                           \
			i2c_sedi_data_##n.err = 1;                                                 \
		}                                                                                  \
		k_sem_give(i2c_sedi_data_##n.sem);                                                 \
	};                                                                                         \
	static void i2c_sedi_irq_config_##n(const struct device *dev)                              \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), i2c_sedi_isr,               \
			    DEVICE_DT_INST_GET(n), I2C_SEDI_IRQ_FLAGS(n));                         \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	static const struct i2c_sedi_config i2c_sedi_config_##n = {                                \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.cb_sedi = &i2c_sedi_callback_##n,                                                 \
		.irq_config = &i2c_sedi_irq_config_##n,                                            \
	};                                                                                         \
	PM_DEVICE_DT_DEFINE(DT_NODELABEL(i2c##n), i2c_sedi_pm_action);                             \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_sedi_init, PM_DEVICE_DT_GET(DT_NODELABEL(i2c##n)),        \
				  &i2c_sedi_data_##n, &i2c_sedi_config_##n, PRE_KERNEL_2,          \
				  CONFIG_I2C_INIT_PRIORITY, &i2c_sedi_apis);

DT_INST_FOREACH_STATUS_OKAY(I2C_DEVICE_INIT_SEDI)
