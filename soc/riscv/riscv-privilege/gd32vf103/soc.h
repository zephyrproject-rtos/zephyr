/*
 * Copyright (c) 2021 Tokita, Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the GigaDevice GD32VF103 processor
 */

#ifndef RISCV_GD32VF103_SOC_H_
#define RISCV_GD32VF103_SOC_H_

#include <soc_common.h>
#include <zephyr/devicetree.h>

/* Timer configuration */
#define RISCV_MTIME_BASE      DT_REG_ADDR(DT_NODELABEL(systimer))
#define RISCV_MTIMECMP_BASE   (RISCV_MTIME_BASE + 8)

#endif  /* RISCV_GD32VF103_SOC_H */
