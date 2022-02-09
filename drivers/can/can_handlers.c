/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syscall_handler.h>
#include <drivers/can.h>

static inline int z_vrfy_can_set_timing(const struct device *dev,
					const struct can_timing *timing,
					const struct can_timing *timing_data)
{
	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, set_timing));

	return z_impl_can_set_timing((const struct device *)dev,
				     (const struct can_timing *)timing,
				     (const struct can_timing *)timing_data);
}
#include <syscalls/can_set_timing_mrsh.c>

static inline int z_vrfy_can_get_core_clock(const struct device *dev,
					    uint32_t *rate)
{

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, get_core_clock));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(rate, sizeof(rate)));

	return z_impl_can_get_core_clock(dev, rate);
}
#include <syscalls/can_get_core_clock_mrsh.c>

static inline int z_vrfy_can_send(const struct device *dev,
				  const struct zcan_frame *frame,
				  k_timeout_t timeout,
				  can_tx_callback_t callback,
				  void *user_data)
{
	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, send));

	Z_OOPS(Z_SYSCALL_MEMORY_READ((const struct zcan_frame *)frame,
				      sizeof(struct zcan_frame)));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(((struct zcan_frame *)frame)->data,
				     sizeof((struct zcan_frame *)frame)->data));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(callback == 0,
				    "callbacks may not be set from user mode"));

	Z_OOPS(Z_SYSCALL_MEMORY_READ((void *)user_data, sizeof(void *)));

	return z_impl_can_send((const struct device *)dev,
			       (const struct zcan_frame *)frame,
			       (k_timeout_t)timeout,
			       (can_tx_callback_t) callback,
			       (void *)user_data);
}
#include <syscalls/can_send_mrsh.c>

static inline int z_vrfy_can_add_rx_filter_msgq(const struct device *dev,
						struct k_msgq *msgq,
						const struct zcan_filter *filter)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	Z_OOPS(Z_SYSCALL_MEMORY_READ((struct zcan_filter *)filter,
				     sizeof(struct zcan_filter)));
	Z_OOPS(Z_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));

	return z_impl_can_add_rx_filter_msgq((const struct device *)dev,
					     (struct k_msgq *)msgq,
					     (const struct zcan_filter *)filter);
}
#include <syscalls/can_add_rx_filter_msgq_mrsh.c>

static inline void z_vrfy_can_remove_rx_filter(const struct device *dev, int filter_id)
{

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, remove_rx_filter));

	z_impl_can_remove_rx_filter((const struct device *)dev, (int)filter_id);
}
#include <syscalls/can_remove_rx_filter_mrsh.c>

static inline
int z_vrfy_can_get_state(const struct device *dev, enum can_state *state,
			 struct can_bus_err_cnt *err_cnt)
{

	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	if (state != NULL) {
		Z_OOPS(Z_SYSCALL_MEMORY_WRITE(state, sizeof(enum can_state)));
	}

	if (err_cnt != NULL) {
		Z_OOPS(Z_SYSCALL_MEMORY_WRITE(err_cnt, sizeof(struct can_bus_err_cnt)));
	}

	return z_impl_can_get_state(dev, state, err_cnt);
}
#include <syscalls/can_get_state_mrsh.c>

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static inline int z_vrfy_can_recover(const struct device *dev,
				     k_timeout_t timeout)
{

	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_recover(dev, k_timeout_t timeout);
}
#include <syscalls/can_recover_mrsh.c>
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
