/*
 * Copyright (c) 2009-2010, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PCI bus support
 *
 *
 * This module implements the PCI config space access functions
 *
 */

#include <kernel.h>
#include <arch/cpu.h>

#include <pci/pci_mgr.h>
#include <string.h>

/**
 *
 * @brief Write a 32bit data to pci reg in offset
 *
 * @param bus_no      Bus number.
 * @param device_no   Device number
 * @param func_no     Function number
 * @param offset      Offset into the configuration space.
 * @param data        Data written to the offset.
 *
 * @return N/A
 */
void pci_config_out_long(u32_t bus_no, u32_t device_no, u32_t func_no,
						 u32_t offset, u32_t data)
{
	union pci_addr_reg pci_addr;

	/* create the PCI address we're going to access */

	pci_addr.field.bus = bus_no;
	pci_addr.field.device = device_no;
	pci_addr.field.func = func_no;
	pci_addr.field.reg = offset / 4U;
	pci_addr.field.offset = 0;

	/* write to the PCI controller */

	pci_write(DEFAULT_PCI_CONTROLLER, pci_addr, sizeof(u32_t), data);
}

/**
 *
 * @brief Write a 16bit data to pci reg in offset
 *
 * @param bus_no      Bus number.
 * @param device_no   Device number.
 * @param func_no     Function number.
 * @param offset      Offset into the configuration space.
 * @param data        Data written to the offset.
 *
 * @return N/A
 */
void pci_config_out_word(u32_t bus_no, u32_t device_no, u32_t func_no,
						 u32_t offset, u16_t data)
{
	union pci_addr_reg pci_addr;

	/* create the PCI address we're going to access */

	pci_addr.field.bus = bus_no;
	pci_addr.field.device = device_no;
	pci_addr.field.func = func_no;
	pci_addr.field.reg = offset / 4U;
	pci_addr.field.offset = offset & 2;

	/* write to the PCI controller */

	pci_write(DEFAULT_PCI_CONTROLLER, pci_addr, sizeof(u16_t), data);
}

/**
 *
 * @brief Write a 8bit data to pci reg in offset
 *
 * @param bus_no      Bus number.
 * @param device_no   Device number.
 * @param func_no     Function number.
 * @param offset      Offset into the configuration space.
 * @param data        Data written to the offset.
 *
 * @return N/A
 */
void pci_config_out_byte(u32_t bus_no, u32_t device_no, u32_t func_no,
						 u32_t offset, u8_t data)
{
	union pci_addr_reg pci_addr;

	/* create the PCI address we're going to access */

	pci_addr.field.bus = bus_no;
	pci_addr.field.device = device_no;
	pci_addr.field.func = func_no;
	pci_addr.field.reg = offset / 4U;
	pci_addr.field.offset = offset % 4;

	/* write to the PCI controller */

	pci_write(DEFAULT_PCI_CONTROLLER, pci_addr, sizeof(u8_t), data);
}

/**
 *
 * @brief Read a 32bit data from pci reg in offset
 *
 * @param bus_no      Bus number.
 * @param device_no   Device number.
 * @param func_no     Function number.
 * @param offset      Offset into the configuration space.
 * @param data        Data read from the offset.
 *
 * @return N/A
 *
 */
void pci_config_in_long(u32_t bus_no, u32_t device_no, u32_t func_no,
						u32_t offset, u32_t *data)
{
	union pci_addr_reg pci_addr;

	/* create the PCI address we're going to access */

	pci_addr.field.bus = bus_no;
	pci_addr.field.device = device_no;
	pci_addr.field.func = func_no;
	pci_addr.field.reg = offset / 4U;
	pci_addr.field.offset = 0;

	/* read from the PCI controller */

	pci_read(DEFAULT_PCI_CONTROLLER, pci_addr, sizeof(u32_t), data);
}

