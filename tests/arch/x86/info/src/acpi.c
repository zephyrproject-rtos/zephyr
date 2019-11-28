/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2019 Intel Corp.
 */

#include <zephyr.h>
#include <arch/x86/acpi.h>

void acpi(void)
{
	int nr_cpus;

	for (nr_cpus = 0; z_acpi_get_cpu(nr_cpus); ++nr_cpus) {
		/* count number of CPUs present */
	}

	if (nr_cpus == 0) {
		printk("ACPI: no RSDT/MADT found\n\n");
	} else {
		printk("ACPI: %d CPUs found\n", nr_cpus);

		for (int i = 0; i < nr_cpus; ++i) {
			struct acpi_cpu *cpu = z_acpi_get_cpu(i);
			printk("\tCPU #%d: APIC ID 0x%02x\n", i, cpu->id);
		}
	}

	printk("\n");
}
