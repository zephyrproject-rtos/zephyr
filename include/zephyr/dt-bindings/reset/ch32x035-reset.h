/*
 * Copyright (c) 2026 Chen Ren
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_CH32X035_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_CH32X035_RESET_H_

#include "ch32-common.h"

/**
 * @file
 * @brief CH32 reset controller devicetree helper macros for CH32X035.
 * @ingroup reset_controller_ch32
 */

/** @cond INTERNAL_HIDDEN */

/* RCC bus reset register offsets (RM 3.4.3, 3.4.4, 3.4.9) */
#define CH32_RESET_BUS_APB2 0x0C
#define CH32_RESET_BUS_APB1 0x10
#define CH32_RESET_BUS_AHB  0x28

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_CH32X035_RESET_H_ */
