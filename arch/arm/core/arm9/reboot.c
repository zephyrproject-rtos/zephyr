/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/linker-defs.h>

/**
 *
 * @brief Reset the system
 *
 * This routine resets the processor.
 *
 */

void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
}
