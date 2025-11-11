/*
 * Copyright (c) 2025 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/acpi/acpi.h>

void z_sys_poweroff(void)
{
#if defined(CONFIG_ACPI_POWEROFF)
	acpi_poweroff();
#endif
	CODE_UNREACHABLE;
}
