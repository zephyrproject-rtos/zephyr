/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_ACPI_H
#define ZEPHYR_INCLUDE_ARCH_X86_ACPI_H

#ifndef _ASMLANGUAGE

extern void z_x86_acpi_init(void);
extern struct x86_acpi_cpu *z_x86_acpi_get_cpu(int);

/*
 * The RSDP is a "floating" structure which simply points to the RSDT.
 */

#define X86_ACPI_RSDP_SIGNATURE	0x2052545020445352 /* 'RSD PTR ' */

struct x86_acpi_rsdp {
	u64_t signature;	/* X86_ACPI_RSDP_SIG */
	u8_t checksum;
	u8_t dontcare[7];
	u32_t rsdt;		/* physical address of RSDT */
} __packed;

/*
 * All SDTs have a common header.
 */

struct x86_acpi_sdt {
	u32_t signature;
	u32_t length;		/* in bytes, of entire table */
	u8_t dontcare0;
	u8_t checksum;		/* of entire table */
	u8_t dontcare1[26];
} __packed;

/*
 * The RSDT is the container for an array of pointers to other SDTs.
 */

#define X86_ACPI_RSDT_SIGNATURE 0x54445352	/* 'RSDT' */

struct x86_acpi_rsdt {
	struct x86_acpi_sdt sdt;
	u32_t sdts[];  /* physical addresses of other SDTs */
} __packed;

/*
 * The MADT is the "Multiple APIC Descriptor Table", which
 * we use to enumerate the CPUs and I/O APICs in the system.
 */

#define X86_ACPI_MADT_SIGNATURE 0x43495041	/* 'APIC' */

struct x86_acpi_madt_entry {
	u8_t type;
	u8_t length;
} __packed;

struct x86_acpi_madt {
	struct x86_acpi_sdt sdt;
	u32_t lapic;	/* local APIC MMIO address */
	u32_t flags;	/* see X86_ACPI_MADT_FLAGS_* below */
	struct x86_acpi_madt_entry entries[];
} __packed;

#define X86_ACPI_MADT_FLAGS_PICS 0x01	/* legacy 8259s installed */

/*
 * There's an MADT "local APIC" entry for each CPU in the system.
 */

#define X86_ACPI_MADT_ENTRY_CPU	0

struct x86_acpi_cpu {
	struct x86_acpi_madt_entry entry;
	u8_t dontcare;
	u8_t id;	/* local APIC ID */
	u8_t flags;	/* see X86_ACPI_MADT_CPU_FLAGS_* */
} __packed;

#define X86_ACPI_CPU_FLAGS_ENABLED 0x01

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_ACPI_H */
