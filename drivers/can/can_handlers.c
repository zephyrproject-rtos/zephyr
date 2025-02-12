/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/can.h>

static int z_vrfy_can_calc_timing(const struct device *dev, struct can_timing *res,
				  uint32_t bitrate, uint16_t sample_pnt)
{
	struct can_timing res_copy;
	int err;

	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, get_core_clock));
	K_OOPS(k_usermode_from_copy(&res_copy, res, sizeof(res_copy)));

	err = z_impl_can_calc_timing(dev, &res_copy, bitrate, sample_pnt);
	K_OOPS(k_usermode_to_copy(res, &res_copy, sizeof(*res)));

	return err;
}
#include <zephyr/syscalls/can_calc_timing_mrsh.c>

static inline int z_vrfy_can_set_timing(const struct device *dev,
					const struct can_timing *timing)
{
	struct can_timing timing_copy;

	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, set_timing));
	K_OOPS(k_usermode_from_copy(&timing_copy, timing, sizeof(timing_copy)));

	return z_impl_can_set_timing(dev, &timing_copy);
}
#include <zephyr/syscalls/can_set_timing_mrsh.c>

static inline int z_vrfy_can_get_core_clock(const struct device *dev,
					    uint32_t *rate)
{
	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, get_core_clock));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(rate, sizeof(*rate)));

	return z_impl_can_get_core_clock(dev, rate);
}
#include <zephyr/syscalls/can_get_core_clock_mrsh.c>

static inline uint32_t z_vrfy_can_get_bitrate_min(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_get_bitrate_min(dev);
}
#include <zephyr/syscalls/can_get_bitrate_min_mrsh.c>

static inline uint32_t z_vrfy_can_get_bitrate_max(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_get_bitrate_max(dev);
}
#include <zephyr/syscalls/can_get_bitrate_max_mrsh.c>

static inline const struct can_timing *z_vrfy_can_get_timing_min(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_get_timing_min(dev);
}
#include <zephyr/syscalls/can_get_timing_min_mrsh.c>

static inline const struct can_timing *z_vrfy_can_get_timing_max(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_get_timing_max(dev);
}
#include <zephyr/syscalls/can_get_timing_max_mrsh.c>

#ifdef CONFIG_CAN_FD_MODE

static int z_vrfy_can_calc_timing_data(const struct device *dev, struct can_timing *res,
				       uint32_t bitrate, uint16_t sample_pnt)
{
	struct can_timing res_copy;
	int err;

	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, get_core_clock));
	K_OOPS(k_usermode_from_copy(&res_copy, res, sizeof(res_copy)));

	err = z_impl_can_calc_timing_data(dev, &res_copy, bitrate, sample_pnt);
	K_OOPS(k_usermode_to_copy(res, &res_copy, sizeof(*res)));

	return err;
}
#include <zephyr/syscalls/can_calc_timing_data_mrsh.c>

static inline const struct can_timing *z_vrfy_can_get_timing_data_min(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_get_timing_data_min(dev);
}
#include <zephyr/syscalls/can_get_timing_data_min_mrsh.c>

static inline const struct can_timing *z_vrfy_can_get_timing_data_max(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_get_timing_data_max(dev);
}
#include <zephyr/syscalls/can_get_timing_data_max_mrsh.c>

static inline int z_vrfy_can_set_timing_data(const struct device *dev,
					     const struct can_timing *timing_data)
{
	struct can_timing timing_data_copy;

	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, set_timing_data));
	K_OOPS(k_usermode_from_copy(&timing_data_copy, timing_data, sizeof(timing_data_copy)));

	return z_impl_can_set_timing_data(dev, &timing_data_copy);
}
#include <zephyr/syscalls/can_set_timing_data_mrsh.c>

static inline int z_vrfy_can_set_bitrate_data(const struct device *dev,
					      uint32_t bitrate_data)
{
	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, set_timing_data));

	return z_impl_can_set_bitrate_data(dev, bitrate_data);
}
#include <zephyr/syscalls/can_set_bitrate_data_mrsh.c>

#endif /* CONFIG_CAN_FD_MODE */

static inline int z_vrfy_can_get_max_filters(const struct device *dev, bool ide)
{
	/* Optional API function */
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_get_max_filters(dev, ide);
}
#include <zephyr/syscalls/can_get_max_filters_mrsh.c>

static inline int z_vrfy_can_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, get_capabilities));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(cap, sizeof(*cap)));

	return z_impl_can_get_capabilities(dev, cap);
}
#include <zephyr/syscalls/can_get_capabilities_mrsh.c>

static inline const struct device *z_vrfy_can_get_transceiver(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_get_transceiver(dev);
}
#include <zephyr/syscalls/can_get_transceiver_mrsh.c>

static inline int z_vrfy_can_start(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, start));

	return z_impl_can_start(dev);
}
#include <zephyr/syscalls/can_start_mrsh.c>

static inline int z_vrfy_can_stop(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, stop));

	return z_impl_can_stop(dev);
}
#include <zephyr/syscalls/can_stop_mrsh.c>

