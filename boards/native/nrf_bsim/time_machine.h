/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _BOARD_POSIX_NRF52_BSIM_TIME_MACHINE_H
#define _BOARD_POSIX_NRF52_BSIM_TIME_MACHINE_H

#include "bs_types.h"
#include "time_machine_if.h"
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This header and prototype exists for backwards compatibility with old bsim tests */
void tm_set_phy_max_resync_offset(bs_time_t offset_in_us);

#ifdef __cplusplus
}
#endif

#endif
