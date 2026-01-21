/* linker.ld - Linker command/script file */

/*
 * Copyright (C) 2025 Savoir-faire Linux, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define rom_start .isr_vectors

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
