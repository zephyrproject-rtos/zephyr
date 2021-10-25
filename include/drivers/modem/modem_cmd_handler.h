/** @file
 * @brief Modem command handler header file.
 *
 * Text-based command handler implementation for modem context driver.
 */

/*
 * Copyright (c) 2021 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_CMD_HANDLER_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_CMD_HANDLER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MODEM_CMD_DEFINE(name_) \
static int name_(struct modem_cmd_handler_data *data, uint16_t len, \
		 uint8_t **argv, uint16_t argc)

#define MODEM_CMD(cmd_, func_cb_, acount_, adelim_) { \
	.cmd = cmd_, \
	.cmd_len = (uint16_t)sizeof(cmd_)-1, \
	.func = func_cb_, \
	.arg_count_min = acount_, \
	.arg_count_max = acount_, \
	.delim = adelim_, \
	.direct = false, \
}

#define MODEM_CMD_ARGS_MAX(cmd_, func_cb_, acount_, acountmax_, adelim_) { \
	.cmd = cmd_, \
	.cmd_len = (uint16_t)sizeof(cmd_)-1, \
	.func = func_cb_, \
	.arg_count_min = acount_, \
	.arg_count_max = acountmax_, \
	.delim = adelim_, \
	.direct = false, \
}

#define MODEM_CMD_DIRECT_DEFINE(name_) MODEM_CMD_DEFINE(name_)

#define MODEM_CMD_DIRECT(cmd_, func_cb_) { \
	.cmd = cmd_, \
	.cmd_len = (uint16_t)sizeof(cmd_)-1, \
	.func = func_cb_, \
	.arg_count_min = 0, \
	.arg_count_max = 0, \
	.delim = "", \
	.direct = true, \
}

struct modem_cmd_handler_data;

struct modem_cmd {
	int (*func)(struct modem_cmd_handler_data *data, uint16_t len,
		    uint8_t **argv, uint16_t argc);
	const char *cmd;
	const char *delim;
	uint16_t cmd_len;
	uint16_t arg_count_min;
	uint16_t arg_count_max;
	bool direct;
};

#define SETUP_CMD(cmd_send_, match_cmd_, func_cb_, num_param_, delim_) { \
	.send_cmd = cmd_send_, \
	MODEM_CMD(match_cmd_, func_cb_, num_param_, delim_) \
}

#define SETUP_CMD_NOHANDLE(send_cmd_) \
		SETUP_CMD(send_cmd_, NULL, NULL, 0U, NULL)

/* series of modem setup commands to run */
struct setup_cmd {
	const char *send_cmd;
	struct modem_cmd handle_cmd;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_CMD_HANDLER_H_ */
