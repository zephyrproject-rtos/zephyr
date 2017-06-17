/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PCI probe and information routines
 *
 * Module implements routines for PCI bus initialization and query.
 *
 * USAGE
 * To use the driver, the platform must define:
 * - Numbers of BUSes:
 *     - PCI_BUS_NUMBERS;
 * - Register addresses:
 *     - PCI_CTRL_ADDR_REG;
 *     - PCI_CTRL_DATA_REG;
 * - pci_pin2irq() - the routine that converts the PCI interrupt pin
 *   number to IRQ number.
 *
 * About scanning the PCI buses:
 * At every new usage of this API, the code should call pci_bus_scan_init().
 * It should own a struct pci_dev_info, filled in with the parameters it is
 * interested to look for: class and/or vendor_id/device_id.
 *
 * Then it can loop on pci_bus_scan() providing a pointer on that structure.
 * Such function can be called as long as it returns 1. At every successful
 * return of pci_bus_scan() it means the provided structure pointer will have
 * been updated with the current scan result which the code might be interested
 * in. On pci_bus_scan() returning 0, the code should discard the result and
 * stop calling pci_bus_scan(). If it wants to retrieve the result, it will
 * have to restart the procedure all over again.
 *
 * EXAMPLE
 * struct pci_dev_info info = {
 *     .class_type = PCI_CLASS_COMM_CTLR
 * };
 *
 * pci_bus_scan_init();
 *
 * while (pci_bus_scan(&info)) {
 *      // do something with "info" which holds a valid result, i.e. some
 *      // device information matching the PCI class PCI_CLASS_COMM_CTLR
 * }
 *
 * INTERNALS
 * The whole logic runs around a structure: struct lookup_data, which exists
 * on one instantiation called 'lookup'.
 * Such structure is used for 2 distinct roles:
 * - to match devices the caller is looking for
 * - to loop on PCI bus, devices, function and BARs
 *
 * The search criteria are the class and/or the vendor_id/device_id of a PCI
 * device. The caller first initializes the lookup structure by calling
 * pci_bus_scan_init(), which will reset the search criteria as well as the
 * loop parameters to 0. At the very first subsequent call of pci_bus_scan()
 * the lookup structure will store the search criteria. Then the loop starts.
 * For each bus it will run through each device on which it will loop on each
 * function and BARs, as long as the criteria does not match or until it hit
 * the limit of bus/dev/functions to scan.
 *
 * On a successful match, it will stop the loop, fill in the caller's
 * pci_dev_info structure with the found device information, and return 1.
 * Hopefully, the lookup structure still remembers where it stopped and the
 * original search criteria. Thus, when the caller asks to scan again for
 * a possible result next, the loop will restart where it stopped.
 * That will work as long as there are relevant results found.
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <misc/printk.h>
#include <toolchain.h>
#include <linker/sections.h>

#include <board.h>

#include <pci/pci_mgr.h>
#include <pci/pci.h>

#ifdef CONFIG_PCI_ENUMERATION

/* NOTE. These parameters may need to be configurable */
#define LSPCI_MAX_BUS PCI_BUS_NUMBERS /* maximum number of buses to scan */
#define LSPCI_MAX_DEV 32  /* maximum number of devices to scan */
#define LSPCI_MAX_FUNC PCI_MAX_FUNCTIONS  /* maximum functions to scan */
#define LSPCI_MAX_REG 64  /* maximum device registers to read */

/* Base Address Register configuration fields */

#define BAR_SPACE(x) ((x) & 0x00000001)

#define BAR_TYPE(x) ((x) & 0x00000006)
#define BAR_TYPE_32BIT 0
#define BAR_TYPE_64BIT 4

#define BAR_PREFETCH(x) (((x) >> 3) & 0x00000001)
#define BAR_ADDR(x) (((x) >> 4) & 0x0fffffff)

#define BAR_IO_MASK(x) ((x) & ~0x3)
#define BAR_MEM_MASK(x) ((x) & ~0xf)


struct lookup_data {
	struct pci_dev_info info;
	u32_t bus:9;
	u32_t dev:6;
	u32_t func:4;
	u32_t baridx:3;
	u32_t barofs:3;
	u32_t unused:7;
};

static struct lookup_data __noinit lookup;

/**
 *
 * @brief Return the configuration for the specified BAR
 *
 * @return 0 if BAR is implemented, -1 if not.
 */

static inline int pci_bar_config_get(union pci_addr_reg pci_ctrl_addr,
							u32_t *config)
{
	u32_t old_value;

	/* save the current setting */
	pci_read(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(old_value),
			&old_value);

	/* write to the BAR to see how large it is */
	pci_write(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(u32_t),
			0xffffffff);

	pci_read(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(*config),
			config);

	/* put back the old configuration */
	pci_write(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(old_value),
			old_value);

	/* check if this BAR is implemented */
	if (*config != 0xffffffff && *config != 0) {
		return 0;
	}

	/* BAR not supported */

	return -1;
}

