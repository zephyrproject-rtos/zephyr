/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_HAL_BSIM_CPU_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_HAL_BSIM_CPU_H_

#include <zephyr/toolchain.h>

static inline void cpu_dmb(void)
{
	compiler_barrier();
}

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_CONTROLLER_LL_SW_NATIVE_HAL_BSIM_CPU_H_ */
