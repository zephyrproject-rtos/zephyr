/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_ARM)
#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>

/* Let SystemInit() be called in place of z_arm_platform_init() by default. */
PROVIDE(z_arm_platform_init = SystemInit);

#elif defined(CONFIG_RISCV)
#include <zephyr/arch/riscv/common/linker.ld>

#else
#error Unsupported architecture

#endif
