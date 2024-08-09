/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Let SystemInit() be called in place of platform_reset() by default. */
PROVIDE(platform_reset = SystemInit);
