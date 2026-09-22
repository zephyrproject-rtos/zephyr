/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2019 Intel Corp.
 */

#include <zephyr/kernel.h>
#include "info.h"

__weak void multiboot(void)
{
	printk("MULTIBOOT: Not supported in this build.\n\n");
}

__weak void memmap(void)
{
	printk("MEMMAP: Not supported in this build.\n\n");
}

__weak void acpi(void)
{
	printk("ACPI: Not supported in this build.\n\n");
}

int main(void)
{
	printk("\n\ninfo: the Zephyr x86 platform information tool\n\n");

	multiboot();
	memmap();
	acpi();
	timer();

	printk("info: complete\n");
	return 0;
}