/**
 *
 * @brief Retrieve the I/O address and IRQ of the specified BAR
 *
 * @return -1 on error, 0 if 32 bit BAR retrieved or 1 if 64 bit BAR retrieved
 *
 * NOTE: Routine does not set up parameters for 64 bit BARS, they are ignored.
 */
static inline int pci_bar_params_get(union pci_addr_reg pci_ctrl_addr,
				     struct pci_dev_info *dev_info,
				     int max_bars)
{
	u32_t bar_value;
	u32_t bar_config;
	u32_t bar_hival;
	u32_t addr;
	u32_t mask;

	pci_ctrl_addr.field.reg = 4 + lookup.barofs;

	pci_read(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(bar_value),
			&bar_value);
	if (pci_bar_config_get(pci_ctrl_addr, &bar_config) != 0) {
		return -1;
	}

	if (BAR_SPACE(bar_config) == BAR_SPACE_MEM) {
		dev_info->mem_type = BAR_SPACE_MEM;
		mask = ~0xf;
		if (BAR_TYPE(bar_config) == BAR_TYPE_64BIT) {
			/* Last BAR register cannot be 64-bit */
			if (++lookup.barofs >= max_bars)
				return 1;

			/* Make sure the address is accessible */
			pci_ctrl_addr.field.reg++;
			pci_read(DEFAULT_PCI_CONTROLLER,
				 pci_ctrl_addr,
				 sizeof(bar_hival),
				 &bar_hival);
			if (bar_hival)
				return 1; /* Inaccessible memory */
		}
	} else {
		dev_info->mem_type = BAR_SPACE_IO;
		mask = ~0x3;
	}

	dev_info->addr = bar_value & mask;

	addr = bar_config & mask;
	if (addr != 0) {
		/* calculate the size of the BAR memory required */
		dev_info->size = 1 << (find_lsb_set(addr) - 1);
	}

	return 0;
}

/**
 *
 * @brief Scan the specified PCI device for all sub functions
 *
 * @return 1 if a device has been found, 0 otherwise.
 */
static inline int pci_dev_scan(union pci_addr_reg pci_ctrl_addr,
					struct pci_dev_info *dev_info)
{
	static union pci_dev pci_dev_header;
	u32_t pci_data;
	int max_bars;

	/* verify first if there is a valid device at this point */
	pci_ctrl_addr.field.func = 0;

	pci_read(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(pci_data),
			&pci_data);

	if (pci_data == 0xffffffff) {
		return 0;
	}

	/* scan all the possible functions for this device */
	for (; lookup.func < LSPCI_MAX_FUNC;
	     lookup.baridx = 0, lookup.barofs = 0, lookup.func++) {
		if (lookup.info.function != PCI_FUNCTION_ANY &&
		    lookup.func != lookup.info.function) {
			return 0;
		}

		pci_ctrl_addr.field.func = lookup.func;

		if (lookup.func != 0) {
			pci_read(DEFAULT_PCI_CONTROLLER,
					pci_ctrl_addr,
					sizeof(pci_data),
					&pci_data);

			if (pci_data == 0xffffffff) {
				continue;
			}
		}

		/* get the PCI header from the device */
		pci_header_get(DEFAULT_PCI_CONTROLLER,
					pci_ctrl_addr,
					&pci_dev_header);

		/*
		 * Skip a device if its class is specified by the
		 * caller and does not match
		 */
		if (lookup.info.class_type &&
		    pci_dev_header.field.class != lookup.info.class_type) {
			continue;
		}

		if (lookup.info.vendor_id && lookup.info.device_id &&
		    (lookup.info.vendor_id != pci_dev_header.field.vendor_id ||
		    lookup.info.device_id != pci_dev_header.field.device_id)) {
			continue;
		}

		/* Get memory and interrupt information */
		if ((pci_dev_header.field.hdr_type & 0x7f) == 1) {
			max_bars = 2;
		} else {
			max_bars = PCI_MAX_BARS;
		}

		for (; lookup.barofs < max_bars;
		     lookup.baridx++, lookup.barofs++) {
			/* Ignore BARs with errors */
			if (pci_bar_params_get(pci_ctrl_addr, dev_info,
					       max_bars) != 0) {
				continue;
			} else if (lookup.info.bar != PCI_BAR_ANY &&
				   lookup.baridx != lookup.info.bar) {
				continue;
			} else {
				dev_info->bus = lookup.bus;
				dev_info->dev = lookup.dev;
				dev_info->vendor_id =
					pci_dev_header.field.vendor_id;
				dev_info->device_id =
					pci_dev_header.field.device_id;
				dev_info->class_type =
					pci_dev_header.field.class;
				dev_info->irq = pci_pin2irq(dev_info->bus,
					dev_info->dev,
					pci_dev_header.field.interrupt_pin);
				dev_info->function = lookup.func;
				dev_info->bar = lookup.baridx;

				lookup.baridx++;
				lookup.barofs++;
				if (lookup.barofs >= max_bars) {
					lookup.baridx = 0;
					lookup.barofs = 0;
				}

				return 1;
			}
		}
	}

