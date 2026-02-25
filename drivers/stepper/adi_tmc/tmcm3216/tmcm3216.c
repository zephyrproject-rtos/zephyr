/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Alexis Czezar C Torreno
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_tmcm3216

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <adi_tmcm_rs485.h>
#include "tmcm3216.h"
#include "tmcm3216_reg.h"

LOG_MODULE_REGISTER(tmcm3216, CONFIG_STEPPER_LOG_LEVEL);

/* Shared bus helpers — called from stepper_ctrl and stepper_driver */

int tmcm3216_sap(const struct device *dev, uint8_t motor, uint8_t parameter, uint32_t value)
{
	const struct tmcm3216_stepper_config *config = dev->config;
	const struct device *parent = config->controller;
	const struct tmcm3216_config *parent_config = parent->config;
	struct tmcm3216_data *parent_data = parent->data;
	struct tmcm_rs485_cmd cmd;
	uint32_t reply;
	int err;

	cmd.module_addr = parent_config->rs485_addr;
	cmd.command_number = TMCL_CMD_SAP;
	cmd.type_number = parameter;
	cmd.bank_number = motor;
	cmd.data = value;

	k_sem_take(&parent_data->sem, K_FOREVER);

	err = tmcm_rs485_send_command(parent_config->rs485, &cmd, &reply, &parent_config->de_gpio);

	k_sem_give(&parent_data->sem);

	if (err < 0) {
		LOG_ERR("SAP failed: motor=%d param=%d value=%d", motor, parameter, value);
		return err;
	}
	return 0;
}

int tmcm3216_gap(const struct device *dev, uint8_t motor, uint8_t parameter, uint32_t *value)
{
	const struct tmcm3216_stepper_config *config = dev->config;
	const struct device *parent = config->controller;
	const struct tmcm3216_config *parent_config = parent->config;
	struct tmcm3216_data *parent_data = parent->data;
	struct tmcm_rs485_cmd cmd;
	int err;

	cmd.module_addr = parent_config->rs485_addr;
	cmd.command_number = TMCL_CMD_GAP;
	cmd.type_number = parameter;
	cmd.bank_number = motor;
	cmd.data = 0; /* Not used for GAP */

	k_sem_take(&parent_data->sem, K_FOREVER);

	err = tmcm_rs485_send_command(parent_config->rs485, &cmd, value, &parent_config->de_gpio);

	k_sem_give(&parent_data->sem);

	if (err < 0) {
		LOG_ERR("GAP failed: motor=%d param=%d", motor, parameter);
		return err;
	}
	return 0;
}

int tmcm3216_send_command(const struct device *dev, struct tmcm_rs485_cmd *cmd, uint32_t *reply)
{
	const struct tmcm3216_stepper_config *config = dev->config;
	const struct device *parent = config->controller;
	const struct tmcm3216_config *parent_config = parent->config;
	struct tmcm3216_data *parent_data = parent->data;
	int err;

	/* Set the correct module address from parent config */
	cmd->module_addr = parent_config->rs485_addr;

	k_sem_take(&parent_data->sem, K_FOREVER);
	err = tmcm_rs485_send_command(parent_config->rs485, cmd, reply, &parent_config->de_gpio);
	k_sem_give(&parent_data->sem);

	if (err < 0) {
		LOG_ERR("RS485 command failed: %d", err);
		return err;
	}

	return 0;
}

/* Initialize TMCM-3216 controller (parent device) */
static int tmcm3216_init(const struct device *dev)
{
	const struct tmcm3216_config *config = dev->config;
	int err;

	/* Verify UART/RS485 device is ready */
	if (!device_is_ready(config->rs485)) {
		LOG_ERR("RS485 UART device not ready");
		return -ENODEV;
	}

	/* Configure DE pin if specified */
	if (config->de_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->de_gpio)) {
			LOG_ERR("DE GPIO device not ready");
			return -ENODEV;
		}

		err = gpio_pin_configure_dt(&config->de_gpio, GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("Failed to configure DE pin: %d", err);
			return err;
		}

		LOG_INF("DE pin configured on %s pin %d", config->de_gpio.port->name,
			config->de_gpio.pin);
	}

	LOG_INF("TMCM-3216 controller initialized (RS485 addr: %d)", config->rs485_addr);

	return 0;
}

#define TMCM3216_DEFINE(inst)                                                                      \
	BUILD_ASSERT(                                                                              \
		DT_INST_CHILD_NUM(inst) <= 6,                                                      \
		"tmcm3216 can drive three steppers at max (6 child nodes: 3 driver + 3 ctrl)");    \
	static struct tmcm3216_data tmcm3216_data_##inst = {                                       \
		.sem = Z_SEM_INITIALIZER(tmcm3216_data_##inst.sem, 1, 1),                          \
	};                                                                                         \
	static const struct tmcm3216_config tmcm3216_config_##inst = {                             \
		.rs485 = DEVICE_DT_GET(DT_INST_BUS(inst)),                                         \
		.rs485_addr = DT_INST_PROP(inst, module_address),                                  \
		.de_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, de_gpios, {0}),                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, tmcm3216_init, NULL, &tmcm3216_data_##inst,                    \
			      &tmcm3216_config_##inst, POST_KERNEL, CONFIG_STEPPER_INIT_PRIORITY,  \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(TMCM3216_DEFINE)
