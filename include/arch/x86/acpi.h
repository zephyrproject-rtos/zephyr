/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_ACPI_H
#define ZEPHYR_INCLUDE_ARCH_X86_ACPI_H

#ifndef _ASMLANGUAGE

extern void *z_acpi_find_table(uint32_t signature);

/* Standard table header */
struct acpi_sdt {
	uint32_t sig;
	uint32_t len;
	uint8_t rev;
	uint8_t csum;
	char oemid[6];
	char oem_table_id[8];
	uint32_t oemrevision;
	uint32_t creator_id;
	uint32_t creator_rev;
} __packed;

/* MCFG table storing MMIO addresses for PCI configuration space */

#define ACPI_MCFG_SIGNATURE 0x4746434d  /* 'MCFG' */

struct acpi_mcfg {
	struct acpi_sdt sdt;
	uint64_t _rsvd;
	struct {
		uint64_t base_addr;
		uint16_t seg_group_num;
		uint8_t start_bus;
		uint8_t end_bus;
	} pci_segs[];
} __packed;

/* MADT table storing IO-APIC and multiprocessor configuration  */

#define ACPI_MADT_SIGNATURE 0x43495041	/* 'APIC' */

#define ACPI_MADT_FLAGS_PICS 0x01	/* legacy 8259s installed */
#define ACPI_MADT_ENTRY_CPU	0

struct acpi_madt_entry {
	uint8_t type;
	uint8_t length;
} __packed;

struct acpi_madt {
	struct acpi_sdt sdt;
	uint32_t lapic; /* local APIC MMIO address */
	uint32_t flags; /* see ACPI_MADT_FLAGS_* below */
	struct acpi_madt_entry entries[];
} __packed;

struct acpi_cpu {
	struct acpi_madt_entry entry;
	uint8_t dontcare;
	uint8_t id;	/* local APIC ID */
	uint8_t flags;	/* see ACPI_MADT_CPU_FLAGS_* */
} __packed;

struct acpi_cpu *z_acpi_get_cpu(int n);

#define ACPI_CPU_FLAGS_ENABLED 0x01

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_ACPI_H */
