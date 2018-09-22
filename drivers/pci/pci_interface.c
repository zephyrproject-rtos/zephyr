/*
 * Copyright (c) 2009-2011, 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PCI bus support
 *
 *
 * This module implements the PCI H/W access functions.
 */

#include <kernel.h>
#include <arch/cpu.h>

#include <pci/pci_mgr.h>
#include <string.h>
#include <board.h>

#if (PCI_CTRL_ADDR_REG == 0)
#error "PCI_CTRL_ADDR_REG cannot be zero"
#endif

#if (PCI_CTRL_DATA_REG == 0)
#error "PCI_CTRL_DATA_REG cannot be zero"
#endif

/**
 *
 * @brief Read a PCI controller register
 *
 * @param reg PCI register to read
 * @param data where to put the data
 * @param size size of the data to read (8/16/32 bits)
 *
 * This routine reads the specified register from the PCI controller and
 * places the data into the provided buffer.
 *
 * @return N/A
 *
 */

static void pci_ctrl_read(u32_t reg, u32_t *data, u32_t size)
{
	/* read based on the size requested */

	switch (size) {
		/* long (32 bits) */
	case SYS_PCI_ACCESS_32BIT:
		*data = sys_in32(reg);
		break;
		/* word (16 bits) */
	case SYS_PCI_ACCESS_16BIT:
		*data = sys_in16(reg);
		break;
		/* byte (8 bits) */
	case SYS_PCI_ACCESS_8BIT:
		*data = sys_in8(reg);
		break;
	}
}

/**
 *
 * @brief Write a PCI controller register
 *
 * @param reg PCI register to write
 * @param data data to write
 * @param size size of the data to write (8/16/32 bits)
 *
 * This routine writes the provided data to the specified register in the PCI
 * controller.
 *
 * @return N/A
 *
 */

static void pci_ctrl_write(u32_t reg, u32_t data, u32_t size)
{
	/* write based on the size requested */

	switch (size) {
		/* long (32 bits) */
	case SYS_PCI_ACCESS_32BIT:
		sys_out32(data, reg);
		break;
		/* word (16 bits) */
	case SYS_PCI_ACCESS_16BIT:
		sys_out16(data, reg);
		break;
		/* byte (8 bits) */
	case SYS_PCI_ACCESS_8BIT:
		sys_out8(data, reg);
		break;
	}
}

/**
 *
 * @brief Read the PCI controller data register
 *
 * @param controller controller number
 * @param offset is the offset within the data region
 * @param data is the returned data
 * @param size is the size of the data to read
 *
 * This routine reads the data register of the specified PCI controller.
 *
 * @return 0 or -1
 *
 */

static int pci_ctrl_data_read(u32_t controller, u32_t offset,
				u32_t *data, u32_t size)
{
	/* we only support one controller */

	if (controller != DEFAULT_PCI_CONTROLLER) {
		return (-1);
	}

	pci_ctrl_read(PCI_CTRL_DATA_REG + offset, data, size);

	return 0;
}

/**
 *
 * @brief Write the PCI controller data register
 *
 * @param controller the controller number
 * @param offset is the offset within the address register
 * @param data is the data to write
 * @param size is the size of the data
 *
 * This routine writes the provided data to the data register of the
 * specified PCI controller.
 *
 * @return 0 or -1
 *
 */

static int pci_ctrl_data_write(u32_t controller, u32_t offset,
			       u32_t data, u32_t size)
{
	/* we only support one controller */

	if (controller != DEFAULT_PCI_CONTROLLER) {
		return (-1);
	}

	pci_ctrl_write(PCI_CTRL_DATA_REG + offset, data, size);

	return 0;
}

/**
 *
 * @brief Write the PCI controller address register
 *
 * @param controller is the controller number
 * @param offset is the offset within the address register
 * @param data is the data to write
 * @param size is the size of the data
 *
 * This routine writes the provided data to the address register of the
 * specified PCI controller.
 *
 * @return 0 or -1
 *
 */

static int pci_ctrl_addr_write(u32_t controller, u32_t offset,
			       u32_t data, u32_t size)
{
	/* we only support one controller */

	if (controller != DEFAULT_PCI_CONTROLLER) {
		return (-1);
	}

	pci_ctrl_write(PCI_CTRL_ADDR_REG + offset, data, size);
	return 0;
}

/**
 *
 * @brief Read a PCI register from a device
 *
 * This routine reads data from a PCI device's configuration space.  The
 * device and register to read is specified by the address parameter ("addr")
 * and must be set appropriately by the caller.  The address is defined by
 * the structure type pci_addr_t and contains the following members:
 *
 *     bus:     PCI bus number (0-255)
 *     device:  PCI device number (0-31)
 *     func:    device function number (0-7)
 *     reg:     device 32-bit register number to read (0-63)
 *     offset:  offset within 32-bit register to read (0-3)
 *
 * The size parameter specifies the number of bytes to read from the PCI
 * configuration space, valid values are 1, 2, and 4 bytes.  A 32-bit value
 * is always returned but it will contain only the number of bytes specified
 * by the size parameter.
 *
 * If multiple PCI controllers are present in the system, the controller id
 * can be specified in the "controller" parameter.  If only one controller
 * is present, the id DEFAULT_PCI_CONTROLLER can be used to denote this
 * controller.
 *
 * Example:
 *
 *  union pci_addr_reg addr;
 *  u32_t status;
 *
 *  addr.field.bus    = 0;    /@ PCI bus zero        @/
 *  addr.field.device = 1;    /@ PCI device one      @/
 *  addr.field.func   = 0;    /@ PCI function zero   @/
 *  addr.field.reg    = 4;    /@ PCI register 4      @/
 *  addr.field.offset = 0;    /@ PCI register offset @/
 *
 *  pci_read (DEFAULT_PCI_CONTROLLER, addr, sizeof(u16_t), &status);
 *
 *
 * NOTE:
 *   Reading of PCI data must be performed as an atomic operation. It is up to
 *   the caller to enforce this.
 *
 * @param controller is the PCI controller number to use
 * @param addr is the PCI address to read
 * @param size is the size of the data in bytes
 * @param data is a pointer to the data read from the device
 *
 * @return N/A
 *
 */

