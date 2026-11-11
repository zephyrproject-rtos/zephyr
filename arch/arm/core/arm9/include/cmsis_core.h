/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ARMv5 predates CMSIS-Core, but a fair amount of common Zephyr ARM code
 * includes <cmsis_core.h> unconditionally for the __DSB()/__WFI()/... family
 * of intrinsics. Provide those from the ARMv5 CP15 helpers instead.
 */

#ifndef ZEPHYR_ARCH_ARM_CORE_ARM9_CMSIS_CORE_H_
#define ZEPHYR_ARCH_ARM_CORE_ARM9_CMSIS_CORE_H_

#include <zephyr/arch/arm/arm9/lib_helpers.h>

#endif /* ZEPHYR_ARCH_ARM_CORE_ARM9_CMSIS_CORE_H_ */
