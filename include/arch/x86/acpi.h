/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_ACPI_H
#define ZEPHYR_INCLUDE_ARCH_X86_ACPI_H

#ifndef _ASMLANGUAGE

extern void z_acpi_init(void);
extern struct acpi_cpu *z_acpi_get_cpu(int);

/*
 * The RSDP is a "floating" structure which simply points to the RSDT.
 */

#define ACPI_RSDP_SIGNATURE	0x2052545020445352 /* 'RSD PTR ' */

struct acpi_rsdp {
	u64_t signature;	/* ACPI_RSDP_SIG */
	u8_t checksum;
	u8_t dontcare[7];
	u32_t rsdt;		/* physical address of RSDT */
} __packed;

/*
 * All SDTs have a common header.
 */

struct acpi_sdt {
	u32_t signature;
	u32_t length;		/* in bytes, of entire table */
	u8_t dontcare0;
	u8_t checksum;		/* of entire table */
	u8_t dontcare1[26];
} __packed;

/*
 * The RSDT is the container for an array of pointers to other SDTs.
 */

#define ACPI_RSDT_SIGNATURE 0x54445352	/* 'RSDT' */

struct acpi_rsdt {
	struct acpi_sdt sdt;
	u32_t sdts[];  /* physical addresses of other SDTs */
} __packed;

/*
 * The MADT is the "Multiple APIC Descriptor Table", which
 * we use to enumerate the CPUs and I/O APICs in the system.
 */

#define ACPI_MADT_SIGNATURE 0x43495041	/* 'APIC' */

struct acpi_madt_entry {
	u8_t type;
	u8_t length;
} __packed;

struct acpi_madt {
	struct acpi_sdt sdt;
	u32_t lapic;	/* local APIC MMIO address */
	u32_t flags;	/* see ACPI_MADT_FLAGS_* below */
	struct acpi_madt_entry entries[];
} __packed;

#define ACPI_MADT_FLAGS_PICS 0x01	/* legacy 8259s installed */

/*
 * There's an MADT "local APIC" entry for each CPU in the system.
 */

#define ACPI_MADT_ENTRY_CPU	0

struct acpi_cpu {
	struct acpi_madt_entry entry;
	u8_t dontcare;
	u8_t id;	/* local APIC ID */
	u8_t flags;	/* see ACPI_MADT_CPU_FLAGS_* */
} __packed;

#define ACPI_CPU_FLAGS_ENABLED 0x01

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_ACPI_H */
