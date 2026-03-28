/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2019 Intel Corp.
 */

#include <zephyr/kernel.h>
#include <zephyr/acpi/acpi.h>

static const char *get_dmar_scope_type(int type)
{
	switch (type) {
	case ACPI_DMAR_SCOPE_TYPE_ENDPOINT:
		return "PCI Endpoint";
	case ACPI_DMAR_SCOPE_TYPE_BRIDGE:
		return "PCI Sub-hierarchy";
	case ACPI_DMAR_SCOPE_TYPE_IOAPIC:
		return "IOAPIC";
	case ACPI_DMAR_SCOPE_TYPE_HPET:
		return "MSI Capable HPET";
	case ACPI_DMAR_SCOPE_TYPE_NAMESPACE:
		return "ACPI name-space enumerated";
	default:
		return "unknown";
	}
}

static void dmar_devsope_handler(ACPI_DMAR_DEVICE_SCOPE *devscope, void *arg)
{
	ARG_UNUSED(arg);

	printk("\t\t\t. Scope type %s\n", get_dmar_scope_type(devscope->EntryType));
	printk("\t\t\t. Enumeration ID %u\n", devscope->EnumerationId);

	if (devscope->EntryType < ACPI_DMAR_SCOPE_TYPE_RESERVED) {
		ACPI_DMAR_PCI_PATH *devpath;
		int num_path = (devscope->Length - 6u) / 2u;
		int i = 0;

		devpath = ACPI_ADD_PTR(ACPI_DMAR_PCI_PATH, devscope,
				       sizeof(ACPI_DMAR_DEVICE_SCOPE));

		while (num_path--) {
			printk("\t\t\t. PCI Path %02x:%02x.%02x\n", devscope->Bus,
			       devpath[i].Device, devpath[i].Function);
		}
	}
}

static void vtd_drhd_info(ACPI_DMAR_HEADER *subtable)
{
	struct acpi_dmar_hardware_unit *drhd = (void *)subtable;
	static int unit;

	printk("\t\t[ Hardware Unit Definition %d ]\n", unit++);

	if (drhd->Flags & ACPI_DRHD_FLAG_INCLUDE_PCI_ALL) {
		printk("\t\t- Includes all PCI devices");
	} else {
		printk("\t\t- Includes only listed PCI devices");
	}

	printk(" under given Segment\n");

	printk("\t\t- Segment number %u\n", drhd->Segment);
	printk("\t\t- Base Address 0x%llx\n", drhd->Address);

	printk("\t\t- Device Scopes:\n");

	acpi_dmar_foreach_devscope(drhd, dmar_devsope_handler, NULL);
}

static void dmar_subtable_handler(ACPI_DMAR_HEADER *subtable, void *arg)
{
	ARG_UNUSED(arg);

	if (subtable->Type != ACPI_DMAR_TYPE_HARDWARE_UNIT) {
		return;
	}

	vtd_drhd_info(subtable);
}

static void vtd_info(void)
{
	struct acpi_table_dmar *dmar;

	dmar = acpi_table_get("DMAR", 0);
	if (dmar == NULL) {
		printk("\tIntel VT-D not supported or exposed\n");
		return;
	}

	printk("\tIntel VT-D Supported:\n");

	printk("\t-> X2APIC ");
	if (dmar->Flags & ACPI_DMAR_FLAG_X2APIC_OPT_OUT) {
		printk("should be opted out\n");
	} else {
		printk("does not need to be opted out\n");
	}

	if (dmar->Flags & ACPI_DMAR_FLAG_INTR_REMAP) {
		printk("\t-> Interrupt remapping supported\n");
		acpi_dmar_foreach_subtable(dmar, dmar_subtable_handler, NULL);
	} else {
		printk("\t-> Interrupt remapping not supported\n");
	}
}

void acpi(void)
{
	int nr_cpus;

	for (nr_cpus = 0; acpi_local_apic_get(nr_cpus); ++nr_cpus) {
		/* count number of CPUs present */
	}

	if (nr_cpus == 0) {
		printk("ACPI: no RSDT/MADT found\n\n");
	} else {
		printk("ACPI: %d CPU%s found\n", nr_cpus, nr_cpus == 1 ? "" : "s");

		for (int i = 0; i < nr_cpus; ++i) {
			struct acpi_madt_local_apic *cpu = acpi_local_apic_get(i);

			printk("\tCPU #%d: APIC ID 0x%02x\n", i, cpu->Id);
		}
	}

	printk("\n");

	vtd_info();

	printk("\n");
}
