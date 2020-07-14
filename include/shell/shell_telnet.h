/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_TELNET_H__
#define SHELL_TELNET_H__

#include <shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct shell_transport_api shell_telnet_transport_api;

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

	/** Network context of TELNET client. */
	struct net_context *client_ctx;

	/** RX packet FIFO. */
	struct k_fifo rx_fifo;

	/** The delayed work is used to send non-lf terminated output that has
	 *  been around for "too long". This will prove to be useful
	 *  to send the shell prompt for instance.
	 */
	struct k_delayed_work send_work;

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

#endif /* SHELL_TELNET_H__ */
