/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Bouffalo Lab SoC specific attributes for memory relocation
 */

#ifndef ZEPHYR_SOC_RISCV_BFLB_COMMON_SOC_MEMORY_ATTRIBUTES_H_
#define ZEPHYR_SOC_RISCV_BFLB_COMMON_SOC_MEMORY_ATTRIBUTES_H_

/** Enforce relocation of critical code into TCM coupled to CPU directly */
#if defined(CONFIG_SOC_SERIES_BL61X)
/* All RAM is tightly coupled on BL61x */
#define __critfunc __ramfunc
#else
/* Only TCMs are tightly coupled on E24 platforms */
#define __critfunc __GENERIC_SECTION(.itcm)
#endif

#endif /* ZEPHYR_SOC_RISCV_BFLB_COMMON_SOC_MEMORY_ATTRIBUTES_H_ */
