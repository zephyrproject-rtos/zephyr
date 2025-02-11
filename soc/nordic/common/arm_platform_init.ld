/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Let SystemInit() be called in place of z_arm_platform_init() by default. */
PROVIDE(z_arm_platform_init = SystemInit);
