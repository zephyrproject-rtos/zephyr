/*
 * Copyright (c) 2025 GP Orcullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_SAM0_H_
#define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_SAM0_H_

#include <zephyr/sys/util.h>

#define DIR_OFFSET    0x00
#define DIRCLR_OFFSET 0x04
#define DIRSET_OFFSET 0x08
#define OUT_OFFSET    0x10
#define OUTCLR_OFFSET 0x14
#define OUTSET_OFFSET 0x18
#define OUTTGL_OFFSET 0x1C
#define IN_OFFSET     0x20
#define PMUX_OFFSET   0x30
#define PINCFG_OFFSET 0x40

#define PINCFG_PMUXEN_BIT 0
#define PINCFG_INEN_BIT   1
#define PINCFG_PULLEN_BIT 2

#define PMUX_PMUXE_MASK GENMASK(3, 0)
#define PMUX_PMUXO_MASK GENMASK(7, 4)

#endif /* ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_SAM0_H_ */
