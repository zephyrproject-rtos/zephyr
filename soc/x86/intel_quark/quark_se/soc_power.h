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
 * SYS_POWER_STATE_LOW_POWER_1:		C1 state
 * SYS_POWER_STATE_LOW_POWER_2:		C2 state
 * SYS_POWER_STATE_LOW_POWER_3:		C2LP state
 * SYS_POWER_STATE_DEEP_SLEEP_1:	SLEEP state
 * SYS_POWER_STATE_DEEP_SLEEP_2:	SLEEP state with LPMODE enabled
 */

/**
 * @brief Check if ARC core is ready to enter in DEEP_SLEEP states.
 *
 * @retval true If ARC is ready.
 * @retval false Otherwise.
 */
bool sys_power_state_is_arc_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* _SOC_POWER_H_ */
