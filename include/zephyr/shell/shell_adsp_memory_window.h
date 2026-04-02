/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_ADSP_MEMORY_WINDOW_H__
#define SHELL_ADSP_MEMORY_WINDOW_H__

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const struct shell_transport_api shell_adsp_memory_window_transport_api;

struct sys_winstream;

/** Memwindow based shell transport. */
struct shell_adsp_memory_window {
	/** Handler function registered by shell. */
	shell_transport_handler_t shell_handler;

	struct k_timer timer;

	/** Context registered by shell. */
	void *shell_context;

	/** Receive winstream object */
	struct sys_winstream *ws_rx;

	/** Transmit winstream object */
	struct sys_winstream *ws_tx;

	/** Last read sequence number */
	uint32_t read_seqno;
};

#define SHELL_ADSP_MEMORY_WINDOW_DEFINE(_name)				\
	static struct shell_adsp_memory_window _name##_shell_adsp_memory_window;\
	struct shell_transport _name = {					\
		.api = &shell_adsp_memory_window_transport_api,		\
		.ctx = &_name##_shell_adsp_memory_window,	\
	}

/**
 * @brief This function provides pointer to shell ADSP memory window backend instance.
 *
 * Function returns pointer to the shell ADSP memory window instance. This instance can be
 * next used with shell_execute_cmd function in order to test commands behavior.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_adsp_memory_window_get_ptr(void);

#ifdef __cplusplus
}
#endif

#endif /* SHELL_ADSP_MEMORY_WINDOW_H__ */
