/*
 * Copyright (c) 2024 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8801_kbd

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/mfd/mfd_ite_it8801.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_kbd_matrix.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_ite_it8801_kbd, CONFIG_INPUT_LOG_LEVEL);

struct it8801_mfd_input_altctrl_cfg {
	/* GPIO control device structure */
	const struct device *gpiocr;
	/* GPIO control pin */
	uint8_t pin;
	/* GPIO function select */
	uint8_t alt_func;
};

struct kbd_it8801_config {
	struct input_kbd_matrix_common_config common;
	/* IT8801 controller dev */
	const struct device *mfd;
	/* KSO alternate configuration */
	const struct it8801_mfd_input_altctrl_cfg *altctrl;
	/* I2C device for the MFD parent */
	const struct i2c_dt_spec i2c_dev;
	int mfdctrl_len;
	uint8_t kso_mapping[DT_INST_PROP(0, col_size)];
	/* Keyboard scan out mode control register */
	uint8_t reg_ksomcr;
	/* Keyboard scan in data register */
	uint8_t reg_ksidr;
	/* Keyboard scan in edge event register */
	uint8_t reg_ksieer;
	/* Keyboard scan in interrupt enable register */
	uint8_t reg_ksiier;
};

struct kbd_it8801_data {
	struct input_kbd_matrix_common_data common;
	struct it8801_mfd_callback it8801_kbd_callback;
};

INPUT_KBD_STRUCT_CHECK(struct kbd_it8801_config, struct kbd_it8801_data);

static void kbd_it8801_drive_column(const struct device *dev, int col)
{
	const struct kbd_it8801_config *config = dev->config;
	int ret;
	uint8_t kso_val;

	if (col == INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE) {
		/* Tri-state all outputs. KSO[22:11, 6:0] output high */
		kso_val = IT8801_REG_MASK_KSOSDIC | IT8801_REG_MASK_AKSOSC;
	} else if (col == INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL) {
		/* Assert all outputs. KSO[22:11, 6:0] output low */
		kso_val = IT8801_REG_MASK_AKSOSC;
	} else {
		/* Selected KSO[22:11, 6:0] output low, all others KSO output high */
		kso_val = config->kso_mapping[col];
	}

	ret = i2c_reg_write_byte_dt(&config->i2c_dev, config->reg_ksomcr, kso_val);
	if (ret != 0) {
		LOG_ERR("Failed to drive column (ret %d)", ret);
		return;
	}
}

static kbd_row_t kbd_it8801_read_row(const struct device *dev)
{
	const struct kbd_it8801_config *const config = dev->config;
	int ret;
	uint8_t value;

	ret = i2c_reg_read_byte_dt(&config->i2c_dev, config->reg_ksidr, &value);
	if (ret != 0) {
		LOG_ERR("Failed to read row (ret %d)", ret);
	}

	/* Bits are active-low, so invert returned levels */
	return (~value) & 0xff;
}

static void it8801_input_alert_handler(const struct device *dev)
{
	const struct kbd_it8801_config *const config = dev->config;
	int ret;
	uint8_t ksieer_val;

	ret = i2c_reg_read_byte_dt(&config->i2c_dev, config->reg_ksieer, &ksieer_val);
	if (ret != 0) {
		LOG_ERR("Failed to read KBD interrupt status (ret %d)", ret);
	}

	if (ksieer_val != 0) {
		/* Clear pending interrupts */
		ret = i2c_reg_write_byte_dt(&config->i2c_dev, config->reg_ksieer, GENMASK(7, 0));
		if (ret != 0) {
			LOG_ERR("Failed to clear pending interrupts (ret %d)", ret);
		}

		input_kbd_matrix_poll_start(dev);
	}
}

static void kbd_it8801_set_detect_mode(const struct device *dev, bool enable)
{
	const struct kbd_it8801_config *const config = dev->config;
	int ret;

	if (enable) {
		/* Clear pending interrupts */
		ret = i2c_reg_write_byte_dt(&config->i2c_dev, config->reg_ksieer, GENMASK(7, 0));
		if (ret != 0) {
			LOG_ERR("Failed to clear pending interrupts (ret %d)", ret);
			return;
		}
		/* Enable KSI falling edge event trigger interrupt */
		ret = i2c_reg_write_byte_dt(&config->i2c_dev, config->reg_ksiier, GENMASK(7, 0));
		if (ret != 0) {
			LOG_ERR("Failed to enable KSI event trigger interrupt (ret %d)", ret);
			return;
		}
	} else {
		/* Disable KSI falling edge event trigger interrupt */
		ret = i2c_reg_write_byte_dt(&config->i2c_dev, config->reg_ksiier, 0x00);
		if (ret != 0) {
			LOG_ERR("Failed to disable KSI event trigger interrupt (ret %d)", ret);
			return;
		}
	}
}

