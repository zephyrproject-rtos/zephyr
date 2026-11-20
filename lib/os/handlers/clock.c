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
	struct timespec ts_copy;

	K_OOPS(k_usermode_from_copy(&ts_copy, ts, sizeof(ts_copy)));
	return z_impl_sys_clock_settime(clock_id, &ts_copy);
}
#include <zephyr/syscalls/sys_clock_settime_mrsh.c>

int z_vrfy_sys_clock_nanosleep(int clock_id, int flags, const struct timespec *rqtp,
			       struct timespec *rmtp)
{
	struct timespec rqtp_copy;

	K_OOPS(k_usermode_from_copy(&rqtp_copy, rqtp, sizeof(rqtp_copy)));
	if (rmtp != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(rmtp, sizeof(*rmtp)));
	}
	return z_impl_sys_clock_nanosleep(clock_id, flags, &rqtp_copy, rmtp);
}
#include <zephyr/syscalls/sys_clock_nanosleep_mrsh.c>
