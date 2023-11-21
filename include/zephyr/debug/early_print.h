 /*
  * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef ZEPHYR_INCLUDE_DEBUG_EARLY_PRINT_H_
#define ZEPHYR_INCLUDE_DEBUG_EARLY_PRINT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Define early print init stages. Different platforms might have their own demands
 * of init stages, e.g. Arm64 with MMU using UART needs two stages init,
 * 1) UART device early init, 2) early map for UART MMIO after MMU is enabled.
 * Most MPU system platforms probably need only one stage, hence currently define
 * maximum stages for the use cases that demand the most stages.
 */
#define EARLY_PRINT_INIT	0
#define EARLY_PRINT_MMIO_MAP	1

void z_early_print_init(int stage);

#ifdef __cplusplus
}
#endif

#endif
