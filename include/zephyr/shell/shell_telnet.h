/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SHELL_TELNET_H_
#define ZEPHYR_INCLUDE_SHELL_TELNET_H_

#include <zephyr/net/socket.h>
#include <zephyr/shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct shell_transport_api shell_telnet_transport_api;

#define SHELL_TELNET_POLLFD_COUNT 3
#define SHELL_TELNET_MAX_CMD_SIZE 3

/** Line buffer structure. */
struct shell_telnet_line_buf {
	/** Line buffer. */
	char buf[CONFIG_SHELL_TELNET_LINE_BUF_SIZE];

	/** Current line length. */
	uint16_t len;
};

/** TELNET-based shell transport. */
struct shell_telnet {
	/** Handler function registered by shell. */
	shell_transport_handler_t shell_handler;

	/** Context registered by shell. */
	void *shell_context;

	/** Buffer for outgoing line. */
	struct shell_telnet_line_buf line_out;

	/** Array for sockets used by the telnet service. */
	struct zsock_pollfd fds[SHELL_TELNET_POLLFD_COUNT];

	/** Input buffer. */
	uint8_t rx_buf[CONFIG_SHELL_CMD_BUFF_SIZE];

	/** Number of data bytes within the input buffer. */
	size_t rx_len;

	/** Mutex protecting the input buffer access. */
	struct k_mutex rx_lock;
	uint8_t cmd_buf[SHELL_TELNET_MAX_CMD_SIZE];
	uint8_t cmd_len;

	/** The delayed work is used to send non-lf terminated output that has
	 *  been around for "too long". This will prove to be useful
	 *  to send the shell prompt for instance.
	 */
	struct k_work_delayable send_work;
	struct k_work_sync work_sync;

	/** If set, no output is sent to the TELNET client. */
	bool output_lock;
};

#define SHELL_TELNET_DEFINE(_name)					\
	static struct shell_telnet _name##_shell_telnet;		\
	struct shell_transport _name = {				\
		.api = &shell_telnet_transport_api,			\
		.ctx = (struct shell_telnet *)&_name##_shell_telnet	\
	}

/**
 * @brief This function provides pointer to shell telnet backend instance.
 *
 * Function returns pointer to the shell telnet instance. This instance can be
 * next used with shell_execute_cmd function in order to test commands behavior.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_telnet_get_ptr(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHELL_TELNET_H_ */
