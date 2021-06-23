/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2020 acontis technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(pcie, LOG_LEVEL_ERR);

#include <kernel.h>
#include <stdbool.h>
#include <drivers/pcie/pcie.h>

#if CONFIG_PCIE_MSI
#include <drivers/pcie/msi.h>
#endif

#ifdef CONFIG_PCIE_CONTROLLER
#include <drivers/pcie/controller.h>
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

uint32_t pcie_get_cap(pcie_bdf_t bdf, uint32_t cap_id)
{
	uint32_t reg = 0U;
	uint32_t data;

	data = pcie_conf_read(bdf, PCIE_CONF_CMDSTAT);
	if (data & PCIE_CONF_CMDSTAT_CAPS) {
		data = pcie_conf_read(bdf, PCIE_CONF_CAPPTR);
		reg = PCIE_CONF_CAPPTR_FIRST(data);
	}

	while (reg) {
		data = pcie_conf_read(bdf, reg);

		if (PCIE_CONF_CAP_ID(data) == cap_id) {
			break;
		}

		reg = PCIE_CONF_CAP_NEXT(data);
	}

	return reg;
}

uint32_t pcie_get_ext_cap(pcie_bdf_t bdf, uint32_t cap_id)
{
	unsigned int reg = PCIE_CONF_EXT_CAPPTR; /* Start at end of the PCI configuration space */
	uint32_t data;

	while (reg) {
		data = pcie_conf_read(bdf, reg);
		if (!data || data == 0xffffffff) {
			return 0;
		}

		if (PCIE_CONF_EXT_CAP_ID(data) == cap_id) {
			break;
		}

		reg = PCIE_CONF_EXT_CAP_NEXT(data) >> 2;

		if (reg < PCIE_CONF_EXT_CAPPTR) {
			return 0;
		}
	}

	return reg;
}

bool pcie_get_mbar(pcie_bdf_t bdf,
		   unsigned int bar_index,
		   struct pcie_mbar *mbar)
{
	uint32_t reg = bar_index + PCIE_CONF_BAR0;
#ifdef CONFIG_PCIE_CONTROLLER
	const struct device *dev;
#endif
	uintptr_t phys_addr;
	size_t size;

#ifdef CONFIG_PCIE_CONTROLLER
	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_pcie_controller));
	if (!dev) {
		LOG_ERR("Failed to get PCIe root complex");
		return false;
	}
#endif

	if (reg > PCIE_CONF_BAR5) {
		return false;
	}

	phys_addr = pcie_conf_read(bdf, reg);
#ifndef CONFIG_PCIE_CONTROLLER
	if (PCIE_CONF_BAR_IO(phys_addr)) {
		/* Discard I/O bars */
		return false;
	}
#endif

	if (PCIE_CONF_BAR_INVAL_FLAGS(phys_addr)) {
		/* Discard on invalid flags */
		return false;
	}

	pcie_conf_write(bdf, reg, 0xFFFFFFFF);
	size = pcie_conf_read(bdf, reg);
	pcie_conf_write(bdf, reg, (uint32_t)phys_addr);

	if (IS_ENABLED(CONFIG_64BIT) && PCIE_CONF_BAR_64(phys_addr)) {
		reg++;
		phys_addr |= ((uint64_t)pcie_conf_read(bdf, reg)) << 32;

		if (PCIE_CONF_BAR_ADDR(phys_addr) == PCIE_CONF_BAR_INVAL64 ||
		    PCIE_CONF_BAR_ADDR(phys_addr) == PCIE_CONF_BAR_NONE) {
			/* Discard on invalid address */
			return false;
		}

		pcie_conf_write(bdf, reg, 0xFFFFFFFF);
		size |= ((uint64_t)pcie_conf_read(bdf, reg)) << 32;
		pcie_conf_write(bdf, reg, (uint32_t)((uint64_t)phys_addr >> 32));
	} else if (PCIE_CONF_BAR_ADDR(phys_addr) == PCIE_CONF_BAR_INVAL ||
		   PCIE_CONF_BAR_ADDR(phys_addr) == PCIE_CONF_BAR_NONE) {
		/* Discard on invalid address */
		return false;
	}

	if (PCIE_CONF_BAR_IO(phys_addr)) {
		size = PCIE_CONF_BAR_IO_ADDR(size);
		if (size == 0) {
			/* Discard on invalid size */
			return false;
		}
	} else {
		size = PCIE_CONF_BAR_ADDR(size);
		if (size == 0) {
			/* Discard on invalid size */
			return false;
		}
	}

#ifdef CONFIG_PCIE_CONTROLLER
	/* Translate to physical memory address from bus address */
	if (!pcie_ctrl_region_xlate(dev, bdf, PCIE_CONF_BAR_MEM(phys_addr),
				    PCIE_CONF_BAR_64(phys_addr),
				    PCIE_CONF_BAR_MEM(phys_addr) ?
					  PCIE_CONF_BAR_IO_ADDR(phys_addr)
					: PCIE_CONF_BAR_ADDR(phys_addr),
				    &mbar->phys_addr)) {
		return false;
	}
#else
	mbar->phys_addr = PCIE_CONF_BAR_ADDR(phys_addr);
#endif /* CONFIG_PCIE_CONTROLLER */
	mbar->size = size & ~(size-1);

	return true;
}

bool pcie_probe_mbar(pcie_bdf_t bdf,
		     unsigned int index,
		     struct pcie_mbar *mbar)
{
	uint32_t reg;

	for (reg = PCIE_CONF_BAR0;
	     index > 0 && reg <= PCIE_CONF_BAR5; reg++, index--) {
		uintptr_t addr = pcie_conf_read(bdf, reg);

		if (PCIE_CONF_BAR_MEM(addr) && PCIE_CONF_BAR_64(addr)) {
			reg++;
		}
	}

	if (index != 0) {
		return false;
	}

	return pcie_get_mbar(bdf, reg - PCIE_CONF_BAR0, mbar);
}

/* The first bit is used to indicate whether the list of reserved interrupts
 * have been initialized based on content stored in the irq_alloc linker
 * section in ROM.
 */
#define IRQ_LIST_INITIALIZED 0

#ifndef CONFIG_MAX_IRQ_LINES
#warning TOFIX for non-x86
#define CONFIG_MAX_IRQ_LINES 0
#endif

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
	if (pcie_msi_enable(bdf, NULL, 1, irq)) {
		return;
	}
#endif
	irq_enable(irq);
}

pcie_bdf_t pcie_bdf_lookup(pcie_id_t id)
{
	int bus, dev, func;

	for (bus = 0; bus <= PCIE_MAX_BUS; bus++) {
		for (dev = 0; dev <= PCIE_MAX_DEV; dev++) {
			for (func = 0; func <= PCIE_MAX_FUNC; func++) {
				pcie_bdf_t bdf = PCIE_BDF(bus, dev, func);
				uint32_t data;

				data = pcie_conf_read(bdf, PCIE_CONF_ID);
				if (data == PCIE_ID_NONE) {
					continue;
				}

				if (data == id) {
					return bdf;
				}
			}
		}
	}

	return PCIE_BDF_NONE;
}