	return 0;
}

void pci_bus_scan_init(void)
{
	lookup.info.class_type = 0;
	lookup.info.vendor_id = 0;
	lookup.info.device_id = 0;
	lookup.info.function = PCI_FUNCTION_ANY;
	lookup.info.bar = PCI_BAR_ANY;
	lookup.bus = 0;
	lookup.dev = 0;
	lookup.func = 0;
	lookup.baridx = 0;
	lookup.barofs = 0;
}


/**
 *
 * @brief Scans PCI bus for devices
 *
 * The routine scans the PCI bus for the devices on criteria provided in the
 * given dev_info at first call. Which criteria can be class and/or
 * vendor_id/device_id.
 *
 * @return 1 on success, 0 otherwise. On success, dev_info is filled in with
 * currently found device information
 */
int pci_bus_scan(struct pci_dev_info *dev_info)
{
	union pci_addr_reg pci_ctrl_addr;

	int init_from_dev_info =
		!lookup.info.class_type &&
		!lookup.info.vendor_id &&
		!lookup.info.device_id &&
		lookup.info.bar == PCI_BAR_ANY &&
		lookup.info.function == PCI_FUNCTION_ANY;

	if (init_from_dev_info) {
		lookup.info.class_type = dev_info->class_type;
		lookup.info.vendor_id = dev_info->vendor_id;
		lookup.info.device_id = dev_info->device_id;
		lookup.info.function = dev_info->function;
		lookup.info.bar = dev_info->bar;
	}

	/* initialise the PCI controller address register value */
	pci_ctrl_addr.value = 0;

	if (lookup.info.function != PCI_FUNCTION_ANY) {
		lookup.func = lookup.info.function;
	}

	/* run through the buses and devices */
	for (; lookup.bus < LSPCI_MAX_BUS; lookup.bus++) {
		for (; (lookup.dev < LSPCI_MAX_DEV); lookup.dev++) {
			pci_ctrl_addr.field.bus = lookup.bus;
			pci_ctrl_addr.field.device = lookup.dev;

			if (pci_dev_scan(pci_ctrl_addr, dev_info)) {
				return 1;
			}

			if (lookup.info.function != PCI_FUNCTION_ANY) {
				lookup.func = lookup.info.function;
			} else {
				lookup.func = 0;
			}
		}
		lookup.dev = 0;
	}

	return 0;
}
#endif /* CONFIG_PCI_ENUMERATION */

static void pci_set_command_bits(struct pci_dev_info *dev_info, u32_t bits)
{
	union pci_addr_reg pci_ctrl_addr;
	u32_t pci_data;

	pci_ctrl_addr.value = 0;
	pci_ctrl_addr.field.func = dev_info->function;
	pci_ctrl_addr.field.bus = dev_info->bus;
	pci_ctrl_addr.field.device = dev_info->dev;
	pci_ctrl_addr.field.reg = 1;

#ifdef CONFIG_PCI_DEBUG
	printk("pci_set_command_bits 0x%x\n", pci_ctrl_addr.value);
#endif

	pci_read(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(u16_t),
			&pci_data);

	pci_data = pci_data | bits;

	pci_write(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(u16_t),
			pci_data);
}

void pci_enable_regs(struct pci_dev_info *dev_info)
{
	pci_set_command_bits(dev_info, PCI_CMD_MEM_ENABLE);
}

void pci_enable_bus_master(struct pci_dev_info *dev_info)
{
	pci_set_command_bits(dev_info, PCI_CMD_MASTER_ENABLE);
}

#ifdef CONFIG_PCI_DEBUG
/**
 *
 * @brief Show PCI device
 *
 * Shows the PCI device found provided as parameter.
 *
 * @return N/A
 */

void pci_show(struct pci_dev_info *dev_info)
{
	printk("PCI device:\n");
	printk("%u:%u %X:%X class: 0x%X, %u, %u, %s,"
		"addrs: 0x%X-0x%X, IRQ %d\n",
		dev_info->bus,
		dev_info->dev,
		dev_info->vendor_id,
		dev_info->device_id,
		dev_info->class_type,
		dev_info->function,
		dev_info->bar,
		(dev_info->mem_type == BAR_SPACE_MEM) ? "MEM" : "I/O",
		(u32_t)dev_info->addr,
		(u32_t)(dev_info->addr + dev_info->size - 1),
		dev_info->irq);
}
#endif /* CONFIG_PCI_DEBUG */
