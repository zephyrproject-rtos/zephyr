/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_POWER_H_
#define _SOC_POWER_H_

#include <stdbool.h>
#include <power.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT

/*
 * Power state map:
 * SYS_POWER_STATE_DEEP_SLEEP: System OFF
 */

/**
 * @brief Put processor into low power state
 */
void sys_set_power_state(enum power_states state);

/**
 * @brief Do any SoC or architecture specific post ops after low power states.
 */
void sys_power_state_post_ops(enum power_states state);

#endif /* CONFIG_SYS_POWER_MANAGEMENT */

#ifdef __cplusplus
}
#endif

#endif /* _SOC_POWER_H_ */
