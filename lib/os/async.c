/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/async.h>

#ifdef CONFIG_POLL
void async_signal_cb(int result, void *sig_data)
{
	struct k_poll_signal *sig = (struct k_poll_signal *)sig_data;

	k_poll_signal_raise(sig, result);
}
#endif /* CONFIG_POLL */
