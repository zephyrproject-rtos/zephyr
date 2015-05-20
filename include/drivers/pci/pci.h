/* pci.h - PCI probe and information routines */

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
Module declares routines of PCI bus initialization and query
*/

#ifndef _PCI_H_
#define _PCI_H_

#undef PCI_DEBUG

#define BAR_SPACE_MEM 0
#define BAR_SPACE_IO 1

/* PCI device information */

struct pci_dev_info {
	uint32_t addr; /* I/O or memory region address */
	uint32_t size; /* memory region size */
	int irq;
	uint16_t mem_type:1; /* memory type: BAR_SPACE_MEM/BAR_SPACE_IO */
	uint16_t class:8;
	uint16_t function:3;
	uint16_t bar:3;
	uint16_t unused:1;
	uint16_t vendor_id;
	uint16_t device_id;
};

extern void pci_bus_scan_init(void);
extern int pci_bus_scan(struct pci_dev_info *dev_info);

#ifdef PCI_DEBUG
extern void pci_show(struct pci_dev_info *dev_info);
#endif /* PCI_DEBUG */

#endif /* _PCI_H_ */
