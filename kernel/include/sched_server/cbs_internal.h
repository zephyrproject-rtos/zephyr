/*
 *  Copyright (c) 2024 Instituto Superior de Engenharia do Porto (ISEP)
 *  SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * @brief Constant Bandwidth Server (CBS) internal APIs and definitions
 */

#ifndef ZEPHYR_CBS_INTERNAL
#define ZEPHYR_CBS_INTERNAL

#include <zephyr/sched_server/cbs.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_TIMEOUT_64BIT
#define CBS_TICKS_TO_CYC(t) k_ticks_to_cyc_floor32(t)
#else
#define CBS_TICKS_TO_CYC(t) k_ticks_to_cyc_floor64(t)
#endif

void cbs_switched_in(struct k_cbs *cbs);
void cbs_switched_out(struct k_cbs *cbs);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_CBS_INTERNAL */
