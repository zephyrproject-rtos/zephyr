/*
 * Copyright (c) 2026 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CH32 reset controller devicetree helper macros for CH32V00X.
 * @ingroup reset_controller_ch32
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_CH32V00X_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_CH32V00X_RESET_H_

#include "ch32-common.h"

/** @cond INTERNAL_HIDDEN */

/* RCC bus reset register offset */
#define CH32_RESET_BUS_APB2 0x0C
#define CH32_RESET_BUS_APB1 0x10

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_CH32V00X_RESET_H_ */
