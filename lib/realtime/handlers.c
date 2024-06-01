/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/realtime.h>
#include <zephyr/internal/syscall_handler.h>

static inline int z_vrfy_sys_realtime_get_timestamp(int64_t *timestamp_ms)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(timestamp_ms, sizeof(int64_t)));
	return z_impl_sys_realtime_get_timestamp(timestamp_ms);
}
#include <zephyr/syscalls/sys_realtime_get_timestamp_mrsh.c>

static inline int z_vrfy_sys_realtime_set_timestamp(const int64_t *timestamp_ms)
{
	K_OOPS(K_SYSCALL_MEMORY_READ(timestamp_ms, sizeof(int64_t)));
	return z_impl_sys_realtime_set_timestamp(timestamp_ms);
}
#include <zephyr/syscalls/sys_realtime_set_timestamp_mrsh.c>

static inline int z_vrfy_sys_realtime_get_datetime(struct sys_datetime *datetime)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(datetime, sizeof(struct sys_datetime)));
	return z_impl_sys_realtime_get_datetime(datetime);
}
#include <zephyr/syscalls/sys_realtime_get_datetime_mrsh.c>

static inline int z_vrfy_sys_realtime_set_datetime(const struct sys_datetime *datetime)
{
	K_OOPS(K_SYSCALL_MEMORY_READ(datetime, sizeof(struct sys_datetime)));
	return z_impl_sys_realtime_set_datetime(datetime);
}
#include <zephyr/syscalls/sys_realtime_set_datetime_mrsh.c>
