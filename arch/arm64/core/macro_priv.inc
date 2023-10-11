/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MACRO_PRIV_INC_
#define _MACRO_PRIV_INC_

#include <zephyr/arch/arm64/tpidrro_el0.h>

#ifdef _ASMLANGUAGE

/*
 * Get CPU id
 */

.macro get_cpu_id xreg0
	mrs	\xreg0, mpidr_el1
	/* FIMXME: aff3 not taken into consideration */
	ubfx	\xreg0, \xreg0, #0, #24
.endm

/*
 * Get CPU pointer
 * Note: keep in sync with `arch_curr_cpu` in include/zephyr/arch/arm64/arch_inlines.h
 */

.macro get_cpu xreg0
	mrs	\xreg0, tpidrro_el0
	and	\xreg0, \xreg0, #TPIDRROEL0_CURR_CPU
.endm

#endif /* _ASMLANGUAGE */

#endif /* _MACRO_PRIV_INC_ */
