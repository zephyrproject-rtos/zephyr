/*
 * Copyright (c) 2021 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the HummingBird core
 */

#ifndef RISCV_HUMMINGBIRD_SOC_H_
#define RISCV_HUMMINGBIRD_SOC_H_

#include <soc_common.h>
#include <devicetree.h>

/* Timer configuration */
#define HBIRD_MTIMER_OFFSET 0xBFF8
#define HBIRD_MTIMERCMP_OFFSET 0x4000
#define RISCV_MTIME_BASE    (DT_REG_ADDR_BY_IDX(DT_NODELABEL(mtimer), 0) + HBIRD_MTIMER_OFFSET)
#define RISCV_MTIMECMP_BASE (DT_REG_ADDR_BY_IDX(DT_NODELABEL(mtimer), 0) + HBIRD_MTIMERCMP_OFFSET)

#ifndef _ASMLANGUAGE
#include <toolchain.h>
#endif  /* !_ASMLANGUAGE */

#endif  /* RISCV_HUMMINGBIRD_SOC_H_ */
