/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2019 Intel Corp.
 */

#include <zephyr.h>
#include "x86_info.h"

__weak void multiboot(void)
{
	printk("MULTIBOOT: Not supported in this build.\n\n");
}

__weak void acpi(void)
{
	printk("ACPI: Not supported in this build.\n\n");
}

void main(void)
{
	printk("\n\nx86_info: the Zephyr x86 platform information tool\n\n");

	multiboot();
	memmap();
	acpi();
	timer();

	printk("x86_info: complete\n");
}
