/*
 * Copyright (c) 2018 Makaio GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SHELL_RTT_H_
#define ZEPHYR_INCLUDE_SHELL_RTT_H_

#include <zephyr/shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct shell_transport_api shell_rtt_transport_api;

struct shell_rtt {
	shell_transport_handler_t handler;
	struct k_timer timer;
	void *context;
};

#define SHELL_RTT_DEFINE(_name)					\
	static struct shell_rtt _name##_shell_rtt;			\
	struct shell_transport _name = {				\
		.api = &shell_rtt_transport_api,			\
		.ctx = (struct shell_rtt *)&_name##_shell_rtt		\
	}

/**
 * @brief Function provides pointer to shell rtt backend instance.
 *
 * Function returns pointer to the shell rtt instance. This instance can be
 * next used with shell_execute_cmd function in order to test commands behavior.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_rtt_get_ptr(void);
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHELL_RTT_H_ */
