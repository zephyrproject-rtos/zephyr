/*
 * Copyright (c) 2022 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/modem/pipe.h>

#ifndef ZEPHYR_MODEM_BACKEND_TTY_
#define ZEPHYR_MODEM_BACKEND_TTY_

#ifdef __cplusplus
extern "C" {
#endif

struct modem_backend_tty {
	const char *tty_path;
	int tty_fd;
	struct modem_pipe pipe;
	struct k_thread thread;
	k_thread_stack_t *stack;
	size_t stack_size;
	atomic_t state;
};

struct modem_backend_tty_config {
	const char *tty_path;
	k_thread_stack_t *stack;
	size_t stack_size;
};

struct modem_pipe *modem_backend_tty_init(struct modem_backend_tty *backend,
					  const struct modem_backend_tty_config *config);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODEM_BACKEND_TTY_ */
