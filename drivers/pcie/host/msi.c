/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <drivers/pcie/msi.h>

/* functions documented in include/drivers/pcie/msi.h */

#ifdef CONFIG_PCIE_MSI_MULTI_VECTOR

__weak uint8_t arch_pcie_msi_vectors_allocate(unsigned int priority,
					      msi_vector_t *vectors,
					      uint8_t n_vector)
{
	ARG_UNUSED(priority);
	ARG_UNUSED(vectors);
	ARG_UNUSED(n_vector);

	return 0;
}


__weak bool arch_pcie_msi_vector_connect(msi_vector_t *vector,
					 void (*routine)(const void *parameter),
					 const void *parameter,
					 uint32_t flags)
{
	ARG_UNUSED(vector);
	ARG_UNUSED(routine);
	ARG_UNUSED(parameter);
	ARG_UNUSED(flags);

	return false;
}

uint8_t pcie_msi_vectors_allocate(pcie_bdf_t bdf,
				  unsigned int priority,
				  msi_vector_t *vectors,
				  uint8_t n_vector)
{
	uint32_t base;
	uint32_t mcr;
	uint8_t mmc;

	base = pcie_get_cap(bdf, PCIE_MSI_CAP_ID);
	if (base == 0U) {
		return 0;
	}

	mcr = pcie_conf_read(bdf, base + PCIE_MSI_MCR);
	mmc = (uint8_t)((mcr & PCIE_MSI_MCR_MMC) >> PCIE_MSI_MCR_MMC_SHIFT);
	/* Getting MMC true count: 2^(MMC field) */
	mmc = 1 << mmc;

	if (n_vector > mmc) {
		n_vector = mmc;
	}

	return arch_pcie_msi_vectors_allocate(priority, vectors, n_vector);
}

bool pcie_msi_vector_connect(pcie_bdf_t bdf,
			     msi_vector_t *vector,
			     void (*routine)(const void *parameter),
			     const void *parameter,
			     uint32_t flags)
{
	uint32_t base;

	base = pcie_get_cap(bdf, PCIE_MSI_CAP_ID);
	if (base == 0U) {
		return false;
	}

	vector->bdf = bdf;

	return arch_pcie_msi_vector_connect(vector, routine, parameter, flags);
}

#endif /* CONFIG_PCIE_MSI_MULTI_VECTOR */

bool pcie_msi_enable(pcie_bdf_t bdf,
		     msi_vector_t *vectors,
		     uint8_t n_vector)
{
	unsigned int irq;
	uint32_t base;
	uint32_t mcr;
	uint32_t map;
	uint32_t mdr;
	uint32_t mme;

	base = pcie_get_cap(bdf, PCIE_MSI_CAP_ID);
	if (base == 0U) {
		return false;
	}

	irq = pcie_get_irq(bdf);

	map = pcie_msi_map(irq, vectors);
	pcie_conf_write(bdf, base + PCIE_MSI_MAP0, map);

	mdr = pcie_msi_mdr(irq, vectors);
	mcr = pcie_conf_read(bdf, base + PCIE_MSI_MCR);
	if (mcr & PCIE_MSI_MCR_64) {
		pcie_conf_write(bdf, base + PCIE_MSI_MAP1_64, 0U);
		pcie_conf_write(bdf, base + PCIE_MSI_MDR_64, mdr);
	} else {
		pcie_conf_write(bdf, base + PCIE_MSI_MDR_32, mdr);
	}

	/* Generating MME field (1 counts as a power of 2) */
	for (mme = 0; n_vector > 1; mme++) {
		n_vector >>= 1;
	}

	mcr |= mme << PCIE_MSI_MCR_MME_SHIFT;

	mcr |= PCIE_MSI_MCR_EN;
	pcie_conf_write(bdf, base + PCIE_MSI_MCR, mcr);
	pcie_set_cmd(bdf, PCIE_CONF_CMDSTAT_MASTER, true);

	return true;
}
