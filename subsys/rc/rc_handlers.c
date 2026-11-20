/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/rc.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/rc/rc.h>

static inline int z_vrfy_rc_get_caps(const struct device *dev, struct rc_caps *caps)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_RC));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(caps, sizeof(*caps)));
	return z_impl_rc_get_caps(dev, caps);
}
#include <zephyr/syscalls/rc_get_caps_mrsh.c>

static inline int z_vrfy_rc_set_rx_timeout(const struct device *dev, uint32_t us)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_RC));
	return z_impl_rc_set_rx_timeout(dev, us);
}
#include <zephyr/syscalls/rc_set_rx_timeout_mrsh.c>

static inline int z_vrfy_rc_protocol_enable(const struct device *dev, uint16_t proto)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_RC));
	return z_impl_rc_protocol_enable(dev, proto);
}
#include <zephyr/syscalls/rc_protocol_enable_mrsh.c>

static inline int z_vrfy_rc_protocol_disable(const struct device *dev, uint16_t proto)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_RC));
	return z_impl_rc_protocol_disable(dev, proto);
}
#include <zephyr/syscalls/rc_protocol_disable_mrsh.c>

static inline int z_vrfy_rc_protocol_is_enabled(const struct device *dev, uint16_t proto)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_RC));
	return z_impl_rc_protocol_is_enabled(dev, proto);
}
#include <zephyr/syscalls/rc_protocol_is_enabled_mrsh.c>
