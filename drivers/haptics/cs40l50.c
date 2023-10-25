/*
 * Copyright 2023 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT cirrus_cs40l50

#include "cs40l50.h"

#include <zephyr/sys/byteorder.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(cirrus_cs40l50);

enum cs40l50_pm_state {
	CS40L50_PM_STATE_HIBERNATE = 0,
	CS40L50_PM_STATE_WAKEUP,
	CS40L50_PM_STATE_PREVENT_HIBERNATE,
	CS40L50_PM_STATE_ALLOW_HIBERNATE,
	CS40L50_PM_STATE_SHUTDOWN,
};

struct cs40l50_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec reset_gpio;
	void (*irq_cfg_func)(void);
	void (*irq_enable_func)(void);
	void (*irq_disable_func)(void);
};

struct cs40l50_data {
	uint32_t rev_id;
};

static int cs40l50_i2c_write_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
				    const uint32_t value)
{
	uint8_t addr_buf[4], data_buf[4], msg_buf[8];
	int i, j = 4;

	sys_put_le32(reg_addr, addr_buf);
	sys_put_le32(value, data_buf);

	for (i = 0; i < ARRAY_SIZE(addr_buf); i++) {
		msg_buf[i] = addr_buf[i];
		msg_buf[j] = data_buf[i];
		j++;
	}

	return i2c_write_dt(spec, msg_buf, sizeof(msg_buf));
}

static int cs40l50_i2c_read_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
				   uint32_t *value)
{
	uint8_t write_buf[4], read_buf[4];
	int ret;

	sys_put_le32(reg_addr, write_buf);

	ret = i2c_write_read_dt(spec, write_buf, sizeof(write_buf), read_buf, sizeof(read_buf));
	if (ret < 0) {
		return ret;
	}

	*value = sys_get_le32(read_buf);

	return 0;
}

static int cs40l50_update_reg_dt(const struct i2c_dt_spec *spec, const uint32_t reg_addr,
				 const uint32_t mask, const uint32_t value)
{
	uint32_t old_value, new_value;
	int ret;

	ret = cs40l50_i2c_read_reg_dt(spec, reg_addr, &old_value);
	if (ret < 0) {
		return ret;
	}

	new_value = (old_value & ~mask) | (value & mask);
	if (new_value == old_value) {
		return 0;
	}

	return cs40l50_i2c_write_reg_dt(spec, reg_addr, new_value);
}

static int cs40l50_apply_errata(const struct device *dev)
{
	return 0;
}

static int cs40l50_reset(const struct device *dev)
{
	const struct cs40l50_config *config = dev->config;
	struct cs40l50_data *data = dev->data;
	uint32_t devid, value;
	int i = 10, ret;

	gpio_pin_set_dt(&config->reset_gpio, true);
	k_usleep(CS40L50_T_RLPW_US);
	gpio_pin_set_dt(&config->reset_gpio, false);
	k_usleep(CS40L50_T_IRS_US);

	ret = cs40l50_i2c_read_reg_dt(&config->i2c, CS40L50_REG_DEVID, &devid);
	if (ret < 0) {
		return ret;
	}

	ret = cs40l50_i2c_read_reg_dt(&config->i2c, CS40L50_REG_REVID, &data->rev_id);
	if (ret < 0) {
		return ret;
	}

	if (data->rev_id != CS40L50_REVID_B0) {
		return -ENODEV;
	}

	do {
		ret = cs40l50_i2c_read_reg_dt(&config->i2c, CS40L50_REG_DSP_STATUS_0, &value);
		if (ret < 0) {
			return ret;
		}

		value &= CS40L50_HALO_STATE;

		if (value == CS40L50_HALO_STATE_RUNNING) {
			break;
		}

		k_msleep(10);

		i--;
	} while (i > 0);

	LOG_INF("Found %s, DEVID:0x%x, REVID:0x%x\n", dev->name, devid, data->rev_id);

	ret = cs40l50_apply_errata(dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int haptics_cs40l50_preempt_output(const struct device *dev)
{
	return 0;
}

static void haptics_cs40l50_stop_output(const struct device *dev)
{
}

static int haptics_cs40l50_start_output(const struct device *dev)
{
	return 0;
}

static void cs40l50_isr(void *arg)
{
}

static int cs40l50_boot(const struct device *dev)
{
}

static int cs40l50_pm_state_transition(const struct device *dev, enum cs40l50_pm_state state)
{
	const struct cs40l50_config *config = dev->config;
	uint32_t cmd, value;
	int i, ret;

	cmd = state + CS40L50_DSP_MBOX_PM_CMD_BASE;

	switch (state) {
	case CS40L50_PM_STATE_WAKEUP:
	case CS40L50_PM_STATE_PREVENT_HIBERNATE:
		ret = cs40l50_i2c_write_reg_dt(&config->i2c, CS40L50_REG_DSP_VIRTUAL1_MBOX_1, cmd);
		if (ret < 0) {
			return ret;
		}

		for (i = 0; i < 10; i++) {
			k_msleep(10);

			cs40l50_i2c_read_reg_dt(&config->i2c, CS40L50_REG_DSP_VIRTUAL1_MBOX_1,
						&value);

			if (value == CS40L50_DSP_MBOX_RESET) {
				return 0;
			}
		}

		return -EIO;
	case CS40L50_PM_STATE_HIBERNATE:
	case CS40L50_PM_STATE_ALLOW_HIBERNATE:
	case CS40L50_PM_STATE_SHUTDOWN:
		return cs40l50_i2c_write_reg_dt(&config->i2c, CS40L50_REG_DSP_VIRTUAL1_MBOX_1, cmd);
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int cs40l50_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return cs40l50_pm_state_transition(dev, CS40L50_PM_STATE_WAKEUP);
	case PM_DEVICE_ACTION_SUSPEND:
		return cs40l50_pm_state_transition(dev, CS40L50_PM_STATE_SHUTDOWN);
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int cs40l50_init(const struct device *dev)
{
	const struct cs40l50_config *config = dev->config;
	int ret;

	ret = cs40l50_reset(dev);
	if (ret < 0) {
		return ret;
	}

	/* to-do */

	config->irq_cfg_func();
	config->irq_enable_func();

	return 0;
}

