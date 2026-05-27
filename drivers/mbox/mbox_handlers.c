/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/mbox.h>

static inline int z_vrfy_mbox_send(const struct device *dev,
				   mbox_channel_id_t channel_id,
				   const struct mbox_msg *msg)
{
	struct mbox_msg msg_copy;

	K_OOPS(K_SYSCALL_DRIVER_MBOX(dev, send));

	if (msg == NULL) {
		/* Signalling mode: NULL msg is valid per the API contract */
		return z_impl_mbox_send(dev, channel_id, NULL);
	}

	/* Copy the userspace struct into a kernel-stack snapshot before
	 * validating the nested data pointer, preventing a TOCTOU race where
	 * a concurrent thread could replace msg->data with a supervisor address
	 * after the access check.
	 */
	K_OOPS(k_usermode_from_copy(&msg_copy, msg, sizeof(msg_copy)));
	K_OOPS(K_SYSCALL_MEMORY_READ(msg_copy.data, msg_copy.size));

	return z_impl_mbox_send(dev, channel_id, &msg_copy);
}
#include <zephyr/syscalls/mbox_send_mrsh.c>

static inline int z_vrfy_mbox_mtu_get(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_MBOX(dev, mtu_get));

	return z_impl_mbox_mtu_get(dev);
}
#include <zephyr/syscalls/mbox_mtu_get_mrsh.c>

static inline uint32_t z_vrfy_mbox_max_channels_get(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_MBOX(dev, max_channels_get));

	return z_impl_mbox_max_channels_get(dev);
}
#include <zephyr/syscalls/mbox_max_channels_get_mrsh.c>

static inline int z_vrfy_mbox_set_enabled(const struct device *dev,
					  mbox_channel_id_t channel_id,
					  bool enabled)
{
	K_OOPS(K_SYSCALL_DRIVER_MBOX(dev, set_enabled));

	return z_impl_mbox_set_enabled(dev, channel_id, enabled);
}
#include <zephyr/syscalls/mbox_set_enabled_mrsh.c>
