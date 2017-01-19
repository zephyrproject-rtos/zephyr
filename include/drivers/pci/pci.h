/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PCI probe and information routines
 *
 * Module declares routines of PCI bus initialization and query
 */

#ifndef _PCI_H_
#define _PCI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define BAR_SPACE_MEM 0
#define BAR_SPACE_IO 1

#define PCI_MAX_FUNCTIONS 8
#define PCI_FUNCTION_ANY PCI_MAX_FUNCTIONS

#define PCI_MAX_BARS 6
#define PCI_BAR_ANY PCI_MAX_BARS

/* PCI device information */

struct pci_dev_info {
	uint32_t addr; /* I/O or memory region address */
	uint32_t size; /* memory region size */
	int irq;

	uint32_t bus:8;
	uint32_t dev:5;
	uint32_t function:4;
	uint32_t mem_type:1; /* memory type: BAR_SPACE_MEM/BAR_SPACE_IO */
	uint32_t class_type:8;
	uint32_t bar:3;
	uint32_t _reserved:3;

	uint16_t vendor_id;
	uint16_t device_id;
};

#ifdef CONFIG_PCI_ENUMERATION
extern void pci_bus_scan_init(void);
extern int pci_bus_scan(struct pci_dev_info *dev_info);
#else
#define pci_bus_scan_init(void) { ; }
static inline int pci_bus_scan(struct pci_dev_info *dev_info)
{
	return 1;
}
#endif /* CONFIG_PCI_ENUMERATION */

void pci_enable_regs(struct pci_dev_info *dev_info);
void pci_enable_bus_master(struct pci_dev_info *dev_info);
int pci_legacy_bridge_detect(struct pci_dev_info *dev_info);
void pci_legacy_bridge_configure(struct pci_dev_info *dev_info,
				 int io_block_num,
				 int pci_interrupt_pin,
				 int irq_number);

#ifdef CONFIG_PCI_DEBUG
extern void pci_show(struct pci_dev_info *dev_info);
#else
#define pci_show(__unused__) { ; }
#endif

#ifdef __cplusplus
}
#endif

#endif /* _PCI_H_ */
