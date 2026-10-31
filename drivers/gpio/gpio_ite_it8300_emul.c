/*
 * Copyright (c) 2026 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8300

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/gpio/gpio_ite_it8300_emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(ite_it8300, CONFIG_GPIO_LOG_LEVEL);

#define IT8300_REG_GISR       0x00U
#define IT8300_REG_PDRG_FIRST 0x01U
#define IT8300_REG_PDRG_LAST  0x05U
#define IT8300_REG_ECA        0x30U
#define IT8300_REG_EAE        0x31U
#define IT8300_REG_PDMR_FIRST 0x29U
#define IT8300_REG_LAST       0x3dU

#define IT8300_ECA_CLEAR_ALERT 0x5aU
#define IT8300_EAE_AE          BIT(0)

static const uint8_t it8300_isr_regs[] = {0x06U, 0x07U, 0x28U, 0x2eU, 0x2fU};

struct it8300_emul_config {
	struct gpio_dt_spec alert_gpio;
};

struct it8300_emul_data {
	uint8_t regs[IT8300_REG_LAST + 1U];
};

void it8300_emul_reset(const struct emul *target)
{
	struct it8300_emul_data *data = target->data;
	const struct it8300_emul_config *cfg = target->cfg;

	memset(data->regs, 0, sizeof(data->regs));
	if (cfg->alert_gpio.port != NULL && device_is_ready(cfg->alert_gpio.port)) {
		gpio_emul_input_set_dt(&cfg->alert_gpio, 1);
	}
}

static int it8300_emul_set_alert(const struct emul *target, int value)
{
	const struct it8300_emul_config *cfg = target->cfg;

	if (cfg->alert_gpio.port == NULL) {
		return 0;
	}

	return gpio_emul_input_set_dt(&cfg->alert_gpio, value);
}

static void it8300_emul_update_gisr(struct it8300_emul_data *data)
{
	uint8_t gisr = 0U;

	for (size_t group = 0; group < ARRAY_SIZE(it8300_isr_regs); group++) {
		if (data->regs[it8300_isr_regs[group]] != 0U) {
			gisr |= BIT(group);
		}
	}

	data->regs[IT8300_REG_GISR] = gisr;
}

int it8300_emul_set_input(const struct emul *target, uint8_t group, uint8_t value)
{
	struct it8300_emul_data *data = target->data;
	uint8_t changed;
	uint8_t pdmr;

	if (group >= ARRAY_SIZE(it8300_isr_regs)) {
		return -EINVAL;
	}

	pdmr = IT8300_REG_PDMR_FIRST + group;
	changed = data->regs[pdmr] ^ value;
	data->regs[pdmr] = value;
	if (changed == 0U) {
		return 0;
	}

	data->regs[it8300_isr_regs[group]] |= changed;
	it8300_emul_update_gisr(data);

	if ((data->regs[IT8300_REG_EAE] & IT8300_EAE_AE) != 0U) {
		return it8300_emul_set_alert(target, 0);
	}

	return 0;
}

int it8300_emul_set_reg(const struct emul *target, uint8_t reg, uint8_t value)
{
	struct it8300_emul_data *data = target->data;

	if (reg > IT8300_REG_LAST) {
		return -EINVAL;
	}

	data->regs[reg] = value;
	return 0;
}

int it8300_emul_get_reg(const struct emul *target, uint8_t reg, uint8_t *value)
{
	struct it8300_emul_data *data = target->data;

	if (reg > IT8300_REG_LAST || value == NULL) {
		return -EINVAL;
	}

	*value = data->regs[reg];
	return 0;
}

static int it8300_emul_write_reg(const struct emul *target, uint8_t reg, uint8_t value)
{
	struct it8300_emul_data *data = target->data;

	for (size_t group = 0; group < ARRAY_SIZE(it8300_isr_regs); group++) {
		if (reg == it8300_isr_regs[group]) {
			/* GPIO interrupt status registers are write-1-to-clear. */
			data->regs[reg] &= ~value;
			it8300_emul_update_gisr(data);
			return 0;
		}
	}

	data->regs[reg] = value;

	/* Output pins are reflected in the corresponding pin data mirror register. */
	if (reg >= IT8300_REG_PDRG_FIRST && reg <= IT8300_REG_PDRG_LAST) {
		data->regs[IT8300_REG_PDMR_FIRST + reg - IT8300_REG_PDRG_FIRST] = value;
	}

	if (reg == IT8300_REG_ECA && value == IT8300_ECA_CLEAR_ALERT) {
		/* A newly latched interrupt keeps ALERT# asserted. */
		return it8300_emul_set_alert(target, data->regs[IT8300_REG_GISR] == 0U ? 1 : 0);
	}

	if (reg == IT8300_REG_EAE && (value & IT8300_EAE_AE) != 0U &&
	    data->regs[IT8300_REG_GISR] != 0U) {
		return it8300_emul_set_alert(target, 0);
	}

	return 0;
}

static int it8300_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				int addr)
{
	struct it8300_emul_data *data = target->data;
	uint8_t reg;

	ARG_UNUSED(addr);

	if (num_msgs < 1 || msgs[0].len < 1 || (msgs[0].flags & I2C_MSG_READ) != 0U) {
		return -EIO;
	}

	reg = msgs[0].buf[0];
	if (reg > IT8300_REG_LAST) {
		return -EIO;
	}

	/* A single message writes one or more registers. */
	if (num_msgs == 1) {
		if (msgs[0].len < 2) {
			return -EIO;
		}

		/* Validate each register before writing it. */
		for (size_t i = 1; i < msgs[0].len; i++) {
			size_t offset = (size_t)reg + i - 1;
			int ret;

			if (offset >= ARRAY_SIZE(data->regs)) {
				return -EIO;
			}

			ret = it8300_emul_write_reg(target, (uint8_t)offset, msgs[0].buf[i]);
			if (ret < 0) {
				return ret;
			}
		}

		return 0;
	}

	/* Two messages select a register and then read one or more registers. */
	if (num_msgs == 2) {
		if (msgs[0].len != 1 || (msgs[1].flags & I2C_MSG_READ) == 0U) {
			return -EIO;
		}

		/* Validate each register before reading it. */
		for (size_t i = 0; i < msgs[1].len; i++) {
			size_t offset = (size_t)reg + i;

			if (offset >= ARRAY_SIZE(data->regs)) {
				return -EIO;
			}

			msgs[1].buf[i] = data->regs[offset];
		}

		return 0;
	}

	return -EIO;
}

static int it8300_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);
	return 0;
}

static const struct i2c_emul_api it8300_emul_i2c_api = {
	.transfer = it8300_emul_transfer,
};

#define IT8300_EMUL_DEFINE(inst)                                                                   \
	static struct it8300_emul_data it8300_emul_data_##inst;                                    \
	static const struct it8300_emul_config it8300_emul_config_##inst = {                       \
		.alert_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, alert_gpios, {0}),                    \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(inst, it8300_emul_init, &it8300_emul_data_##inst,                      \
			    &it8300_emul_config_##inst, &it8300_emul_i2c_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(IT8300_EMUL_DEFINE)
