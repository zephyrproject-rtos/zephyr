/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Let renesas_ra_reset_hook() be called in place of soc_reset_hook() by default. */
PROVIDE(soc_reset_hook = renesas_ra_reset_hook);
