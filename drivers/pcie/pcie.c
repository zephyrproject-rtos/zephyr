/*
 * Copyright (c) 2019 Intel Corporation
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

uintptr_t pcie_get_mbar(pcie_bdf_t bdf, unsigned int index)
{
	uint32_t reg, bar;
	uintptr_t addr = PCIE_CONF_BAR_NONE;

	reg = PCIE_CONF_BAR0;
	for (bar = 0; bar < index && reg <= PCIE_CONF_BAR5; bar++) {
		if (PCIE_CONF_BAR_64(pcie_conf_read(bdf, reg++))) {
			reg++;
		}
	}

	if (bar == index) {
		addr = pcie_conf_read(bdf, reg++);
		if (IS_ENABLED(CONFIG_64BIT) && PCIE_CONF_BAR_64(addr)) {
			addr |= ((uint64_t)pcie_conf_read(bdf, reg)) << 32;
		}
	}

	return PCIE_CONF_BAR_ADDR(addr);
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
