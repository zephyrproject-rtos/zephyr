/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/async.h>

#ifdef CONFIG_POLL
void async_signal_cb(struct async_callee *callee_data, int result, void *signal)
{
	ARG_UNUSED(callee_data);

	struct k_poll_signal *sig = (struct k_poll_signal *)signal;

	k_poll_signal_raise(sig, result);
}

void async_signal_with_data_cb(struct async_callee *callee_data, int result, void *signal)
{
	struct async_poll_signal *async_signal = (struct async_poll_signal *)signal;

	async_signal->callee_data = *(void **)callee_data;
	async_signal_cb(callee_data, result, &async_signal->signal);
}

#endif /* CONFIG_POLL */
