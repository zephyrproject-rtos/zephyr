/*
 * Copyright (c) 2024 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_RPMSG_H__
#define SHELL_RPMSG_H__

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <openamp/rpmsg.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct shell_transport_api shell_rpmsg_transport_api;

/** RPMsg received message placeholder */
struct shell_rpmsg_rx {
	/** Pointer to the data held by RPMsg endpoint */
	void *data;
	/** The length of the data */
	size_t len;
};

/** RPMsg-based shell transport. */
struct shell_rpmsg {
	/** Handler function registered by shell. */
	shell_transport_handler_t shell_handler;

	/** Context registered by shell. */
	void *shell_context;

	/** Indicator if we are ready to read/write */
	bool ready;

	/** Setting for blocking mode */
	bool blocking;

	/** RPMsg endpoint */
	struct rpmsg_endpoint ept;

	/** Queue for received data. */
	struct k_msgq rx_q;

	/** Buffer for received messages */
	struct shell_rpmsg_rx rx_buf[CONFIG_SHELL_RPMSG_MAX_RX];

	/** The current rx message */
	struct shell_rpmsg_rx rx_cur;

	/** The number of bytes consumed from rx_cur */
	size_t rx_consumed;
};

#define SHELL_RPMSG_DEFINE(_name)					\
	static struct shell_rpmsg _name##_shell_rpmsg;			\
	struct shell_transport _name = {				\
		.api = &shell_rpmsg_transport_api,			\
		.ctx = (struct shell_rpmsg *)&_name##_shell_rpmsg,	\
	}

/**
 * @brief Initialize the Shell backend using the provided @p rpmsg_dev device.
 *
 * @param rpmsg_dev A pointer to an RPMsg device
 * @return 0 on success or a negative value on error
 */
int shell_backend_rpmsg_init_transport(struct rpmsg_device *rpmsg_dev);

/**
 * @brief This function provides pointer to shell RPMsg backend instance.
 *
 * Function returns pointer to the shell RPMsg instance. This instance can be
 * next used with shell_execute_cmd function in order to test commands behavior.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_rpmsg_get_ptr(void);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_RPMSG_H__ */
