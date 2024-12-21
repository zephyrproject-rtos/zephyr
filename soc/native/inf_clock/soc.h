/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _POSIX_SOC_INF_CLOCK_SOC_H
#define _POSIX_SOC_INF_CLOCK_SOC_H

#include <stdbool.h>
#include <zephyr/toolchain.h>
#include "board_soc.h"
#include "posix_soc.h"
#include "posix_native_task.h"

#ifdef __cplusplus
extern "C" {
#endif

void posix_soc_clean_up(void);

/**
 * Remap an embedded device address, into an address which can be used in the simulated native
 * board.
 *
 * If the provided address is not in a range known to this function it will not be modified and the
 * function will return false.
 * Otherwise the provided address pointer will be modified, and true returned.
 *
 * Note: The SOC provides a dummy version of this function which does nothing,
 * so all boards will have an implementation.
 * It is optional for boards to provide one if desired.
 */
bool native_emb_addr_remap(void **addr);

#ifdef __cplusplus
}
#endif

#endif /* _POSIX_SOC_INF_CLOCK_SOC_H */
