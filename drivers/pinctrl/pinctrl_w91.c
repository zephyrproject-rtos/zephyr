/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/w91-pinctrl.h>
#include <ipc/ipc_based_driver.h>

/* Driver dts compatibility: telink,w91_pinctrl */
#define DT_DRV_COMPAT telink_w91_pinctrl

enum {
	IPC_DISPATCHER_PINCTRL_PIN_CONFIG = IPC_DISPATCHER_PINCTRL,
};

struct pinctrl_w91_pin_config_req {
	uint8_t pin;
	uint8_t func;
};

static struct ipc_based_driver ipc_data;    /* ipc driver data part */

/* Pinctrl driver initialization */
static int pinctrl_w91_init(void)
{
	ipc_based_driver_init(&ipc_data);

	return 0;
}

/* APIs implementation: pin configure */
static size_t pack_pinctrl_w91_pin_configure(uint8_t inst, void *unpack_data, uint8_t *pack_data)
{
	struct pinctrl_w91_pin_config_req *p_pin_config_req = unpack_data;
	size_t pack_data_len = sizeof(uint32_t) +
		sizeof(p_pin_config_req->pin) + sizeof(p_pin_config_req->func);

	if (pack_data != NULL) {
		uint32_t id = IPC_DISPATCHER_MK_ID(IPC_DISPATCHER_PINCTRL_PIN_CONFIG, inst);

		IPC_DISPATCHER_PACK_FIELD(pack_data, id);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_pin_config_req->pin);
		IPC_DISPATCHER_PACK_FIELD(pack_data, p_pin_config_req->func);
	}

	return pack_data_len;
}

IPC_DISPATCHER_UNPACK_FUNC_ONLY_WITH_ERROR_PARAM(pinctrl_w91_pin_configure);

static int pinctrl_w91_pin_configure(uint8_t pin, uint8_t func)
{
	int err;
	struct pinctrl_w91_pin_config_req pin_config_req;

	pin_config_req.pin = pin;
	pin_config_req.func = func;

	IPC_DISPATCHER_HOST_SEND_DATA(&ipc_data, 0,
			pinctrl_w91_pin_configure, &pin_config_req, &err,
			CONFIG_TELINK_W91_IPC_DISPATCHER_TIMEOUT_MS);

	return err;
}

/* API implementation: configure_pins */
int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	int err = 0;
	uint8_t func;
	uint8_t pin;

	for (uint8_t i = 0; i < pin_cnt; i++) {
		func = W91_PINMUX_GET_FUNC(*pins);
		pin = W91_PINMUX_GET_PIN(*pins);

		err = pinctrl_w91_pin_configure(pin, func);
		if (err < 0) {
			break;
		}

		pins++;
	}

	return err;
}

SYS_INIT(pinctrl_w91_init, POST_KERNEL, CONFIG_TELINK_W91_IPC_PRE_DRIVERS_INIT_PRIORITY);
