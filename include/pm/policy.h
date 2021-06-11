/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PM_POLICY_H_
#define ZEPHYR_INCLUDE_PM_POLICY_H_

#include <pm/state.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function to get the next PM state based on the ticks
 */
struct pm_state_info pm_policy_next_state(int32_t ticks);


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PM_POLICY_H_ */