static int kbd_it8801_init(const struct device *dev)
{
	const struct kbd_it8801_config *const config = dev->config;
	struct kbd_it8801_data *data = dev->data;
	int ret, status;

	/* Verify multi-function parent is ready */
	if (!device_is_ready(config->mfd)) {
		LOG_ERR("(input)%s is not ready", config->mfd->name);
		return -ENODEV;
	}

	for (int i = 0; i < config->mfdctrl_len; i++) {
		/* Switching the pin to KSO alternate function (KSO[21:18]) */
		status = mfd_it8801_configure_pins(&config->i2c_dev, config->altctrl[i].gpiocr,
						   config->altctrl[i].pin,
						   config->altctrl[i].alt_func);
		if (status != 0) {
			LOG_ERR("Failed to configure KSO[21:18] pins");
			return status;
		}
	}

	/* Disable wakeup and interrupt of KSI pins before configuring */
	kbd_it8801_set_detect_mode(dev, false);

	/* Start with KEYBOARD_COLUMN_ALL, KSO[22:11, 6:0] output low */
	ret = i2c_reg_write_byte_dt(&config->i2c_dev, config->reg_ksomcr, IT8801_REG_MASK_AKSOSC);
	if (ret != 0) {
		LOG_ERR("Failed to set all KSO output low (ret %d)", ret);
		return ret;
	}
	/* Gather KSI interrupt enable */
	ret = i2c_reg_write_byte_dt(&config->i2c_dev, IT8801_REG_GIECR, IT8801_REG_MASK_GKSIIE);
	if (ret != 0) {
		LOG_ERR("Failed to enable gather KSI interrupt (ret %d)", ret);
		return ret;
	}
	/* Alert response enable */
	ret = i2c_reg_write_byte_dt(&config->i2c_dev, IT8801_REG_SMBCR, IT8801_REG_MASK_ARE);
	if (ret != 0) {
		LOG_ERR("Failed to enable alert response (ret %d)", ret);
		return ret;
	}

	/* Register the interrupt of IT8801 MFD callback function */
	data->it8801_kbd_callback.cb = it8801_input_alert_handler;
	data->it8801_kbd_callback.dev = dev;
	mfd_it8801_register_interrupt_callback(config->mfd, &data->it8801_kbd_callback);

	return input_kbd_matrix_common_init(dev);
}

static const struct input_kbd_matrix_api kbd_it8801_api = {
	.drive_column = kbd_it8801_drive_column,
	.read_row = kbd_it8801_read_row,
	.set_detect_mode = kbd_it8801_set_detect_mode,
};

#define INPUT_IT8801_INIT(inst)                                                                    \
	INPUT_KBD_MATRIX_DT_INST_DEFINE(inst);                                                     \
	PM_DEVICE_DT_INST_DEFINE(inst, input_kbd_matrix_pm_action);                                \
	static const struct it8801_mfd_input_altctrl_cfg                                           \
		it8801_input_altctrl##inst[IT8801_DT_INST_MFDCTRL_LEN(inst)] =                     \
			IT8801_DT_MFD_ITEMS_LIST(inst);                                            \
	static struct kbd_it8801_data kbd_it8801_data_##inst;                                      \
	static const struct kbd_it8801_config kbd_it8801_cfg_##inst = {                            \
		.common = INPUT_KBD_MATRIX_DT_INST_COMMON_CONFIG_INIT(inst, &kbd_it8801_api),      \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
		.i2c_dev = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),                                  \
		.altctrl = it8801_input_altctrl##inst,                                             \
		.mfdctrl_len = IT8801_DT_INST_MFDCTRL_LEN(inst),                                   \
		.kso_mapping = DT_INST_PROP(inst, kso_mapping),                                    \
		.reg_ksomcr = DT_INST_REG_ADDR_BY_IDX(inst, 0),                                    \
		.reg_ksidr = DT_INST_REG_ADDR_BY_IDX(inst, 1),                                     \
		.reg_ksieer = DT_INST_REG_ADDR_BY_IDX(inst, 2),                                    \
		.reg_ksiier = DT_INST_REG_ADDR_BY_IDX(inst, 3),                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &kbd_it8801_init, PM_DEVICE_DT_INST_GET(inst),                 \
			      &kbd_it8801_data_##inst, &kbd_it8801_cfg_##inst, POST_KERNEL,        \
			      CONFIG_MFD_INIT_PRIORITY, NULL);                                     \
	BUILD_ASSERT(IN_RANGE(DT_INST_PROP(inst, row_size), 1, 8), "invalid row-size");            \
	BUILD_ASSERT(IN_RANGE(DT_INST_PROP(inst, col_size), 1, 19), "invalid col-size");

DT_INST_FOREACH_STATUS_OKAY(INPUT_IT8801_INIT)
