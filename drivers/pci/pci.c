/* pci.c - PCI probe and information routines */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
Module implements routines for PCI bus initialization and query.
Note that the BSP must call pci_bus_scan() before any other PCI
API is called.

USAGE
In order to use the driver, BSP has to define:
- Register addresses:
    - PCI_CTRL_ADDR_REG;
    - PCI_CTRL_DATA_REG;
- Register read/write routines:
    - PLB_LONG_REG_READ() / PLB_LONG_REG_WRITE();
    - PLB_WORD_REG_READ() / PLB_WORD_REG_WRITE();
    - PLB_BYTE_REG_READ() / PLB_BYTE_REG_WRITE();
- pci_pin2irq() - the routine that converts the PCI interrupt pin
  number to IRQ number.
*/

#include <nanokernel.h>
#include <nanokernel/cpu.h>
#include <misc/printk.h>
#include <toolchain.h>
#include <sections.h>

#include <board.h>

#include <pci/pci_mgr.h>
#include <pci/pci.h>

/* NOTE. These parameters may need to be configurable */
#define LSPCI_MAX_BUS 256 /* maximum number of buses to scan */
#define LSPCI_MAX_DEV 32  /* maximum number of devices to scan */
#define LSPCI_MAX_FUNC 8  /* maximum device functions to scan */
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

#define MAX_BARS 6

static struct pci_dev_info __noinit dev_info[CONFIG_MAX_PCI_DEVS];
static int dev_info_index = 0;

/*******************************************************************************
*
* pci_get_bar_config - return the configuration for the specified BAR
*
* RETURNS: 0 if BAR is implemented, -1 if not.
*/

static int pci_bar_config_get(uint32_t bus,
			   uint32_t dev,
			   uint32_t func,
			   uint32_t bar,
			   uint32_t *config)
{
	union pci_addr_reg pci_ctrl_addr;
	uint32_t old_value;

	pci_ctrl_addr.value = 0;
	pci_ctrl_addr.field.enable = 1;
	pci_ctrl_addr.field.bus = bus;
	pci_ctrl_addr.field.device = dev;
	pci_ctrl_addr.field.func = func;
	pci_ctrl_addr.field.reg = 4 + bar;

	/* save the current setting */

	pci_read(DEFAULT_PCI_CONTROLLER,
		pci_ctrl_addr,
		sizeof(old_value),
		&old_value);

	/* write to the BAR to see how large it is */
	pci_write(DEFAULT_PCI_CONTROLLER,
		 pci_ctrl_addr,
		 sizeof(uint32_t),
		 0xffffffff);
	pci_read(DEFAULT_PCI_CONTROLLER, pci_ctrl_addr, sizeof(*config), config);

	/* put back the old configuration */

	pci_write(DEFAULT_PCI_CONTROLLER,
		 pci_ctrl_addr,
		 sizeof(old_value),
		 old_value);

	/* check if this BAR is implemented */
	if (*config != 0xffffffff && *config != 0)
		return 0;

	/* BAR not supported */

	return -1;
}

/*******************************************************************************
 *
 * pci_bar_params_get - retrieve the I/O address and IRQ of the specified BAR
 *
 * RETURN: -1 on error, 0 if 32 bit BAR retrieved or 1 if 64 bit BAR retrieved
 *
 * NOTE: Routine does not set up parameters for 64 bit BARS, they are ignored.
 *
 * \NOMANUAL
 */

static inline int pci_bar_params_get(uint32_t bus,
				  uint32_t dev,
				  uint32_t func,
				  uint32_t bar,
				  struct pci_dev_info *dev_info)
{
	static union pci_addr_reg pci_ctrl_addr;
	uint32_t bar_value;
	uint32_t bar_config;
	uint32_t addr;
	uint32_t mask;

	pci_ctrl_addr.value = 0;
	pci_ctrl_addr.field.enable = 1;
	pci_ctrl_addr.field.bus = bus;
	pci_ctrl_addr.field.device = dev;
	pci_ctrl_addr.field.func = func;
	pci_ctrl_addr.field.reg = 4 + bar;

	pci_read(DEFAULT_PCI_CONTROLLER,
		pci_ctrl_addr,
		sizeof(bar_value),
		&bar_value);
	if (pci_bar_config_get(bus, dev, func, bar, &bar_config) != 0)
		return -1;

	if (BAR_SPACE(bar_config) == BAR_SPACE_MEM) {
		dev_info->mem_type = BAR_SPACE_MEM;
		mask = ~0xf;
		if (bar < 5 && BAR_TYPE(bar_config) == BAR_TYPE_64BIT)
			return 1; /* 64-bit MEM */
	} else {
		dev_info->mem_type = BAR_SPACE_IO;
		mask = ~0x3;
	}

	dev_info->addr = bar_value & mask;

	addr = bar_config & mask;
	if (addr != 0) {
		/* calculate the size of the BAR memory required */
		dev_info->size = 1 << (find_first_set_inline(addr) - 1);
	}

	return 0;
}

/*******************************************************************************
 *
 * pci_dev_scan - scan the specified PCI device for all sub functions
 *
 * RETURNS: N/A
 */

