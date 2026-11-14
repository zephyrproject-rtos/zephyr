/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_HAL_BSIM_CNTR_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_HAL_BSIM_CNTR_H_

#include <stdbool.h>

/* The compare event shares its interrupt line with the ULL mayflies, so the
 * ISR has to ask whether the compare is what fired.
 */
bool cntr_cmp_evt_get_clear(void);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_HAL_BSIM_CNTR_H_ */
