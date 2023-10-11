/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2019 Intel Corp.
 */

#include <zephyr/kernel.h>
#include <zephyr/acpi/acpi.h>

static const uint32_t dmar_scope[] = {ACPI_DMAR_SCOPE_TYPE_ENDPOINT, ACPI_DMAR_SCOPE_TYPE_BRIDGE,
				      ACPI_DMAR_SCOPE_TYPE_IOAPIC, ACPI_DMAR_SCOPE_TYPE_HPET,
				      ACPI_DMAR_SCOPE_TYPE_NAMESPACE};

static void vtd_dev_scope_info(int type, struct acpi_dmar_device_scope *dev_scope,
			       union acpi_dmar_id *dmar_id, int num_inst)
{
	int i = 0;

	printk("\t\t\t. Type: ");

	switch (type) {
	case ACPI_DMAR_SCOPE_TYPE_ENDPOINT:
		printk("PCI Endpoint");
		break;
	case ACPI_DMAR_SCOPE_TYPE_BRIDGE:
		printk("PCI Sub-hierarchy");
		break;
	case ACPI_DMAR_SCOPE_TYPE_IOAPIC:

		break;
	case ACPI_DMAR_SCOPE_TYPE_HPET:
		printk("MSI Capable HPET");
		break;
	case ACPI_DMAR_SCOPE_TYPE_NAMESPACE:
		printk("ACPI name-space enumerated");
		break;
	default:
		printk("unknown\n");
		return;
	}

	printk("\n");

	printk("\t\t\t. Enumeration ID %u\n", dev_scope->EnumerationId);
	printk("\t\t\t. PCI Bus %u\n", dev_scope->Bus);

	for (; num_inst > 0; num_inst--, i++) {
		printk("Info: Bus: %d, dev:%d, fun:%d\n", dmar_id[i].bits.bus,
			dmar_id[i].bits.device, dmar_id[i].bits.function);
	}

	printk("\n");
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
	for (i = 0; i < 5; i++) {
		if (acpi_drhd_get(dmar_scope[i], &dev_scope, dmar_id, &num_inst, 4u)) {
			printk(" No DRHD entry found for scope type:%d\n", dmar_scope[i]);
			continue;
		}

		printk("Found DRHD entry: %d\n", i);

		vtd_dev_scope_info(dmar_scope[i], &dev_scope, dmar_id, num_inst);
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
			printk("error in retrieve DHRD!!\n");
			return;
		}
		vtd_drhd_info(drhd);
	} else {
		printk("\t-> Interrupt remapping not supported\n");
	}
}

void acpi(void)
{
	int nr_cpus = 0, i, inst_cnt;
	struct acpi_madt_local_apic *lapic;

	if (acpi_madt_entry_get(ACPI_MADT_TYPE_LOCAL_APIC, (ACPI_SUBTABLE_HEADER **)&lapic,
				&inst_cnt)) {
		printk("error on get MAD table\n");
		return;
	}

	/* count number of CPUs present which are enabled*/
	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		if (lapic[i].LapicFlags & 1u) {
			nr_cpus++;
		}
	}

	if (nr_cpus == 0) {
		printk("ACPI: no RSDT/MADT found\n\n");
	} else {
		printk("ACPI: %d CPUs found\n", nr_cpus);

		for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; ++i) {
			printk("\tCPU #%d: APIC ID 0x%02x\n", i, lapic[i].Id);
		}
	}

	printk("\n");

	vtd_info();

	printk("\n");
}
