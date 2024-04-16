/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BOARDS_POSIX_NRF52_BSIM_BOARD_IRQ_H
#define BOARDS_POSIX_NRF52_BSIM_BOARD_IRQ_H

#include <zephyr/types.h>
#include "../common/irq/board_irq.h"

#ifdef __cplusplus
extern "C" {
#endif

void nrfbsim_WFE_model(void);
void nrfbsim_SEV_model(void);

#define IRQ_ZERO_LATENCY	BIT(1) /* Unused in this board*/

#ifdef __cplusplus
}
#endif

#endif /* BOARDS_POSIX_NRF52_BSIM_BOARD_IRQ_H */