static void pci_dev_scan(uint32_t bus,
		       uint32_t dev,
		       uint32_t class_mask /* bitmask, bits set for each needed
					     class */
		       )
{
	uint32_t func;
	uint32_t pci_data;
	static union pci_addr_reg pci_ctrl_addr;
	static union pci_dev pci_dev_header;
	int i;
	int max_bars;

	if (dev_info_index == CONFIG_MAX_PCI_DEVS) {
		/* No more room in the table */
		return;
	}

	/* initialise the PCI controller address register value */

	pci_ctrl_addr.value = 0;
	pci_ctrl_addr.field.enable = 1;
	pci_ctrl_addr.field.bus = bus;
	pci_ctrl_addr.field.device = dev;

	/* scan all the possible functions for this device */

	for (func = 0; func < LSPCI_MAX_FUNC; func++) {
		pci_ctrl_addr.field.func = func;
		pci_read(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(pci_data),
			&pci_data);

		if (pci_data == 0xffffffff)
			continue;

		/* get the PCI header from the device */
		pci_header_get(
			DEFAULT_PCI_CONTROLLER, bus, dev, func, &pci_dev_header);

		/* Skip a device if it's class is not specified by the caller */
		if (!((1 << pci_dev_header.field.class) & class_mask))
			continue;

		/* Get memory and interrupt information */
		if ((pci_dev_header.field.hdr_type & 0x7f) == 1)
			max_bars = 2;
		else
			max_bars = MAX_BARS;
		for (i = 0; i < max_bars; ++i) {
			/* Ignore BARs with errors and 64 bit BARs */
			if (pci_bar_params_get(
				    bus, dev, func, i, dev_info + dev_info_index) !=
			    0)
				continue;
			else {
				dev_info[dev_info_index].vendor_id =
					pci_dev_header.field.vendor_id;
				dev_info[dev_info_index].device_id =
					pci_dev_header.field.device_id;
				dev_info[dev_info_index].class =
					pci_dev_header.field.class;
				dev_info[dev_info_index].irq = pci_pin2irq(
					pci_dev_header.field.interrupt_pin);
				dev_info_index++;
				if (dev_info_index == CONFIG_MAX_PCI_DEVS) {
					/* No more room in the table */
					return;
				}
			}
		}
	}
}

/*******************************************************************************
 *
 * pci_bus_scan - scans PCI bus for devices
 *
 * The routine scans the PCI bus for the devices, which classes are provided
 * in the classMask argument.
 * classMask is constructed as:
 * (1 << class1) | (1 << class2) | ... | (1 << classN)
 *
 * \NOMANUAL
 */

void pci_bus_scan(uint32_t class_mask /* bitmask, bits set for each needed class */
		)
{
	uint32_t bus;
	uint32_t dev;
	union pci_addr_reg pci_ctrl_addr;
	uint32_t pci_data;

	/* initialise the PCI controller address register value */
	pci_ctrl_addr.value = 0;
	pci_ctrl_addr.field.enable = 1;

	/* run through the buses and devices */
	for (bus = 0; bus < LSPCI_MAX_BUS; bus++) {
		for (dev = 0; (dev < LSPCI_MAX_DEV) &&
				      (dev_info_index < CONFIG_MAX_PCI_DEVS);
		     dev++) {
			pci_ctrl_addr.field.bus = bus;
			pci_ctrl_addr.field.device = dev;
			/* try and read register zero of the first function */
			pci_read(DEFAULT_PCI_CONTROLLER,
				pci_ctrl_addr,
				sizeof(pci_data),
				&pci_data);

			/* scan the device if we found something */
			if (pci_data != 0xffffffff)
				pci_dev_scan(bus, dev, class_mask);
		}
	}
}

/*******************************************************************************
 *
 * pci_info_get - returns list of PCI devices
 *
 * \NOMANUAL
 */
struct pci_dev_info *pci_info_get(void)
{
	return dev_info;
}

/*******************************************************************************
 *
 * pci_dev_find - find PCI device of a specified class and specified index
 *
 * Routine looks through the list of detected PCI devices and if the device
 * of specified <class> and <index> exists, sets up <address>, <size> and <irq>.
 *
 * This function can return error if the specified device is not found. In most
 * cases the device class and index are known to exist and therefore will be
 * found. However, due to the somewhat dynamic nature of PCI, we allow for the
 * fact the device may not be found and another attempt with different
 * parameters maybe made.
 *
 * RETURNS: 0, if device is found, -1 otherwise
 */

int pci_dev_find(int class, int idx, uint32_t *addr, uint32_t *size, int *irq)
{
	int i;
	int j;

	for (i = 0, j = 0; i < dev_info_index; i++) {
		if (dev_info[i].class != class)
			continue;

		if (j == idx) {
			*addr = dev_info[i].addr;
			*size = dev_info[i].size;
			*irq = dev_info[i].irq;
			return 0;
		}
		j++;
	}
	return -1;
}

#ifdef PCI_DEBUG
/*******************************************************************************
 *
 * pci_show - Show PCI devices
 *
 * Shows the PCI devices found.
 *
 * RETURNS: N/A
 */

void pci_show(void)
{
	int i;

	printk("PCI devices:\n");
	for (i = 0; i < dev_info_index; i++) {
		printk("%X:%X class: 0x%X, %s, addrs: 0x%X-0x%X, IRQ %d\n",
		       dev_info[i].vendor_id,
		       dev_info[i].device_id,
		       dev_info[i].class,
		       (dev_info[i].mem_type == BAR_SPACE_MEM) ? "MEM" : "I/O",
		       (uint32_t)dev_info[i].addr,
		       (uint32_t)(dev_info[i].addr + dev_info[i].size - 1),
		       dev_info[i].irq);
	}
}
#endif /* PCI_DEBUG */
