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

#include <zephyr/kernel.h>

#include "modem_context.h"

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

#define CMD_RESP	0
#define CMD_UNSOL	1
#define CMD_HANDLER	2
#define CMD_MAX		3

/*
 * Flags for modem_send_cmd_ext.
 */
#define MODEM_NO_TX_LOCK	BIT(0)
#define MODEM_NO_SET_CMDS	BIT(1)
#define MODEM_NO_UNSET_CMDS	BIT(2)

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

struct modem_cmd_handler_data {
	const struct modem_cmd *cmds[CMD_MAX];
	size_t cmds_len[CMD_MAX];

	char *match_buf;
	size_t match_buf_len;

	int last_error;

	const char *eol;
	size_t eol_len;

	/* rx net buffer */
	struct net_buf *rx_buf;

	/* allocation info */
	struct net_buf_pool *buf_pool;
	k_timeout_t alloc_timeout;

	/* locks */
	struct k_sem sem_tx_lock;
	struct k_sem sem_parse_lock;

	/* user data */
	void *user_data;
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
				  const struct modem_cmd *handler_cmds,
				  size_t handler_cmds_len,
				  bool reset_error_flag);

/**
 * @brief  send AT command to interface with behavior defined by flags
 *
 * This function is similar to @ref modem_cmd_send, but it allows to choose a
 * specific behavior regarding acquiring tx_lock, setting and unsetting
 * @a handler_cmds.
 *
 * @param  *iface: interface to use
 * @param  *handler: command handler to use
 * @param  *handler_cmds: commands to attach
 * @param  handler_cmds_len: size of commands array
 * @param  *buf: NULL terminated send buffer
 * @param  *sem: wait for response semaphore
 * @param  timeout: timeout of command
 * @param  flags: flags which influence behavior of command sending
 *
 * @retval 0 if ok, < 0 if error.
 */
int modem_cmd_send_ext(struct modem_iface *iface,
		       struct modem_cmd_handler *handler,
		       const struct modem_cmd *handler_cmds,
		       size_t handler_cmds_len, const uint8_t *buf,
		       struct k_sem *sem, k_timeout_t timeout, int flags);

/**
 * @brief  send AT command to interface w/o locking TX
 *
 * @param  *iface: interface to use
 * @param  *handler: command handler to use
 * @param  *handler_cmds: commands to attach
 * @param  handler_cmds_len: size of commands array
 * @param  *buf: NULL terminated send buffer
 * @param  *sem: wait for response semaphore
 * @param  timeout: timeout of command
 *
 * @retval 0 if ok, < 0 if error.
 */
static inline int modem_cmd_send_nolock(struct modem_iface *iface,
					struct modem_cmd_handler *handler,
					const struct modem_cmd *handler_cmds,
					size_t handler_cmds_len,
					const uint8_t *buf, struct k_sem *sem,
					k_timeout_t timeout)
{
	return modem_cmd_send_ext(iface, handler, handler_cmds,
				  handler_cmds_len, buf, sem, timeout,
				  MODEM_NO_TX_LOCK);
}

/**
 * @brief  send AT command to interface w/ a TX lock
 *
 * @param  *iface: interface to use
 * @param  *handler: command handler to use
 * @param  *handler_cmds: commands to attach
 * @param  handler_cmds_len: size of commands array
 * @param  *buf: NULL terminated send buffer
 * @param  *sem: wait for response semaphore
 * @param  timeout: timeout of command
 *
 * @retval 0 if ok, < 0 if error.
 */
static inline int modem_cmd_send(struct modem_iface *iface,
				 struct modem_cmd_handler *handler,
				 const struct modem_cmd *handler_cmds,
				 size_t handler_cmds_len, const uint8_t *buf,
				 struct k_sem *sem, k_timeout_t timeout)
{
	return modem_cmd_send_ext(iface, handler, handler_cmds,
				  handler_cmds_len, buf, sem, timeout, 0);
}

/**
 * @brief  send a series of AT commands w/ a TX lock
 *
 * @param  *iface: interface to use
 * @param  *handler: command handler to use
 * @param  *cmds: array of setup commands to send
 * @param  cmds_len: size of the setup command array
 * @param  *sem: wait for response semaphore
 * @param  timeout: timeout of command
 *
 * @retval 0 if ok, < 0 if error.
 */
int modem_cmd_handler_setup_cmds(struct modem_iface *iface,
				 struct modem_cmd_handler *handler,
				 const struct setup_cmd *cmds, size_t cmds_len,
				 struct k_sem *sem, k_timeout_t timeout);

/**
 * @brief  send a series of AT commands w/o locking TX
 *
 * @param  *iface: interface to use
 * @param  *handler: command handler to use
 * @param  *cmds: array of setup commands to send
 * @param  cmds_len: size of the setup command array
 * @param  *sem: wait for response semaphore
 * @param  timeout: timeout of command
 *
 * @retval 0 if ok, < 0 if error.
 */
int modem_cmd_handler_setup_cmds_nolock(struct modem_iface *iface,
					struct modem_cmd_handler *handler,
					const struct setup_cmd *cmds,
					size_t cmds_len, struct k_sem *sem,
					k_timeout_t timeout);

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

/**
 * @brief  Lock the modem for sending cmds
 *
 * This is semaphore-based rather than mutex based, which means there's no
 * requirements of thread ownership for the user. This function is useful
 * when one needs to prevent threads from sending UART data to the modem for an
 * extended period of time (for example during modem reset).
 *
 * @param  *handler: command handler to lock
 * @param  timeout: give up after timeout
 *
 * @retval 0 if ok, < 0 if error.
 */
int modem_cmd_handler_tx_lock(struct modem_cmd_handler *handler,
			      k_timeout_t timeout);

/**
 * @brief  Unlock the modem for sending cmds
 *
 * @param  *handler: command handler to unlock
 */
void modem_cmd_handler_tx_unlock(struct modem_cmd_handler *handler);


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MODEM_MODEM_CMD_HANDLER_H_ */
