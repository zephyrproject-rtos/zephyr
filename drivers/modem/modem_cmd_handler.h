/** @file
 * @brief Modem command handler header file.
 *
 * Text-based command handler implementation for modem context driver.
 */

/*
 * Copyright (c) 2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_CMD_HANDLER_H_
#define ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_CMD_HANDLER_H_

#include <kernel.h>

#include "modem_context.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MODEM_CMD_DEFINE(name_) \
static void name_(struct modem_cmd_handler_data *data, u16_t len, \
		  u8_t **argv, u16_t argc)

#define MODEM_CMD(cmd_, func_cb_, acount_, adelim_) { \
	.cmd = cmd_, \
	.cmd_len = (u16_t)sizeof(cmd_)-1, \
	.func = func_cb_, \
	.arg_count = acount_, \
	.delim = adelim_, \
}

#define CMD_RESP	0
#define CMD_UNSOL	1
#define CMD_HANDLER	2
#define CMD_MAX		3

struct modem_cmd_handler_data;

struct modem_cmd {
	void (*func)(struct modem_cmd_handler_data *data, u16_t len,
		     u8_t **argv, u16_t argc);
	const char *cmd;
	const char *delim;
	u16_t cmd_len;
	u16_t arg_count;
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

struct modem_cmd_handler_data {
	struct modem_cmd *cmds[CMD_MAX];
	size_t cmds_len[CMD_MAX];

	char *read_buf;
	size_t read_buf_len;

	char *match_buf;
	size_t match_buf_len;

	int last_error;

	/* rx net buffer */
	struct net_buf *rx_buf;

	/* allocation info */
	struct net_buf_pool *buf_pool;
	s32_t alloc_timeout;

	/* locks */
	struct k_sem sem_tx_lock;
	struct k_sem sem_parse_lock;
};

/**
 * @brief  get the last error code
 *
 * @param  *data: command handler data reference
 *
 * @retval last handled error.
 */
int modem_cmd_handler_get_error(struct modem_cmd_handler_data *data);

/**
 * @brief  set the last error code
 *
 * @param  *data: command handler data reference
 * @param  *error_code: error
 *
 * @retval 0 if ok, < 0 if error.
 */
int modem_cmd_handler_set_error(struct modem_cmd_handler_data *data,
				int error_code);

/**
 * @brief  update the parser's handler commands
 *
 * @param  *data: handler data to use
 * @param  *handler_cmds: commands to attach
 * @param  handler_cmds_len: size of commands array
 * @param  reset_error_flag: reset last error code
 *
 * @retval 0 if ok, < 0 if error.
 */
int modem_cmd_handler_update_cmds(struct modem_cmd_handler_data *data,
				  struct modem_cmd *handler_cmds,
				  size_t handler_cmds_len,
				  bool reset_error_flag);

/**
 * @brief  send AT command to interface w/o locking TX
 *
 * @param  *iface: interface to use
 * @param  *handler: command handler to use
 * @param  *buf: NULL terminated send buffer
 * @param  *sem: wait for response semaphore
 * @param  timeout: timeout of command (in ms)
 *
 * @retval 0 if ok, < 0 if error.
 */
int modem_cmd_send_nolock(struct modem_iface *iface,
			  struct modem_cmd_handler *handler,
			  struct modem_cmd *handler_cmds,
			  size_t handler_cmds_len,
			  const u8_t *buf, struct k_sem *sem, int timeout);

/**
 * @brief  send AT command to interface w/ a TX lock
 *
 * @param  *iface: interface to use
 * @param  *handler: command handler to use
 * @param  *buf: NULL terminated send buffer
 * @param  *sem: wait for response semaphore
 * @param  timeout: timeout of command (in ms)
 *
 * @retval 0 if ok, < 0 if error.
 */
int modem_cmd_send(struct modem_iface *iface,
		   struct modem_cmd_handler *handler,
		   struct modem_cmd *handler_cmds, size_t handler_cmds_len,
		   const u8_t *buf, struct k_sem *sem, int timeout);

/**
 * @brief  send a series of AT commands
 *
 * @param  *iface: interface to use
 * @param  *handler: command handler to use
 * @param  *cmds: array of setup commands to send
 * @param  cmds_len: size of the setup command array
 * @param  *sem: wait for response semaphore
 * @param  timeout: timeout of command (in ms)
 *
 * @retval 0 if ok, < 0 if error.
 */
int modem_cmd_handler_setup_cmds(struct modem_iface *iface,
				 struct modem_cmd_handler *handler,
				 struct setup_cmd *cmds, size_t cmds_len,
				 struct k_sem *sem, int timeout);

/**
 * @brief  Init command handler
 *
 * @param  *handler: command handler to initialize
 * @param  *data: command handler data to use
 *
 * @retval 0 if ok, < 0 if error.
 */
int modem_cmd_handler_init(struct modem_cmd_handler *handler,
			   struct modem_cmd_handler_data *data);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_CMD_HANDLER_H_ */
