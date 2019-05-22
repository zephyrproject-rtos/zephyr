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
	u32_t data;

	data = pcie_conf_read(bdf, PCIE_CONF_ID);

	if (data == PCIE_ID_NONE) {
		return false;
	}

	if (id == PCIE_ID_NONE) {
		return true;
	}

	return (id == data);
}

void pcie_set_cmd(pcie_bdf_t bdf, u32_t bits, bool on)
{
	u32_t cmdstat;

	cmdstat = pcie_conf_read(bdf, PCIE_CONF_CMDSTAT);

	if (on) {
		cmdstat |= bits;
	} else {
		cmdstat &= ~bits;
	}

	pcie_conf_write(bdf, PCIE_CONF_CMDSTAT, cmdstat);
}

static u32_t pcie_get_bar(pcie_bdf_t bdf, unsigned int index, bool io)
{
	int bar;
	u32_t data;

	for (bar = PCIE_CONF_BAR0; bar <= PCIE_CONF_BAR5; ++bar) {
		data = pcie_conf_read(bdf, bar);
		if (data == PCIE_CONF_BAR_NONE) {
			continue;
		}

		if ((PCIE_CONF_BAR_IO(data) && io) ||
		    (PCIE_CONF_BAR_MEM(data) && !io)) {
			if (index == 0) {
				return PCIE_CONF_BAR_ADDR(data);
			}

			--index;
		}

		if (PCIE_CONF_BAR_64(data)) {
			++bar;
		}
	}

	return PCIE_CONF_BAR_NONE;
}

u32_t pcie_get_mbar(pcie_bdf_t bdf, unsigned int index)
{
	return pcie_get_bar(bdf, index, false);
}

unsigned int pcie_wired_irq(pcie_bdf_t bdf)
{
	u32_t data = pcie_conf_read(bdf, PCIE_CONF_INTR);

	return PCIE_CONF_INTR_IRQ(data);
}

u32_t pcie_get_iobar(pcie_bdf_t bdf, unsigned int index)
{
	return pcie_get_bar(bdf, index, true);
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
