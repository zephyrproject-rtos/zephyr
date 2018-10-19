/*
 * Copyright (c) 2018 Makaio GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_RTT_H__
#define SHELL_RTT_H__

#include <shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct shell_transport_api shell_rtt_transport_api;

struct shell_rtt {
	struct device *dev;
	shell_transport_handler_t handler;
	struct k_timer timer;
	void *context;
	u8_t rx[5];
	size_t rx_cnt;
};

#define SHELL_RTT_DEFINE(_name)					\
	static struct shell_rtt _name##_shell_rtt;			\
	struct shell_transport _name = {				\
		.api = &shell_rtt_transport_api,			\
		.ctx = (struct shell_rtt *)&_name##_shell_rtt		\
	}

#ifdef __cplusplus
}
#endif

#endif /* SHELL_RTT_H__ */
