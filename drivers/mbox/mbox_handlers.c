/*
 * Copyright (c) 2021 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/syscall_handler.h>
#include <zephyr/drivers/mbox.h>

static inline int z_vrfy_mbox_send(const struct mbox_channel *channel,
				   const struct mbox_msg *msg)
{
	Z_OOPS(Z_SYSCALL_MEMORY_READ(channel, sizeof(struct mbox_channel)));
	Z_OOPS(Z_SYSCALL_DRIVER_MBOX(channel->dev, send));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(msg, sizeof(struct mbox_msg)));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(msg->data, msg->size));

	return z_impl_mbox_send(channel, msg);
}
#include <syscalls/mbox_send_mrsh.c>

static inline int z_vrfy_mbox_mtu_get(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_MBOX(dev, mtu_get));

	return z_impl_mbox_mtu_get(dev);
}
#include <syscalls/mbox_mtu_get_mrsh.c>

static inline uint32_t z_vrfy_mbox_max_channels_get(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_MBOX(dev, max_channels_get));

	return z_impl_mbox_max_channels_get(dev);
}
#include <syscalls/mbox_max_channels_get_mrsh.c>

static inline int z_vrfy_mbox_set_enabled(const struct mbox_channel *channel, bool enable)
{
	Z_OOPS(Z_SYSCALL_MEMORY_READ(channel, sizeof(struct mbox_channel)));
	Z_OOPS(Z_SYSCALL_DRIVER_MBOX(channel->dev, set_enabled));

	return z_impl_mbox_set_enabled(channel, enable);
}
#include <syscalls/mbox_set_enabled_mrsh.c>
