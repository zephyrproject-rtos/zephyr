/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sys/device_mmio.h>
#include <drivers/pcie/pcie.h>

#ifdef CONFIG_ACPI
#include <arch/x86/acpi.h>
#endif

#ifdef CONFIG_PCIE_MSI
#include <drivers/pcie/msi.h>
#endif

/* PCI Express Extended Configuration Mechanism (MMIO) */
#ifdef CONFIG_PCIE_MMIO_CFG

#define MAX_PCI_BUS_SEGMENTS 4

static struct {
	uint32_t start_bus;
	uint32_t n_buses;
	uint8_t *mmio;
} bus_segs[MAX_PCI_BUS_SEGMENTS];

static bool do_pcie_mmio_cfg;

static void pcie_mm_init(void)
{
#ifdef CONFIG_ACPI
	struct acpi_mcfg *m = z_acpi_find_table(ACPI_MCFG_SIGNATURE);

	if (m != NULL) {
		int n = (m->sdt.len - sizeof(*m)) / sizeof(m->pci_segs[0]);

		for (int i = 0; i < n && i < MAX_PCI_BUS_SEGMENTS; i++) {
			size_t size;
			uintptr_t phys_addr;

			bus_segs[i].start_bus = m->pci_segs[i].start_bus;
			bus_segs[i].n_buses = 1 + m->pci_segs[i].end_bus
				- m->pci_segs[i].start_bus;

			phys_addr = m->pci_segs[i].base_addr;
			/* 32 devices & 8 functions per bus, 4k per device */
			size = bus_segs[i].n_buses * (32 * 8 * 4096);

			device_map((mm_reg_t *)&bus_segs[i].mmio, phys_addr,
				   size, K_MEM_CACHE_NONE);
		}

		do_pcie_mmio_cfg = true;
	}
#endif
}

static inline void pcie_mm_conf(pcie_bdf_t bdf, unsigned int reg,
				bool write, uint32_t *data)
{
	if (bus_segs[0].mmio == NULL) {
		pcie_mm_init();
	}

	if (do_pcie_mmio_cfg == false) {
		return;
	}

	for (int i = 0; i < ARRAY_SIZE(bus_segs); i++) {
		int off = PCIE_BDF_TO_BUS(bdf) - bus_segs[i].start_bus;

		if (off >= 0 && off < bus_segs[i].n_buses) {
			bdf = PCIE_BDF(off,
				       PCIE_BDF_TO_DEV(bdf),
				       PCIE_BDF_TO_FUNC(bdf));

			volatile uint32_t *regs
				= (void *)&bus_segs[0].mmio[bdf << 4];

			if (write) {
				regs[reg] = *data;
			} else {
				*data = regs[reg];
			}
		}
	}
}

#endif /* CONFIG_PCIE_MMIO_CFG */

/* Traditional Configuration Mechanism */

#define PCIE_X86_CAP	0xCF8U	/* Configuration Address Port */
#define PCIE_X86_CAP_BDF_MASK	0x00FFFF00U  /* b/d/f bits */
#define PCIE_X86_CAP_EN		0x80000000U  /* enable bit */
#define PCIE_X86_CAP_WORD_MASK	0x3FU  /*  6-bit word index .. */
#define PCIE_X86_CAP_WORD_SHIFT	2U  /* .. is in CAP[7:2] */

#define PCIE_X86_CDP	0xCFCU	/* Configuration Data Port */

/*
 * Helper function for exported configuration functions. Configuration access
 * ain't atomic, so spinlock to keep drivers from clobbering each other.
 */
static inline void pcie_io_conf(pcie_bdf_t bdf, unsigned int reg,
				bool write, uint32_t *data)
{
	static struct k_spinlock lock;
	k_spinlock_key_t k;

	bdf &= PCIE_X86_CAP_BDF_MASK;
	bdf |= PCIE_X86_CAP_EN;
	bdf |= (reg & PCIE_X86_CAP_WORD_MASK) << PCIE_X86_CAP_WORD_SHIFT;

	k = k_spin_lock(&lock);
	sys_out32(bdf, PCIE_X86_CAP);

	if (write) {
		sys_out32(*data, PCIE_X86_CDP);
	} else {
		*data = sys_in32(PCIE_X86_CDP);
	}

	sys_out32(0U, PCIE_X86_CAP);
	k_spin_unlock(&lock, k);
}

static inline void pcie_conf(pcie_bdf_t bdf, unsigned int reg,
			     bool write, uint32_t *data)

{
#ifdef CONFIG_PCIE_MMIO_CFG
	if (do_pcie_mmio_cfg) {
		pcie_mm_conf(bdf, reg, write, data);
	} else
#endif
	{
		pcie_io_conf(bdf, reg, write, data);
	}
}

/* these functions are explained in include/drivers/pcie/pcie.h */

uint32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	uint32_t data = 0;

	pcie_conf(bdf, reg, false, &data);
	return data;
}

void pcie_conf_write(pcie_bdf_t bdf, unsigned int reg, uint32_t data)
{
	pcie_conf(bdf, reg, true, &data);
}

#ifdef CONFIG_PCIE_MSI

/* these functions are explained in include/drivers/pcie/msi.h */

uint32_t pcie_msi_map(unsigned int irq)
{
	ARG_UNUSED(irq);
	return 0xFEE00000U;  /* standard delivery to BSP local APIC */
}

uint16_t pcie_msi_mdr(unsigned int irq)
{
	unsigned char vector = Z_IRQ_TO_INTERRUPT_VECTOR(irq);

	return 0x4000U | vector;  /* edge triggered */
}

#endif
