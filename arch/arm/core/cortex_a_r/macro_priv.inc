/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MACRO_PRIV_INC_
#define _MACRO_PRIV_INC_

#include <zephyr/arch/arm/cortex_a_r/tpidruro.h>

.macro get_cpu rreg0
	/*
         * Get CPU pointer.
	 */
	mrc p15, 0, \rreg0, c13, c0, 3
        and \rreg0, #TPIDRURO_CURR_CPU
.endm

#endif /* _MACRO_PRIV_INC_ */