/**
 *
 * @brief Read in a 16bit data from a pci reg in offset
 *
 * @param bus_no      Bus number.
 * @param device_no   Device number.
 * @param func_no     Function number.
 * @param offset      Offset into the configuration space.
 * @param data        Data read from the offset.
 *
 * @return N/A
 *
 */

void pci_config_in_word(u32_t bus_no, u32_t device_no, u32_t func_no,
						u32_t offset, u16_t *data)
{
	union pci_addr_reg pci_addr;
	u32_t pci_data;

	/* create the PCI address we're going to access */

	pci_addr.field.bus = bus_no;
	pci_addr.field.device = device_no;
	pci_addr.field.func = func_no;
	pci_addr.field.reg = offset / 4U;
	pci_addr.field.offset = offset & 2;

	/* read from the PCI controller */

	pci_read(DEFAULT_PCI_CONTROLLER, pci_addr, sizeof(u16_t), &pci_data);

	/* return the data */

	*data = (u16_t)pci_data;
}

/**
 *
 * @brief Read in a 8bit data from a pci reg in offset
 *
 * @param bus_no      Bus number.
 * @param device_no   Device number.
 * @param func_no     Function number.
 * @param offset      Offset into the configuration space.
 * @param data        Data read from the offset.
 *
 * @return N/A
 *
 */

void pci_config_in_byte(u32_t bus_no, u32_t device_no, u32_t func_no,
						u32_t offset, u8_t *data)
{
	union pci_addr_reg pci_addr;
	u32_t pci_data;

	/* create the PCI address we're going to access */

	pci_addr.field.bus = bus_no;
	pci_addr.field.device = device_no;
	pci_addr.field.func = func_no;
	pci_addr.field.reg = offset / 4U;
	pci_addr.field.offset = offset % 4;

	/* read from the PCI controller */

	pci_read(DEFAULT_PCI_CONTROLLER, pci_addr, sizeof(u8_t), &pci_data);

	/* return the data */

	*data = (u8_t)pci_data;
}

/**
 *
 * @brief Find extended capability in ECP linked list
 *
 * This routine searches for an extended capability in the linked list of
 * capabilities in config space. If found, the offset of the first byte
 * of the capability of interest in config space is returned via pOffset.
 *
 * @param ext_cap_find_id   Extended capabilities ID to search for.
 * @param bus               PCI bus number.
 * @param device            PCI device number.
 * @param function          PCI function number.
 * @param p_offset          Returned config space offset.
 *
 * @return 0 if Extended Capability found, -1 otherwise
 *
 */

int pci_config_ext_cap_ptr_find(u8_t ext_cap_find_id, u32_t bus,
								u32_t device, u32_t function,
								u8_t *p_offset)
{
	u16_t tmp_stat;
	u8_t tmp_offset;
	u8_t cap_offset = 0x00;
	u8_t cap_id = 0x00;

	/* Check to see if the device has any extended capabilities */

	pci_config_in_word(bus, device, function, PCI_CFG_STATUS, &tmp_stat);

	if ((tmp_stat & PCI_STATUS_NEW_CAP) == 0U) {
		return -1;
	}

	/* Get the initial ECP offset and make longword aligned */

	pci_config_in_byte(bus, device, function, PCI_CFG_CAP_PTR, &cap_offset);
	cap_offset &= ~0x02;

	/* Bounds check the ECP offset */

	if (cap_offset < 0x40) {
		return -1;
	}

	/* Look for the specified Extended Cap item in the linked list */

	while (cap_offset != 0x00) {

		/* Get the Capability ID and check */

		pci_config_in_byte(bus, device, function, (int)cap_offset, &cap_id);
		if (cap_id == ext_cap_find_id) {
			*p_offset = cap_offset;
			return 0;
		}

		/* Get the offset to the next New Capabilities item */

		tmp_offset = cap_offset + (u8_t)0x01;
		pci_config_in_byte(bus, device, function, (int)tmp_offset, &cap_offset);
	}

	return -1;
}

