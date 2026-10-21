/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2018 Friedt Professional Engineering Services, Inc
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * System call verifiers for lib/os/clock.c, compiled only with CONFIG_USERSPACE.
 */

#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/clock.h>

void z_vrfy_sys_clock_getrtoffset(struct timespec *tp)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(tp, sizeof(*tp)));
	return z_impl_sys_clock_getrtoffset(tp);
}
#include <zephyr/syscalls/sys_clock_getrtoffset_mrsh.c>

int z_vrfy_sys_clock_settime(int clock_id, const struct timespec *ts)
{
	K_OOPS(K_SYSCALL_MEMORY_READ(ts, sizeof(*ts)));
	return z_impl_sys_clock_settime(clock_id, ts);
}
#include <zephyr/syscalls/sys_clock_settime_mrsh.c>

int z_vrfy_sys_clock_nanosleep(int clock_id, int flags, const struct timespec *rqtp,
			       struct timespec *rmtp)
{
	K_OOPS(K_SYSCALL_MEMORY_READ(rqtp, sizeof(*rqtp)));
	if (rmtp != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(rmtp, sizeof(*rmtp)));
	}
	return z_impl_sys_clock_nanosleep(clock_id, flags, rqtp, rmtp);
}
#include <zephyr/syscalls/sys_clock_nanosleep_mrsh.c>
