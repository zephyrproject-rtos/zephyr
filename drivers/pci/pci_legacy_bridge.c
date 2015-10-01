/* pci_legacy_bridge.c - PCI legacy bridge device driver */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
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
Module provides routines for utilizing the PCI legacy bridge, including
detection of the bridge and using the bridge to configure the routing between
PCI interrupt pins and IRQs.
*/
#include <nanokernel.h>
#include <arch/cpu.h>

#include <drivers/pci/pci_mgr.h>
#include <drivers/pci/pci.h>
#include <board.h>

#define PCI_LEGACY_BRIDGE_REG (0xF0 / 4) /* RCBA offset in 32-bit words */
#define PCI_ADDR_IRQAGENT0 0x3140
#define PCI_ADDR_IRQAGENT1 0x3142
#define PCI_ADDR_IRQAGENT2 0x3144
#define PCI_ADDR_IRQAGENT3 0x3146

/**
 *
 * @brief Return the address mask for the specified root complex
 *
 * The routine checks for the memory present at the specified register
 * and fills in base address mask.
 * Routine should be called in single thread mode during the system
 * initialization.
 *
 * @param pci_ctrl_addr PCI device configuration register address
 * @param mask Pointer to the base address mask
 *
 * @return 0 if root complex is implemented, -1 if not.
 */

static int pci_rcba_mask_get(union pci_addr_reg pci_ctrl_addr,
			     uint32_t *mask)
{
	uint32_t old_value;

	/* save the current setting */
	pci_read(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(old_value),
			&old_value);

	/* write to the RCBA to see how large it is */
	pci_write(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(uint32_t),
			0xffffffff);

	pci_read(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(*mask),
			mask);

	/* put back the old configuration */
	pci_write(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(old_value),
			old_value);

	/* check if this RCBA is implemented */
	if (*mask != 0xffffffff && *mask != 0) {
		/* clear the least address unrelated bit */
		*mask &= ~0x01;
		return 0;
	}

	/* RCBA not supported */

	return -1;
}


/**
 *
 * @brief Retrieves the device information for the PCI legacy bridge, if present
 *
 * @param dev_info device information structure that the routine fills in
 *
 * @return 0 if legacy bridge is detected and -1 otherwise
 */

int pci_legacy_bridge_detect(struct pci_dev_info *dev_info)
{
	union pci_addr_reg pci_ctrl_addr;
	static union pci_dev pci_dev_header;
	uint32_t pci_data; /* temporary data to read */
	uint32_t rcba; /* root complex base address */
	uint32_t rcba_mask; /* bits set for RCBA */

	/* initialise the PCI controller address register value */
	pci_ctrl_addr.value = 0;

	pci_ctrl_addr.field.bus = CONFIG_PCI_LEGACY_BRIDGE_BUS;
	pci_ctrl_addr.field.device = CONFIG_PCI_LEGACY_BRIDGE_DEV;

	/* verify first if there is a valid device at this point */
	pci_ctrl_addr.field.func = 0;

	pci_read(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(pci_data),
			&pci_data);

	if (pci_data == 0xffffffff) {
		return -1;
	}

	/* get the PCI header from the device */
	pci_header_get(DEFAULT_PCI_CONTROLLER,
		       pci_ctrl_addr,
		       &pci_dev_header);

	if (pci_dev_header.field.vendor_id != CONFIG_PCI_LEGACY_BRIDGE_VENDOR_ID ||
	    pci_dev_header.field.device_id != CONFIG_PCI_LEGACY_BRIDGE_DEVICE_ID) {
		return -1;
	}

	pci_ctrl_addr.field.reg = PCI_LEGACY_BRIDGE_REG;

	/* read RCBA PCI register */
	pci_read(DEFAULT_PCI_CONTROLLER,
			pci_ctrl_addr,
			sizeof(rcba),
			&rcba);

	if (pci_rcba_mask_get(pci_ctrl_addr, &rcba_mask) != 0) {
		return -1;
	}

	dev_info->addr = rcba & rcba_mask;
	if (dev_info->addr != 0) {
		/* calculate the size of the root complex memory required */
		dev_info->size = 1 << (find_lsb_set(rcba_mask) - 1);
	}

	dev_info->irq = -1;
	dev_info->bus = CONFIG_PCI_LEGACY_BRIDGE_BUS;
	dev_info->dev = CONFIG_PCI_LEGACY_BRIDGE_DEV;
	dev_info->function = 0;
	dev_info->mem_type = BAR_SPACE_MEM;
	dev_info->class = pci_dev_header.field.class;
	dev_info->bar = 0;
	dev_info->vendor_id = pci_dev_header.field.vendor_id;
	dev_info->device_id = pci_dev_header.field.device_id;

	return 0;
}

/**
 *
 * @brief Configures the route from INTx to IRQx on specified I/O block
 *
 * I/O block 0 includes devices connected to PCIe, I/O block 1 includes
 * UART, SPI, GPIO, I2C
 *
 * @param dev_info device information structure that the routine fills in
 * @param io_block_num number on I/O block
 * @param pci_interrupt_pin PCI interrupt pin
 * @param irq_number IRQ number that corresponds the PCI interrupt pin
 *
 * @return N/A
 */
void pci_legacy_bridge_configure(struct pci_dev_info *dev_info,
				 int io_block_num,
				 int pci_interrupt_pin,
				 int irq_number)
{
	uint32_t addr = (io_block_num == 0) ? PCI_ADDR_IRQAGENT1 :
		PCI_ADDR_IRQAGENT3;
	/*
	 * Each interrupt queue agent register in PCI legacy
	 * bridge has the following format:
	 * Bits 15:12 indicates which IRQ is used for INTD. Valid numbers are
	 *            0-7, which corresponds IRQ 16 - IRQ 23
	 * Bits 11:8  indicates which IRQ is used for INTC.
	 * Bits 7:4   indicates which IRQ is used for INTB.
	 * Bits 3:0   indicates which IRQ is used for INTA.
	 */
	int offset = (pci_interrupt_pin - 1) * 4;
	uint16_t irq_routing = sys_read16(dev_info->addr + addr);

	irq_routing &= ~(0x0f << offset);
	irq_routing |= (irq_number - NUM_STD_IRQS) << offset;
	sys_write16(irq_routing, dev_info->addr + addr);
}
