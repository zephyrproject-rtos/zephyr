/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRFxx family processors
 */

#include <zephyr/toolchain.h>

_ASM_FILE_PROLOGUE

GTEXT(SystemInit)
GTEXT(z_arm_platform_init)

SECTION_FUNC(TEXT, z_arm_platform_init)

	/* Implement z_arm_platform_init() directly in ASM,
	 * and ensure no stack access is performed until
	 * we jump to SystemInit().
	 */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	ldr r0, =SystemInit
	bx r0
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	b SystemInit
#else
#error "Unsupported architecture"
#endif
	/* Return occurs via SystemInit */
