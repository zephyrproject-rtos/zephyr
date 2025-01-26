/*
 * Copyright (c) 2024-2025 Freedom Veiculos Eletricos
 * Copyright (c) 2024-2025 O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Morse Code shell commands.
 */

#include <zephyr/shell/shell.h>
#include <zephyr/morse/morse.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(morse_shell, CONFIG_MORSE_LOG_LEVEL);

struct args_index {
	uint8_t device;
	uint8_t text;
	uint8_t speed;
	uint8_t period;
};

static const struct args_index args_indx = {
	.device = 1,
	.text = 2,
	.speed = 2,
	.period = 3,
};

static void morse_shell_tx_cb_handler(void *ctx, int status)
{
	LOG_INF("Tx status: %d", status);
}

static void morse_shell_rx_cb_handler(void *ctx, enum morse_rx_state status, uint32_t data)
{
	LOG_INF("Rx 0x%02x, '%c'", data, data);

	if (status == MORSE_RX_STATE_END_TRANSMISSION) {
		LOG_INF("End Rx");
		return;
	}

	if (status == MORSE_RX_STATE_END_WORD) {
		LOG_INF("");
	}
}

static int cmd_config(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *morse_code;
	uint16_t speed;

	morse_code = device_get_binding(argv[args_indx.device]);
	if (!morse_code) {
		shell_error(sh, "Morse Device device not found");
		return -EINVAL;
	}

	speed = strtoul(argv[args_indx.speed], NULL, 0);

	return morse_set_config(morse_code, speed);
}

static int cmd_send(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *morse_code;

	morse_code = device_get_binding(argv[args_indx.device]);
	if (!morse_code) {
		shell_error(sh, "Morse Device device not found");
		return -EINVAL;
	}

	if (morse_send(morse_code, NULL, 0)) {
		shell_error(sh, "Device is busy");
		return -EAGAIN;
	}

	morse_manage_callbacks(morse_code, morse_shell_tx_cb_handler,
			       morse_shell_rx_cb_handler, NULL);
	morse_send(morse_code, argv[args_indx.text], strlen(argv[args_indx.text]));

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(morse_cmds,
	SHELL_CMD_ARG(config, NULL, "<device> <speed>", cmd_config, 3, 0),
	SHELL_CMD_ARG(send, NULL, "<device> <text>", cmd_send, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(morse, &morse_cmds, "Morse code shell commands", NULL);
