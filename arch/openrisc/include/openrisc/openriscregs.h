/*
 * Copyright (c) 2025 NVIDIA Corporation
 *
 * Macros for OpenRISC SPR manipulations
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_ARCH_OPENRISC_INCLUDE_OPENRISC_OPENRISCREGS_H_
#define _ZEPHYR_ARCH_OPENRISC_INCLUDE_OPENRISC_OPENRISCREGS_H_

#include <openrisc/spr_defs.h>

#define openrisc_write_spr(spr, val) \
({ \
	__asm__ __volatile__ ("l.mtspr r0,%0,%1" \
	: \
	: "r" (val), "K" (spr)); \
})

#define openrisc_read_spr(spr) \
({ \
	uint32_t val; \
	__asm__ __volatile__ ("l.mfspr %0,r0,%1" \
	: "=r" (val) \
	: "K" (spr)); \
	val; \
})

#endif /* _ZEPHYR_ARCH_OPENRISC_INCLUDE_OPENRISC_OPENRISCREGS_H_ */
