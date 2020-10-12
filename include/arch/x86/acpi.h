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

/* Generic DMA Remapping entry structure part */
struct acpi_dmar_entry {
	uint16_t type; /* See ACPI_DMAR_TYPE_* below */
	uint16_t length;
} __packed;

#define ACPI_DMAR_TYPE_DRHD 0 /* DMA Remapping Hardware Unit Definition */
#define ACPI_DMAR_TYPE_RMRR 1 /* Do not care atm (legacy usage) */
#define ACPI_DMAR_TYPE_ATSR 2 /* Do not care atm (PCIE ATS support) */
#define ACPI_DMAR_TYPE_RHSA 3 /* Do not care atm (NUMA specific ) */
#define ACPI_DMAR_TYPE_ANDD 4 /* Do not care atm (ACPI DSDT related) */
#define ACPI_DMAR_TYPE_SACT 5 /* Do not care atm */

/* PCI Device/Function Pair (forming the BDF, with start_bus_num below) */
struct acpi_dmar_dev_path {
	uint8_t device;
	uint8_t function;
} __packed;

#define ACPI_DMAR_DEV_PATH_SIZE			2

/* DMA Remapping Device Scope */
struct acpi_dmar_dev_scope {
	uint8_t type; /* See ACPI_DRHD_DEV_SCOPE_* below */
	uint8_t length; /* 6 + X where X is Path attribute size */
	uint16_t _reserved;
	uint8_t enumeration_id;
	uint8_t start_bus_num; /* PCI bus, forming BDF with each Path below */
	struct acpi_dmar_dev_path path[]; /* One is at least always found */
} __packed;

#define ACPI_DMAR_DEV_SCOPE_MIN_SIZE		6

#define ACPI_DRHD_DEV_SCOPE_PCI_EPD		0x01
#define ACPI_DRHD_DEV_SCOPE_PCI_SUB_H		0x02
#define ACPI_DRHD_DEV_SCOPE_IOAPIC		0x03
#define ACPI_DRHD_DEV_SCOPE_MSI_CAP_HPET	0x04
#define ACPI_DRHD_DEV_SCOPE_NAMESPACE_DEV	0x05

struct acpi_drhd {
	struct acpi_dmar_entry entry;
	uint8_t flags; /* See ACPI_DRHD_FLAG_* below */
	uint8_t _reserved;
	uint16_t segment_num; /* Associated PCI segment */
	uint64_t base_address; /* Base address of the remapping hw */
	struct acpi_dmar_dev_scope device_scope[];
} __packed;

#define ACPI_DRHD_MIN_SIZE			16

#define ACPI_DRHD_FLAG_INCLUDE_PCI_ALL		BIT(0)

#define ACPI_DMAR_SIGNATURE 0x52414D44  /* 'DMAR' */

#define ACPI_DMAR_FLAG_INTR_REMAP		BIT(0)
#define ACPI_DMAR_FLAG_X2APIC_OPT_OUT		BIT(1)
#define ACPI_DMAR_FLAG_DMA_CTRL_PLATFORM_OPT_IN	BIT(2)

/* DMA Remapping reporting structure */
struct acpi_dmar {
	struct acpi_sdt sdt;
	uint8_t host_addr_width;
	uint8_t flags;
	uint8_t _reserved[10];
	struct acpi_dmar_entry remap_entries[];
} __packed;

#if defined(CONFIG_ACPI)

void *z_acpi_find_table(uint32_t signature);

struct acpi_cpu *z_acpi_get_cpu(int n);

struct acpi_dmar *z_acpi_find_dmar(void);

struct acpi_drhd *z_acpi_find_drhds(int *n);

struct acpi_dmar_dev_scope *z_acpi_get_drhd_dev_scopes(struct acpi_drhd *drhd,
						       int *n);

struct acpi_dmar_dev_path *
z_acpi_get_dev_scope_paths(struct acpi_dmar_dev_scope *dev_scope, int *n);

#else /* CONFIG_ACPI */

#define z_acpi_find_table(...) NULL
#define z_acpi_get_cpu(...) NULL
#define z_acpi_find_dmar(...) NULL
#define z_acpi_find_drhds(...) NULL
#define z_acpi_get_drhd_dev_scopes(...) NULL
#define z_acpi_get_dev_scope_paths(...) NULL

#endif /* CONFIG_ACPI */

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_X86_ACPI_H */