static const struct haptics_driver_api cs40l50_driver_api = {
	.start_output = &haptics_cs40l50_start_output,
	.stop_output = &haptics_cs40l50_stop_output,
	.preempt_output = &haptics_cs40l50_preempt_output,
};

#define CS40L50_INIT(inst)                                                                         \
                                                                                                   \
	static void cs40l50_config_func_##inst(void);                                              \
	static void cs40l50_enable_func_##inst(void);                                              \
	static void cs40l50_disable_func_##inst(void);                                             \
                                                                                                   \
	static const struct cs40l50_config cs40l50_config_##inst = {                               \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.irq_cfg_func = cs40l50_config_func_##inst,                                        \
		.irq_enable_func = cs40l50_enable_func_##inst,                                     \
		.irq_disable_func = cs40l50_disable_func_##inst,                                   \
	};                                                                                         \
                                                                                                   \
	static struct cs40l50_data cs40l50_data_##inst = {                                         \
                                                                                                   \
	};                                                                                         \
                                                                                                   \
	static void cs40l50_config_func_##inst(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), cs40l50_isr,          \
			    DEVICE_DT_INST(inst), 0);                                              \
	};                                                                                         \
                                                                                                   \
	static void cs40l50_enable_func_##inst(void)                                               \
	{                                                                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	};                                                                                         \
                                                                                                   \
	static void cs40l50_disable_func_##inst(void)                                              \
	{                                                                                          \
		irq_disable(DT_INST_IRQN(inst));                                                   \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, cs40l50_pm_action);                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &cs40l50_init, NULL, &cs40l50_data_##inst,                     \
			      &cs40l50_config_##inst, POST_KERNEL, CONFIG_HAPTICS_INIT_PRIORITY,   \
			      &cs40l50_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CS40L50_INIT)
