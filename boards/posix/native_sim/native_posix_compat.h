/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file provides in native_sim a set of APIs the native_posix board provided
 * to allow building the native_posix drivers or applications which depended
 * on those.
 * Note that all these APIs should be considered deprecated in native_sim, as this
 * exists solely as a transitional component.
 */

#ifndef BOARDS_POSIX_NATIVE_SIM_NATIVE_POSIX_COMPAT_H
#define BOARDS_POSIX_NATIVE_SIM_NATIVE_POSIX_COMPAT_H

#include <stdint.h>
#include <zephyr/toolchain.h>
#include "nsi_hw_scheduler.h"

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE void hwm_find_next_timer(void)
{
	nsi_hws_find_next_event();
}

static ALWAYS_INLINE uint64_t hwm_get_time(void)
{
	return nsi_hws_get_time();
}

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_POSIX_NATIVE_SIM_NATIVE_POSIX_COMPAT_H */
