/*
 * Copyright (c) 2020 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_ACPI_H
#define ZEPHYR_INCLUDE_ARCH_X86_ACPI_H

#ifndef _ASMLANGUAGE

#define ACPI_RSDP_SIGNATURE 0x2052545020445352 /* == "RSD PTR " */

/* Root System Description Pointer */
struct acpi_rsdp {
	char     signature[8];
	uint8_t  chksum;
	char     oem_id[6];
	uint8_t  revision;
	uint32_t rsdt_ptr;
	uint32_t length;
	uint64_t xsdt_ptr;
	uint8_t  ext_chksum;
	uint8_t  _reserved[3];
} __packed;

/* Standard table header */
struct acpi_sdt {
	uint32_t signature;
	uint32_t length;
	uint8_t revision;
	uint8_t chksum;
	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} __packed;

/* Root System Description Table */
struct acpi_rsdt {
	struct acpi_sdt sdt;
	uint32_t table_ptrs[];
} __packed;

/* eXtended System Descriptor Table */
struct acpi_xsdt {
	struct acpi_sdt sdt;
	uint64_t table_ptrs[];
} __packed;

/* MCFG table storing MMIO addresses for PCI configuration space */

#define ACPI_MCFG_SIGNATURE 0x4746434d  /* 'MCFG' */

struct acpi_mcfg {
	struct acpi_sdt sdt;
	uint64_t _reserved;
	struct {
		uint64_t base_addr;
		uint16_t seg_group_num;
		uint8_t start_bus;
		uint8_t end_bus;
	} pci_segs[];
} __packed;

/* MADT table storing IO-APIC and multiprocessor configuration  */

#define ACPI_MADT_SIGNATURE 0x43495041	/* 'APIC' */

struct acpi_madt_entry {
	uint8_t type;	/* See ACPI_MADT_ENTRY_* below */
	uint8_t length;
} __packed;

#define ACPI_MADT_ENTRY_CPU 0

struct acpi_madt {
	struct acpi_sdt sdt;
	uint32_t loapic; /* local APIC MMIO address */
	uint32_t flags; /* see ACPI_MADT_FLAGS_* below */
	struct acpi_madt_entry entries[];
} __packed;

#define ACPI_MADT_FLAGS_PICS 0x01	/* legacy 8259s installed */

struct acpi_cpu {
	struct acpi_madt_entry entry;
	uint8_t acpi_id;
	uint8_t apic_id; /* local APIC ID */
	uint8_t flags;   /* see ACPI_CPU_FLAGS_* below */
} __packed;

#define ACPI_CPU_FLAGS_ENABLED 0x01

#if defined(CONFIG_ACPI)

void *z_acpi_find_table(uint32_t signature);

struct acpi_cpu *z_acpi_get_cpu(int n);

#else /* CONFIG_ACPI */

#define z_acpi_find_table(...) NULL
#define z_acpi_get_cpu(...) NULL

#endif /* CONFIG_ACPI */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_ACPI_H */
