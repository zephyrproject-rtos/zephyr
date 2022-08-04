/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/syscall_handler.h>
#include <zephyr/drivers/can.h>

static int z_vrfy_can_calc_timing(const struct device *dev, struct can_timing *res,
				  uint32_t bitrate, uint16_t sample_pnt)
{
	struct can_timing res_copy;
	int err;

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, get_core_clock));
	Z_OOPS(z_user_from_copy(&res_copy, res, sizeof(res_copy)));

	err = z_impl_can_calc_timing(dev, &res_copy, bitrate, sample_pnt);
	Z_OOPS(z_user_to_copy(res, &res_copy, sizeof(*res)));

	return err;
}
#include <syscalls/can_calc_timing_mrsh.c>

static inline int z_vrfy_can_set_timing(const struct device *dev,
					const struct can_timing *timing)
{
	struct can_timing timing_copy;

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, set_timing));
	Z_OOPS(z_user_from_copy(&timing_copy, timing, sizeof(timing_copy)));

	return z_impl_can_set_timing(dev, &timing_copy);
}
#include <syscalls/can_set_timing_mrsh.c>

static inline int z_vrfy_can_get_core_clock(const struct device *dev,
					    uint32_t *rate)
{
	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, get_core_clock));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(rate, sizeof(*rate)));

	return z_impl_can_get_core_clock(dev, rate);
}
#include <syscalls/can_get_core_clock_mrsh.c>

static inline int z_vrfy_can_get_max_bitrate(const struct device *dev,
					     uint32_t *max_bitrate)
{
	/* Optional API function */
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(max_bitrate, sizeof(*max_bitrate)));

	return z_impl_can_get_max_bitrate(dev, max_bitrate);
}
#include <syscalls/can_get_max_bitrate_mrsh.c>

static inline const struct can_timing *z_vrfy_can_get_timing_min(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_get_timing_min(dev);
}
#include <syscalls/can_get_timing_min_mrsh.c>

static inline const struct can_timing *z_vrfy_can_get_timing_max(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_get_timing_max(dev);
}
#include <syscalls/can_get_timing_max_mrsh.c>

#ifdef CONFIG_CAN_FD_MODE

static int z_vrfy_can_calc_timing_data(const struct device *dev, struct can_timing *res,
				       uint32_t bitrate, uint16_t sample_pnt)
{
	struct can_timing res_copy;
	int err;

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, get_core_clock));
	Z_OOPS(z_user_from_copy(&res_copy, res, sizeof(res_copy)));

	err = z_impl_can_calc_timing_data(dev, &res_copy, bitrate, sample_pnt);
	Z_OOPS(z_user_to_copy(res, &res_copy, sizeof(*res)));

	return err;
}
#include <syscalls/can_calc_timing_data_mrsh.c>

static inline const struct can_timing *z_vrfy_can_get_timing_data_min(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_get_timing_data_min(dev);
}
#include <syscalls/can_get_timing_data_min_mrsh.c>

static inline const struct can_timing *z_vrfy_can_get_timing_data_max(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_get_timing_data_max(dev);
}
#include <syscalls/can_get_timing_data_max_mrsh.c>

static inline int z_vrfy_can_set_timing_data(const struct device *dev,
					     const struct can_timing *timing_data)
{
	struct can_timing timing_data_copy;

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, set_timing_data));
	Z_OOPS(z_user_from_copy(&timing_data_copy, timing_data, sizeof(timing_data_copy)));

	return z_impl_can_set_timing_data(dev, &timing_data_copy);
}
#include <syscalls/can_set_timing_data_mrsh.c>

static inline int z_vrfy_can_set_bitrate_data(const struct device *dev,
					      uint32_t bitrate_data)
{
	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, set_timing_data));

	return z_impl_can_set_bitrate_data(dev, bitrate_data);
}
#include <syscalls/can_set_bitrate_data_mrsh.c>

#endif /* CONFIG_CAN_FD_MODE */

static inline int z_vrfy_can_get_max_filters(const struct device *dev, enum can_ide id_type)
{
	/* Optional API function */
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_get_max_filters(dev, id_type);
}
#include <syscalls/can_get_max_filters_mrsh.c>

static inline int z_vrfy_can_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, get_capabilities));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(cap, sizeof(*cap)));

	return z_impl_can_get_capabilities(dev, cap);
}
#include <syscalls/can_get_capabilities_mrsh.c>

static inline int z_vrfy_can_set_mode(const struct device *dev, can_mode_t mode)
{
	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, set_mode));

	return z_impl_can_set_mode(dev, mode);
}
#include <syscalls/can_set_mode_mrsh.c>

static inline int z_vrfy_can_set_bitrate(const struct device *dev, uint32_t bitrate)
{
	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, set_timing));

	return z_impl_can_set_bitrate(dev, bitrate);
}
#include <syscalls/can_set_bitrate_mrsh.c>

static inline int z_vrfy_can_send(const struct device *dev,
				  const struct zcan_frame *frame,
				  k_timeout_t timeout,
				  can_tx_callback_t callback,
				  void *user_data)
{
	struct zcan_frame frame_copy;

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, send));
	Z_OOPS(z_user_from_copy(&frame_copy, frame, sizeof(frame_copy)));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(callback == NULL, "callbacks may not be set from user mode"));

	return z_impl_can_send(dev, &frame_copy, timeout, callback, user_data);
}
#include <syscalls/can_send_mrsh.c>

static inline int z_vrfy_can_add_rx_filter_msgq(const struct device *dev,
						struct k_msgq *msgq,
						const struct zcan_filter *filter)
{
	struct zcan_filter filter_copy;

	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, add_rx_filter));
	Z_OOPS(Z_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	Z_OOPS(z_user_from_copy(&filter_copy, filter, sizeof(filter_copy)));

	return z_impl_can_add_rx_filter_msgq(dev, msgq, &filter_copy);
}
#include <syscalls/can_add_rx_filter_msgq_mrsh.c>

static inline void z_vrfy_can_remove_rx_filter(const struct device *dev, int filter_id)
{
	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, remove_rx_filter));

	z_impl_can_remove_rx_filter(dev, filter_id);
}
#include <syscalls/can_remove_rx_filter_mrsh.c>

static inline int z_vrfy_can_get_state(const struct device *dev, enum can_state *state,
				       struct can_bus_err_cnt *err_cnt)
{
	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, get_state));

	if (state != NULL) {
		Z_OOPS(Z_SYSCALL_MEMORY_WRITE(state, sizeof(*state)));
	}

	if (err_cnt != NULL) {
		Z_OOPS(Z_SYSCALL_MEMORY_WRITE(err_cnt, sizeof(*err_cnt)));
	}

	return z_impl_can_get_state(dev, state, err_cnt);
}
#include <syscalls/can_get_state_mrsh.c>

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static inline int z_vrfy_can_recover(const struct device *dev, k_timeout_t timeout)
{
	Z_OOPS(Z_SYSCALL_DRIVER_CAN(dev, recover));

	return z_impl_can_recover(dev, timeout);
}
#include <syscalls/can_recover_mrsh.c>
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */
