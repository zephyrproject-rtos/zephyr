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

/* The first bit is used to indicate whether the list of reserved interrupts
 * have been initialized based on content stored in the irq_alloc linker
 * section in ROM.
 */
#define IRQ_LIST_INITIALIZED 0

static ATOMIC_DEFINE(irq_reserved, CONFIG_MAX_IRQ_LINES);

static unsigned int irq_alloc(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(irq_reserved); i++) {
		unsigned int fz, irq;

		while ((fz = find_lsb_set(~atomic_get(&irq_reserved[i])))) {
			irq = (fz - 1) + (i * sizeof(atomic_val_t) * 8);
			if (irq >= CONFIG_MAX_IRQ_LINES) {
				break;
			}

			if (!atomic_test_and_set_bit(irq_reserved, irq)) {
				return irq;
			}
		}
	}

	return PCIE_CONF_INTR_IRQ_NONE;
}

static bool irq_is_reserved(unsigned int irq)
{
	return atomic_test_bit(irq_reserved, irq);
}

static void irq_init(void)
{
	extern uint8_t __irq_alloc_start[];
	extern uint8_t __irq_alloc_end[];
	const uint8_t *irq;

	for (irq = __irq_alloc_start; irq < __irq_alloc_end; irq++) {
		__ASSERT_NO_MSG(*irq < CONFIG_MAX_IRQ_LINES);
		atomic_set_bit(irq_reserved, *irq);
	}
}

unsigned int pcie_alloc_irq(pcie_bdf_t bdf)
{
	unsigned int irq;
	uint32_t data;

	if (!atomic_test_and_set_bit(irq_reserved, IRQ_LIST_INITIALIZED)) {
		irq_init();
	}

	data = pcie_conf_read(bdf, PCIE_CONF_INTR);
	irq = PCIE_CONF_INTR_IRQ(data);

	if (irq == PCIE_CONF_INTR_IRQ_NONE || irq >= CONFIG_MAX_IRQ_LINES ||
	    irq_is_reserved(irq)) {

		irq = irq_alloc();
		if (irq == PCIE_CONF_INTR_IRQ_NONE) {
			return irq;
		}

		data &= ~0xffU;
		data |= irq;
		pcie_conf_write(bdf, PCIE_CONF_INTR, data);
	} else {
		atomic_set_bit(irq_reserved, irq);
	}

	return irq;
}

unsigned int pcie_get_irq(pcie_bdf_t bdf)
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
