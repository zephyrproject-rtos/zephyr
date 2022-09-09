/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/syscall_handler.h>
#include <zephyr/drivers/counter.h>

/* For those APIs that just take one argument which is a counter driver
 * instance and return an integral value
 */
#define COUNTER_HANDLER(name) \
	static inline int z_vrfy_counter_##name(const struct device *dev) \
	{ \
		Z_OOPS(Z_SYSCALL_DRIVER_COUNTER(dev, name)); \
		return z_impl_counter_ ## name((const struct device *)dev); \
	}

COUNTER_HANDLER(get_pending_int)
COUNTER_HANDLER(stop)
COUNTER_HANDLER(start)

#include <syscalls/counter_get_pending_int_mrsh.c>
#include <syscalls/counter_stop_mrsh.c>
#include <syscalls/counter_start_mrsh.c>

static inline bool z_vrfy_counter_is_counting_up(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_COUNTER));
	return z_impl_counter_is_counting_up((const struct device *)dev);
}
#include <syscalls/counter_is_counting_up_mrsh.c>

static inline uint8_t z_vrfy_counter_get_num_of_channels(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_COUNTER));
	return z_impl_counter_get_num_of_channels((const struct device *)dev);
}
#include <syscalls/counter_get_num_of_channels_mrsh.c>

static inline uint32_t z_vrfy_counter_get_frequency(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_COUNTER));
	return z_impl_counter_get_frequency((const struct device *)dev);
}
#include <syscalls/counter_get_frequency_mrsh.c>

static inline uint32_t z_vrfy_counter_us_to_ticks(const struct device *dev,
					       uint64_t us)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_COUNTER));
	return z_impl_counter_us_to_ticks((const struct device *)dev,
					  (uint64_t)us);
}
#include <syscalls/counter_us_to_ticks_mrsh.c>

static inline uint64_t z_vrfy_counter_ticks_to_us(const struct device *dev,
					       uint32_t ticks)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_COUNTER));
	return z_impl_counter_ticks_to_us((const struct device *)dev,
					  (uint32_t)ticks);
}
#include <syscalls/counter_ticks_to_us_mrsh.c>

static inline int z_vrfy_counter_get_value(const struct device *dev,
					   uint32_t *ticks)
{
	Z_OOPS(Z_SYSCALL_DRIVER_COUNTER(dev, get_value));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(ticks, sizeof(*ticks)));
	return z_impl_counter_get_value((const struct device *)dev, ticks);
}

static inline int z_vrfy_counter_get_value_64(const struct device *dev,
					   uint64_t *ticks)
{
	Z_OOPS(Z_SYSCALL_DRIVER_COUNTER(dev, get_value_64));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(ticks, sizeof(*ticks)));
	return z_impl_counter_get_value_64((const struct device *)dev, ticks);
}

#include <syscalls/counter_get_value_mrsh.c>

static inline int z_vrfy_counter_set_channel_alarm(const struct device *dev,
						   uint8_t chan_id,
						   const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_alarm_cfg cfg_copy;

	Z_OOPS(Z_SYSCALL_DRIVER_COUNTER(dev, set_alarm));
	Z_OOPS(z_user_from_copy(&cfg_copy, alarm_cfg, sizeof(cfg_copy)));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(cfg_copy.callback == NULL,
				    "callbacks may not be set from user mode"));
	return z_impl_counter_set_channel_alarm((const struct device *)dev,
						(uint8_t)chan_id,
						(const struct counter_alarm_cfg *)&cfg_copy);

}
#include <syscalls/counter_set_channel_alarm_mrsh.c>

static inline int z_vrfy_counter_cancel_channel_alarm(const struct device *dev,
						      uint8_t chan_id)
{
	Z_OOPS(Z_SYSCALL_DRIVER_COUNTER(dev, cancel_alarm));
	return z_impl_counter_cancel_channel_alarm((const struct device *)dev,
						   (uint8_t)chan_id);
}
#include <syscalls/counter_cancel_channel_alarm_mrsh.c>

static inline int z_vrfy_counter_set_top_value(const struct device *dev,
					       const struct counter_top_cfg
					       *cfg)
{
	struct counter_top_cfg cfg_copy;

	Z_OOPS(Z_SYSCALL_DRIVER_COUNTER(dev, set_top_value));
	Z_OOPS(z_user_from_copy(&cfg_copy, cfg, sizeof(cfg_copy)));
	Z_OOPS(Z_SYSCALL_VERIFY_MSG(cfg_copy.callback == NULL,
				    "callbacks may not be set from user mode"));
	return z_impl_counter_set_top_value((const struct device *)dev,
					    (const struct counter_top_cfg *)
					    &cfg_copy);
}
#include <syscalls/counter_set_top_value_mrsh.c>

static inline uint32_t z_vrfy_counter_get_top_value(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_COUNTER(dev, get_top_value));
	return z_impl_counter_get_top_value((const struct device *)dev);
}
#include <syscalls/counter_get_top_value_mrsh.c>

static inline uint32_t z_vrfy_counter_get_max_top_value(const struct device *dev)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_COUNTER));
	return z_impl_counter_get_max_top_value((const struct device *)dev);
}
#include <syscalls/counter_get_max_top_value_mrsh.c>

static inline uint32_t z_vrfy_counter_get_guard_period(const struct device *dev,
							uint32_t flags)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_COUNTER));
	return z_impl_counter_get_guard_period((const struct device *)dev,
					       flags);
}
#include <syscalls/counter_get_guard_period_mrsh.c>

static inline int z_vrfy_counter_set_guard_period(const struct device *dev,
						   uint32_t ticks, uint32_t flags)
{
	Z_OOPS(Z_SYSCALL_OBJ(dev, K_OBJ_DRIVER_COUNTER));
	return z_impl_counter_set_guard_period((const struct device *)dev,
						ticks,
						flags);
}
#include <syscalls/counter_set_guard_period_mrsh.c>
