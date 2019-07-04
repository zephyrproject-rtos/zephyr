/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/pcie/pcie.h>

#ifdef CONFIG_PCIE_MSI
#include <drivers/pcie/msi.h>
#endif

/*
 * The Configuration Mechanism (previously, Configuration Mechanism #1)
 * uses two 32-bit ports in the I/O space, here called CAP and CDP.
 *
 * N.B.: this code relies on the fact that the PCIE_BDF() format (as
 * defined in dt-bindings/pcie/pcie.h) and the CAP agree on the bus/dev/func
 * bitfield positions and sizes.
 */

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
static void pcie_conf(pcie_bdf_t bdf, unsigned int reg, bool write, u32_t *data)
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

/* these functions are explained in include/drivers/pcie/pcie.h */

u32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	u32_t data;

	pcie_conf(bdf, reg, false, &data);
	return data;
}

void pcie_conf_write(pcie_bdf_t bdf, unsigned int reg, u32_t data)
{
	pcie_conf(bdf, reg, true, &data);
}

#ifdef CONFIG_PCIE_MSI

/* these functions are explained in include/drivers/pcie/msi.h */

u32_t pcie_msi_map(unsigned int irq)
{
	ARG_UNUSED(irq);
	return 0xFEE00000U;  /* standard delivery to BSP local APIC */
}

u16_t pcie_msi_mdr(unsigned int irq)
{
	unsigned char vector = Z_IRQ_TO_INTERRUPT_VECTOR(irq);

	return 0x4000U | vector;  /* edge triggered */
}

#endif
