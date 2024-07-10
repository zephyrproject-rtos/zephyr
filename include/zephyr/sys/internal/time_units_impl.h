/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TIME_UNITS_H_
#error "Should only be included by zephyr/sys/time_units.h"
#endif

#ifndef ZEPHYR_INCLUDE_SYS_INTERNAL_TIME_UNITS_IMPL_H_
#define ZEPHYR_INCLUDE_SYS_INTERNAL_TIME_UNITS_IMPL_H_

#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
static inline int z_impl_sys_clock_hw_cycles_per_sec_runtime_get(void)
{
	extern int z_clock_hw_cycles_per_sec;

	return z_clock_hw_cycles_per_sec;
}
#endif /* CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZEPHYR_INCLUDE_SYS_INTERNAL_TIME_UNITS_IMPL_H_ */
