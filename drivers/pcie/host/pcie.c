/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2020 acontis technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <stdbool.h>
#include <drivers/pcie/pcie.h>

#if CONFIG_PCIE_MSI
#include <drivers/pcie/msi.h>
#endif

/* functions documented in drivers/pcie/pcie.h */

bool pcie_probe(pcie_bdf_t bdf, pcie_id_t id)
{
	uint32_t data;

	data = pcie_conf_read(bdf, PCIE_CONF_ID);

	if (data == PCIE_ID_NONE) {
		return false;
	}

	if (id == PCIE_ID_NONE) {
		return true;
	}

	return (id == data);
}

void pcie_set_cmd(pcie_bdf_t bdf, uint32_t bits, bool on)
{
	uint32_t cmdstat;

	cmdstat = pcie_conf_read(bdf, PCIE_CONF_CMDSTAT);

	if (on) {
		cmdstat |= bits;
	} else {
		cmdstat &= ~bits;
	}

	pcie_conf_write(bdf, PCIE_CONF_CMDSTAT, cmdstat);
}

bool pcie_get_mbar(pcie_bdf_t bdf, unsigned int index, struct pcie_mbar *mbar)
{
	bool valid_bar = false;
	uint32_t reg;
	uintptr_t phys_addr = PCIE_CONF_BAR_NONE;
	size_t size = 0;

	for (reg = PCIE_CONF_BAR0; reg <= PCIE_CONF_BAR5; reg++) {
		uint32_t addr = pcie_conf_read(bdf, reg);

		/* skip useless bars and I/O Bars */
		if (addr == 0xFFFFFFFFU || addr == 0x0U || PCIE_CONF_BAR_IO(addr)) {
			continue;
		}

		if (index == 0) {
			valid_bar = true;
			break;
		}

		if (PCIE_CONF_BAR_64(addr)) {
			reg++;
		}

		index--;
	}

	if (valid_bar) {
		phys_addr = pcie_conf_read(bdf, reg);
		pcie_conf_write(bdf, reg, 0xFFFFFFFF);
		size = pcie_conf_read(bdf, reg);
		pcie_conf_write(bdf, reg, (uint32_t)phys_addr);

		if (IS_ENABLED(CONFIG_64BIT) && PCIE_CONF_BAR_64(phys_addr)) {
			reg++;
			phys_addr |= ((uint64_t)pcie_conf_read(bdf, reg)) << 32;
			pcie_conf_write(bdf, reg, 0xFFFFFFFF);
			size |= ((uint64_t)pcie_conf_read(bdf, reg)) << 32;
			pcie_conf_write(bdf, reg, (uint32_t)((uint64_t)phys_addr >> 32));
		}

		size = PCIE_CONF_BAR_ADDR(size);
		if (size) {
			mbar->phys_addr = PCIE_CONF_BAR_ADDR(phys_addr);
			mbar->size = size & ~(size-1);
		} else {
			valid_bar = false;
		}
	}

	return valid_bar;
}

unsigned int pcie_wired_irq(pcie_bdf_t bdf)
{
	uint32_t data = pcie_conf_read(bdf, PCIE_CONF_INTR);

	return PCIE_CONF_INTR_IRQ(data);
}

void pcie_irq_enable(pcie_bdf_t bdf, unsigned int irq)
{
#if CONFIG_PCIE_MSI
	if (pcie_set_msi(bdf, irq)) {
		return;
	}
#endif
	irq_enable(irq);
}