static inline int z_vrfy_can_set_mode(const struct device *dev, can_mode_t mode)
{
	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, set_mode));

	return z_impl_can_set_mode(dev, mode);
}
#include <zephyr/syscalls/can_set_mode_mrsh.c>

static inline can_mode_t z_vrfy_can_get_mode(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_get_mode(dev);
}
#include <zephyr/syscalls/can_get_mode_mrsh.c>

static inline int z_vrfy_can_set_bitrate(const struct device *dev, uint32_t bitrate)
{
	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, set_timing));

	return z_impl_can_set_bitrate(dev, bitrate);
}
#include <zephyr/syscalls/can_set_bitrate_mrsh.c>

static inline int z_vrfy_can_send(const struct device *dev,
				  const struct can_frame *frame,
				  k_timeout_t timeout,
				  can_tx_callback_t callback,
				  void *user_data)
{
	struct can_frame frame_copy;

	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, send));
	K_OOPS(k_usermode_from_copy(&frame_copy, frame, sizeof(frame_copy)));
	K_OOPS(K_SYSCALL_VERIFY_MSG(callback == NULL, "callbacks may not be set from user mode"));

	return z_impl_can_send(dev, &frame_copy, timeout, callback, user_data);
}
#include <zephyr/syscalls/can_send_mrsh.c>

static inline int z_vrfy_can_add_rx_filter_msgq(const struct device *dev,
						struct k_msgq *msgq,
						const struct can_filter *filter)
{
	struct can_filter filter_copy;

	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, add_rx_filter));
	K_OOPS(K_SYSCALL_OBJ(msgq, K_OBJ_MSGQ));
	K_OOPS(k_usermode_from_copy(&filter_copy, filter, sizeof(filter_copy)));

	return z_impl_can_add_rx_filter_msgq(dev, msgq, &filter_copy);
}
#include <zephyr/syscalls/can_add_rx_filter_msgq_mrsh.c>

static inline void z_vrfy_can_remove_rx_filter(const struct device *dev, int filter_id)
{
	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, remove_rx_filter));

	z_impl_can_remove_rx_filter(dev, filter_id);
}
#include <zephyr/syscalls/can_remove_rx_filter_mrsh.c>

static inline int z_vrfy_can_get_state(const struct device *dev, enum can_state *state,
				       struct can_bus_err_cnt *err_cnt)
{
	K_OOPS(K_SYSCALL_DRIVER_CAN(dev, get_state));

	if (state != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(state, sizeof(*state)));
	}

	if (err_cnt != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(err_cnt, sizeof(*err_cnt)));
	}

	return z_impl_can_get_state(dev, state, err_cnt);
}
#include <zephyr/syscalls/can_get_state_mrsh.c>

#ifdef CONFIG_CAN_MANUAL_RECOVERY_MODE
static inline int z_vrfy_can_recover(const struct device *dev, k_timeout_t timeout)
{
	/* Optional API function */
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_recover(dev, timeout);
}
#include <zephyr/syscalls/can_recover_mrsh.c>
#endif /* CONFIG_CAN_MANUAL_RECOVERY_MODE */

#ifdef CONFIG_CAN_STATS

static inline uint32_t z_vrfy_can_stats_get_bit_errors(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_stats_get_bit_errors(dev);
}
#include <zephyr/syscalls/can_stats_get_bit_errors_mrsh.c>

static inline uint32_t z_vrfy_can_stats_get_bit0_errors(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_stats_get_bit0_errors(dev);
}
#include <zephyr/syscalls/can_stats_get_bit0_errors_mrsh.c>

static inline uint32_t z_vrfy_can_stats_get_bit1_errors(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_stats_get_bit1_errors(dev);
}
#include <zephyr/syscalls/can_stats_get_bit1_errors_mrsh.c>

static inline uint32_t z_vrfy_can_stats_get_stuff_errors(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_stats_get_stuff_errors(dev);
}
#include <zephyr/syscalls/can_stats_get_stuff_errors_mrsh.c>

static inline uint32_t z_vrfy_can_stats_get_crc_errors(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_stats_get_crc_errors(dev);
}
#include <zephyr/syscalls/can_stats_get_crc_errors_mrsh.c>

static inline uint32_t z_vrfy_can_stats_get_form_errors(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_stats_get_form_errors(dev);
}
#include <zephyr/syscalls/can_stats_get_form_errors_mrsh.c>

static inline uint32_t z_vrfy_can_stats_get_ack_errors(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_stats_get_ack_errors(dev);
}
#include <zephyr/syscalls/can_stats_get_ack_errors_mrsh.c>

static inline uint32_t z_vrfy_can_stats_get_rx_overruns(const struct device *dev)
{
	K_OOPS(K_SYSCALL_OBJ(dev, K_OBJ_DRIVER_CAN));

	return z_impl_can_stats_get_rx_overruns(dev);
}
#include <zephyr/syscalls/can_stats_get_rx_overruns_mrsh.c>

#endif /* CONFIG_CAN_STATS */
