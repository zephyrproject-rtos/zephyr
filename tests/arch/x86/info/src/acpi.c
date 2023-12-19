/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2019 Intel Corp.
 */

#include <zephyr/kernel.h>
#include <zephyr/acpi/acpi.h>

static const uint32_t dmar_scope[] = {ACPI_DMAR_SCOPE_TYPE_ENDPOINT, ACPI_DMAR_SCOPE_TYPE_BRIDGE,
				      ACPI_DMAR_SCOPE_TYPE_IOAPIC, ACPI_DMAR_SCOPE_TYPE_HPET,
				      ACPI_DMAR_SCOPE_TYPE_NAMESPACE};

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

static void vtd_dev_scope_info(struct acpi_dmar_device_scope *dev_scope,
			       union acpi_dmar_id *dmar_id, int num_inst)
{
	int i = 0;

	printk("\t\t\t. Enumeration ID %u\n", dev_scope->EnumerationId);

	for (; num_inst > 0; num_inst--, i++) {
		printk("\t\t\t. BDF 0x%x:0x%x:0x%x\n",
		       dmar_id[i].bits.bus, dmar_id[i].bits.device,
		       dmar_id[i].bits.function);
	}
}

static void vtd_drhd_info(struct acpi_dmar_hardware_unit *drhd)
{
	struct acpi_dmar_device_scope dev_scope;
	union acpi_dmar_id dmar_id[4];
	int num_inst, i;

	if (drhd->Flags & ACPI_DRHD_FLAG_INCLUDE_PCI_ALL) {
		printk("\t\t- Includes all PCI devices");
	} else {
		printk("\t\t- Includes only listed PCI devices");
	}

	printk(" under given Segment\n");

	printk("\t\t- Segment number %u\n", drhd->Segment);
	printk("\t\t- Base Address 0x%llx\n", drhd->Address);

	printk("\t\t- Device Scopes:\n");
	for (i = 0; i < ARRAY_SIZE(dmar_scope); i++) {
		if (acpi_drhd_get(dmar_scope[i], &dev_scope, dmar_id, &num_inst, 4u)) {
			printk("\t\tNo DRHD type: %s\n",
			       get_dmar_scope_type(dmar_scope[i]));
			continue;
		}

		printk("\t\tDRHD type %s\n", get_dmar_scope_type(dmar_scope[i]));

		vtd_dev_scope_info(&dev_scope, dmar_id, num_inst);
	}

	printk("\n");
}

static void vtd_info(void)
{
	struct acpi_table_dmar *dmar;
	struct acpi_dmar_hardware_unit *drhd;

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

		if (acpi_dmar_entry_get(ACPI_DMAR_TYPE_HARDWARE_UNIT,
					(struct acpi_subtable_header **)&drhd)) {
			printk("\tError in retrieving DHRD!!\n");
			return;
		}
		vtd_drhd_info(drhd);
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
