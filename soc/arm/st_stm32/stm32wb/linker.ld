/* linker.ld - Linker command/script file */

/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define RAM1_SIZE (10 * 1K)
#define RAM1_ADDR 0x20030000
#define RAM2_SIZE (20 * 1K)
#define RAM2_ADDR 0x20038000

#include <arch/arm/cortex_m/scripts/linker.ld>
