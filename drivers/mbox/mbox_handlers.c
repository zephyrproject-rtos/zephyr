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
	K_OOPS(K_SYSCALL_DRIVER_MBOX(dev, send));
	K_OOPS(K_SYSCALL_MEMORY_READ(msg, sizeof(struct mbox_msg)));
	K_OOPS(K_SYSCALL_MEMORY_READ(msg->data, msg->size));

	return z_impl_mbox_send(dev, channel_id, msg);
}
#include <syscalls/mbox_send_mrsh.c>

static inline int z_vrfy_mbox_mtu_get(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_MBOX(dev, mtu_get));

	return z_impl_mbox_mtu_get(dev);
}
#include <syscalls/mbox_mtu_get_mrsh.c>

static inline uint32_t z_vrfy_mbox_max_channels_get(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_MBOX(dev, max_channels_get));

	return z_impl_mbox_max_channels_get(dev);
}
#include <syscalls/mbox_max_channels_get_mrsh.c>

static inline int z_vrfy_mbox_set_enabled(const struct device *dev,
					  mbox_channel_id_t channel_id,
					  bool enabled)
{
	K_OOPS(K_SYSCALL_DRIVER_MBOX(dev, set_enabled));

	return z_impl_mbox_set_enabled(dev, channel_id, enabled);
}
#include <syscalls/mbox_set_enabled_mrsh.c>