void pci_read(u32_t controller, union pci_addr_reg addr,
	      u32_t size, u32_t *data)
{
	u32_t access_size;
	u32_t access_offset;

	/* validate the access size */

	switch (size) {
	case 1:
		access_size = SYS_PCI_ACCESS_8BIT;
		access_offset = addr.field.offset;
		break;
	case 2:
		access_size = SYS_PCI_ACCESS_16BIT;
		access_offset = addr.field.offset;
		break;
	case 4:
	default:
		access_size = SYS_PCI_ACCESS_32BIT;
		access_offset = 0;
		break;
	}

	/* ensure enable has been set */

	addr.field.enable = 1;

	/* clear the offset for the address register */

	addr.field.offset = 0;

	/* read the data from the PCI controller */

	pci_ctrl_addr_write(
		controller, PCI_NO_OFFSET, addr.value, SYS_PCI_ACCESS_32BIT);

	pci_ctrl_data_read(controller, access_offset, data, access_size);
}

/**
 *
 * @brief Write a to a PCI register
 *
 * This routine writes data to a PCI device's configuration space.  The
 * device and register to write is specified by the address parameter ("addr")
 * and must be set appropriately by the caller.  The address is defined by
 * the structure type pci_addr_t and contains the following members:
 *
 *     bus:     PCI bus number (0-255)
 *     device:  PCI device number (0-31)
 *     func:    device function number (0-7)
 *     reg:     device register number to read (0-63)
 *     offset:  offset within 32-bit register to write (0-3)
 *
 * The size parameter specifies the number of bytes to write to the PCI
 * configuration space, valid values are 1, 2, and 4 bytes.  A 32-bit value
 * is always provided but only the number of bytes specified by the size
 * parameter will be written to the device.
 *
 * If multiple PCI controllers are present in the system, the controller id
 * can be specified in the "controller" parameter.  If only one controller
 * is present, the id DEFAULT_PCI_CONTROLLER can be used to denote this
 * controller.
 *
 * Example:
 *
 *  pci_addr_t addr;
 *  u32_t bar0 = 0xE0000000;
 *
 *  addr.field.bus    = 0;    /@ PCI bus zero        @/
 *  addr.field.device = 1;    /@ PCI device one      @/
 *  addr.field.func   = 0;    /@ PCI function zero   @/
 *  addr.field.reg    = 16;   /@ PCI register 16     @/
 *  addr.field.offset = 0;    /@ PCI register offset @/
 *
 *  pci_write (DEFAULT_PCI_CONTROLLER, addr, sizeof(u32_t), bar0);
 *
 * NOTE:
 *   Writing of PCI data must be performed as an atomic operation. It is up to
 *   the caller to enforce this.
 *
 * @param controller is the PCI controller to use
 * @param addr is the PCI address to read
 * @param size is the size in bytes to write
 * @param data is the data to write
 *
 * @return N/A
 *
 */

void pci_write(u32_t controller, union pci_addr_reg addr,
	       u32_t size, u32_t data)
{
	u32_t access_size;
	u32_t access_offset;

	/* validate the access size */

	switch (size) {
	case 1:
		access_size = SYS_PCI_ACCESS_8BIT;
		access_offset = addr.field.offset;
		break;
	case 2:
		access_size = SYS_PCI_ACCESS_16BIT;
		access_offset = addr.field.offset;
		break;
	case 4:
	default:
		access_size = SYS_PCI_ACCESS_32BIT;
		access_offset = 0;
		break;
	}

	/* ensure enable has been set */

	addr.field.enable = 1;

	/* clear the offset for the address register */

	addr.field.offset = 0;

	/* write the data to the PCI controller */

	pci_ctrl_addr_write(
		controller, PCI_NO_OFFSET, addr.value, SYS_PCI_ACCESS_32BIT);
	pci_ctrl_data_write(controller, access_offset, data, access_size);
}

/**
 *
 * @brief Get the PCI header for a device
 *
 * This routine reads the PCI header for the specified device and puts the
 * result in the supplied header structure.
 *
 * @return N/A
 */

void pci_header_get(u32_t controller,
		    union pci_addr_reg pci_ctrl_addr,
		    union pci_dev *pci_dev_header)
{
	u32_t i;

	/* clear out the header */

	(void)memset(pci_dev_header, 0, sizeof(*pci_dev_header));

	/* fill in the PCI header from the device */

	for (i = 0; i < PCI_HEADER_WORDS; i++) {
		pci_ctrl_addr.field.reg = i;
		pci_read(controller,
			pci_ctrl_addr,
			sizeof(u32_t),
			&pci_dev_header->words.word[i]);
	}
}

