/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEST_EDK_H_
#define _TEST_EDK_H_
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

	enum Channels {
		CHAN_TICK = 1,
		CHAN_LAST
	};

	struct channel_tick_data {
		unsigned long l;
	};

	__syscall int publish(enum Channels channel, void *data,
			      size_t data_len);
	__syscall int receive(enum Channels channel, void *data,
			      size_t data_len);
	__syscall int register_subscriber(enum Channels channel,
			      struct k_event *evt);
#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/app_api.h>
#endif /* _TEST_EDK_H_ */
