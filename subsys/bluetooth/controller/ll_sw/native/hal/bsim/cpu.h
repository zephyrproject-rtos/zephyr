/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

static inline void cpu_dmb(void)
{
	compiler_barrier();
}
