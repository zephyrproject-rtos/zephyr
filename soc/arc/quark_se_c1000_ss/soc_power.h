/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_POWER_H_
#define _SOC_POWER_H_

#include <power.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Bit 0 from GP0 register is used internally by the kernel
 * to handle PM multicore support. Any change on QMSI and/or
 * bootloader which affects this bit should take it in
 * consideration.
 */
#define GP0_BIT_SLEEP_READY BIT(0)

/*
 * Power state map:
 * SYS_POWER_STATE_SLEEP_1:		SS1 state with Timer ON
 * SYS_POWER_STATE_SLEEP_2:		SS1 state with Timer ON
 * SYS_POWER_STATE_DEEP_SLEEP_1:	SS2 with LPSS enabled state
 * SYS_POWER_STATE_DEEP_SLEEP_2:	SLEEP state
 * SYS_POWER_STATE_DEEP_SLEEP_3:	SLEEP state with LPMODE enabled
 */

#ifdef __cplusplus
}
#endif

#endif /* _SOC_POWER_H_ */
