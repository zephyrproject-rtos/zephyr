/*
 * Copyright (c) 2025, Baumer (www.baumer.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Make CONFIG_KERNEL_ENTRY a compiler symbol accessible
 * with in code files.
 */
PROVIDE(k_entry = CONFIG_KERNEL_ENTRY);
