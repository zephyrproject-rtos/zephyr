/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Async callback used with signal notifier
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#ifdef CONFIG_POLL

void z_spi_transfer_signal_cb(const struct device *dev,
			      int result,
			      void *userdata)
{
	ARG_UNUSED(dev);

	struct k_poll_signal *sig = userdata;

	k_poll_signal_raise(sig, result);
}

#endif /* CONFIG_POLL */
