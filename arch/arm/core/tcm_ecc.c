/*
 * Copyright (c) 2026 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Early initialization of the ARM Cortex DTCM / ITCM
 */

#include <zephyr/kernel.h>

#if DT_NODE_HAS_COMPAT_STATUS(DT_CHOSEN(zephyr_itcm), arm_itcm_ecc, okay)
#define ITCM_BASE	DT_REG_ADDR(DT_CHOSEN(zephyr_itcm))
#define ITCM_END	(DT_REG_ADDR(DT_CHOSEN(zephyr_itcm)) + DT_REG_SIZE(DT_CHOSEN(zephyr_itcm)))
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_CHOSEN(zephyr_dtcm), arm_dtcm_ecc, okay)
#define DTCM_BASE	DT_REG_ADDR(DT_CHOSEN(zephyr_dtcm))
#define DTCM_END	(DT_REG_ADDR(DT_CHOSEN(zephyr_dtcm)) + DT_REG_SIZE(DT_CHOSEN(zephyr_dtcm)))
#endif

void cpu_tcm_init(void)
{
#ifdef ITCM_BASE
	volatile uint64_t *itcm_start = (void *)ITCM_BASE;
	volatile uint64_t *itcm_end = (void *)ITCM_END;

	for (volatile uint64_t *p = itcm_start; p < itcm_end; p++) {
		*p = 0;
	}
#endif
#ifdef DTCM_BASE
	volatile uint32_t *dtcm_start = (void *)DTCM_BASE;
	volatile uint32_t *dtcm_end = (void *)DTCM_END;

	for (volatile uint32_t *p = dtcm_start; p < dtcm_end; p++) {
		*p = 0;
	}
#endif
}
