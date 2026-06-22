/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Let SystemInit() be called in place of soc_reset_hook() by default. */
PROVIDE(soc_reset_hook = SystemInit);
