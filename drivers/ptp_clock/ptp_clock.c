/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/drivers/ptp_clock.h>

#ifdef CONFIG_USERSPACE
int z_vrfy_ptp_clock_get(const struct device *dev,
			 struct net_ptp_time *tm)
{
	struct net_ptp_time ptp_time;
	int ret;

	K_OOPS(K_SYSCALL_DRIVER_PTP_CLOCK(dev, get));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(tm, sizeof(struct net_ptp_time)));

	ret = z_impl_ptp_clock_get((const struct device *)dev, &ptp_time);
	if (ret != 0) {
		return 0;
	}

	if (k_usermode_to_copy((void *)tm, &ptp_time, sizeof(ptp_time)) != 0) {
		return 0;
	}

	return ret;
}
#include <zephyr/syscalls/ptp_clock_get_mrsh.c>
#endif /* CONFIG_USERSPACE */
