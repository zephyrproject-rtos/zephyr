/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/lin.h>

static inline int z_vrfy_lin_start(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_LIN(dev, start));

	return z_impl_lin_start(dev);
}
#include <zephyr/syscalls/lin_start_mrsh.c>

static inline int z_vrfy_lin_stop(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_LIN(dev, stop));

	return z_impl_lin_stop(dev);
}
#include <zephyr/syscalls/lin_stop_mrsh.c>

static inline int z_vrfy_lin_configure(const struct device *dev, const struct lin_config *config)
{
	K_OOPS(K_SYSCALL_DRIVER_LIN(dev, configure));
	K_OOPS(K_SYSCALL_MEMORY_READ(config, sizeof(struct lin_config)));

	return z_impl_lin_configure(dev, config);
}
#include <zephyr/syscalls/lin_configure_mrsh.c>

static inline int z_vrfy_lin_get_config(const struct device *dev, struct lin_config *config)
{
	K_OOPS(K_SYSCALL_DRIVER_LIN(dev, get_config));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(config, sizeof(struct lin_config)));

	return z_impl_lin_get_config(dev, config);
}
#include <zephyr/syscalls/lin_get_config_mrsh.c>

static inline int z_vrfy_lin_send(const struct device *dev, const struct lin_msg *msg,
				  k_timeout_t timeout)
{
	struct lin_msg msg_copy;

	K_OOPS(K_SYSCALL_DRIVER_LIN(dev, send));
	K_OOPS(k_usermode_from_copy(&msg_copy, msg, sizeof(msg_copy)));

	return z_impl_lin_send(dev, &msg_copy, timeout);
}
#include <zephyr/syscalls/lin_send_mrsh.c>

static inline int z_vrfy_lin_receive(const struct device *dev, struct lin_msg *msg,
				     k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_DRIVER_LIN(dev, receive));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(msg, sizeof(struct lin_msg)));

	return z_impl_lin_receive(dev, msg, timeout);
}
#include <zephyr/syscalls/lin_receive_mrsh.c>

static inline int z_vrfy_lin_response(const struct device *dev, const struct lin_msg *msg,
				      k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_DRIVER_LIN(dev, response));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(msg, sizeof(struct lin_msg)));

	return z_impl_lin_response(dev, msg, timeout);
}
#include <zephyr/syscalls/lin_response_mrsh.c>

static inline int z_vrfy_lin_read(const struct device *dev, struct lin_msg *msg,
				  k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_DRIVER_LIN(dev, read));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(msg, sizeof(struct lin_msg)));

	return z_impl_lin_read(dev, msg, timeout);
}
#include <zephyr/syscalls/lin_read_mrsh.c>

static inline int z_vrfy_lin_wakeup_send(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_LIN(dev, wakeup_send));

	return z_impl_lin_wakeup_send(dev);
}
#include <zephyr/syscalls/lin_wakeup_send_mrsh.c>

static inline int z_vrfy_lin_enter_sleep(const struct device *dev, bool enable)
{
	K_OOPS(K_SYSCALL_DRIVER_LIN(dev, enter_sleep));

	return z_impl_lin_enter_sleep(dev, enable);
}
#include <zephyr/syscalls/lin_enter_sleep_mrsh.c>

static inline int z_vrfy_lin_set_callback(const struct device *dev, lin_event_callback_t callback,
					  void *user_data)
{
	K_OOPS(K_SYSCALL_DRIVER_LIN(dev, set_callback));
	K_OOPS(K_SYSCALL_VERIFY_MSG(callback == NULL,
				    "Application code may not register LIN callbacks"));

	return z_impl_lin_set_callback(dev, callback, user_data);
}
#include <zephyr/syscalls/lin_set_callback_mrsh.c>

static inline int z_vrfy_lin_set_rx_filter(const struct device *dev,
					   const struct lin_filter *filter, size_t filter_count)
{
	struct lin_filter filter_copy;

	K_OOPS(K_SYSCALL_DRIVER_LIN(dev, set_rx_filter));
	K_OOPS(k_usermode_from_copy(&filter_copy, filter, sizeof(filter_copy)));

	/* No direct copying of callbacks */
	filter_copy.callback = NULL;
	filter_copy.user_data = NULL;

	return z_impl_lin_set_rx_filter(dev, &filter_copy, filter_count);
}
#include <zephyr/syscalls/lin_set_rx_filter_mrsh.c>
