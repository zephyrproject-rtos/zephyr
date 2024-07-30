/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "test_helpers.h"
#include <zephyr/logging/log_internal.h>
#include <zephyr/logging/log_ctrl.h>


static log_timestamp_t stamp;

static log_timestamp_t timestamp_get(void)
{
	return stamp++;
}

void z_impl_test_helpers_log_setup(void)
{
	stamp = 0;
	log_core_init();
	log_init();
	z_log_dropped_read_and_clear();

	log_set_timestamp_func(timestamp_get, 0);

}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_test_helpers_log_setup(void)
{
	return z_impl_test_helpers_log_setup();
}
#include <zephyr/syscalls/test_helpers_log_setup_mrsh.c>
#endif

int z_impl_test_helpers_cycle_get(void)
{
	return arch_k_cycle_get_32();
}

#ifdef CONFIG_USERSPACE
static inline int z_vrfy_test_helpers_cycle_get(void)
{
	return z_impl_test_helpers_cycle_get();
}
#include <zephyr/syscalls/test_helpers_cycle_get_mrsh.c>
#endif

bool z_impl_test_helpers_log_dropped_pending(void)
{
	return z_log_dropped_pending();
}

#ifdef CONFIG_USERSPACE
static inline bool z_vrfy_test_helpers_log_dropped_pending(void)
{
	return z_impl_test_helpers_log_dropped_pending();
}
#include <zephyr/syscalls/test_helpers_log_dropped_pending_mrsh.c>
#endif
