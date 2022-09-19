/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pcie/msi.h>
#include <zephyr/drivers/pcie/cap.h>

/* functions documented in include/drivers/pcie/msi.h */

static uint32_t pcie_msi_base(pcie_bdf_t bdf, bool *msi)
{
	uint32_t base;

	if (msi != NULL) {
		*msi = true;
	}

	base = pcie_get_cap(bdf, PCI_CAP_ID_MSI);

	if (IS_ENABLED(CONFIG_PCIE_MSI_X)) {
		uint32_t base_msix;

		base_msix = pcie_get_cap(bdf, PCI_CAP_ID_MSIX);
		if (base_msix != 0U) {
			base = base_msix;

			if (msi != NULL) {
				*msi = false;
			}
		}
	}

	return base;
}

#ifdef CONFIG_PCIE_MSI_MULTI_VECTOR

#include <zephyr/sys/mem_manage.h>

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

#ifdef CONFIG_PCIE_MSI_X

static uint32_t get_msix_table_size(pcie_bdf_t bdf,
				    uint32_t base)
{
	uint32_t mcr;

	mcr = pcie_conf_read(bdf, base + PCIE_MSIX_MCR);

	return ((mcr & PCIE_MSIX_MCR_TSIZE) >> PCIE_MSIX_MCR_TSIZE_SHIFT) + 1;
}

static bool map_msix_table_entries(pcie_bdf_t bdf,
				   uint32_t base,
				   msi_vector_t *vectors,
				   uint8_t n_vector)
{
	uint32_t table_offset;
	uint8_t table_bir;
	struct pcie_mbar bar;
	uintptr_t mapped_table;
	int i;

	table_offset = pcie_conf_read(bdf, base + PCIE_MSIX_TR);
	table_bir = table_offset & PCIE_MSIX_TR_BIR;
	table_offset &= PCIE_MSIX_TR_OFFSET;

	if (!pcie_get_mbar(bdf, table_bir, &bar)) {
		return false;
	}

	z_phys_map((uint8_t **)&mapped_table,
		   bar.phys_addr + table_offset,
		   n_vector * PCIE_MSIR_TABLE_ENTRY_SIZE, K_MEM_PERM_RW);

	for (i = 0; i < n_vector; i++) {
		vectors[i].msix_vector = (struct msix_vector *)
			(mapped_table + (i * PCIE_MSIR_TABLE_ENTRY_SIZE));
	}

	return true;
}

static void set_msix(msi_vector_t *vectors,
		     uint8_t n_vector,
		     bool msix)
{
	int i;

	for (i = 0; i < n_vector; i++) {
		vectors[i].msix = msix;
	}
}

#else
#define get_msix_table_size(...) 0
#define map_msix_table_entries(...) true
#define set_msix(...)
#endif /* CONFIG_PCIE_MSI_X */

static uint32_t get_msi_mmc(pcie_bdf_t bdf,
			    uint32_t base)
{
	uint32_t mcr;

	mcr = pcie_conf_read(bdf, base + PCIE_MSI_MCR);

	/* Getting MMC true count: 2^(MMC field) */
	return 1 << ((mcr & PCIE_MSI_MCR_MMC) >> PCIE_MSI_MCR_MMC_SHIFT);
}

uint8_t pcie_msi_vectors_allocate(pcie_bdf_t bdf,
				  unsigned int priority,
				  msi_vector_t *vectors,
				  uint8_t n_vector)
{
	uint32_t req_vectors;
	uint32_t base;
	bool msi;

	base = pcie_msi_base(bdf, &msi);

	if (IS_ENABLED(CONFIG_PCIE_MSI_X)) {
		set_msix(vectors, n_vector, !msi);

		if (!msi) {
			req_vectors = get_msix_table_size(bdf, base);
			if (!map_msix_table_entries(bdf, base,
						    vectors, n_vector)) {
				return 0;
			}
		}
	}

	if (msi) {
		req_vectors = get_msi_mmc(bdf, base);
	}

	if (n_vector > req_vectors) {
		n_vector = req_vectors;
	}

	for (req_vectors = 0; req_vectors < n_vector; req_vectors++) {
		vectors[req_vectors].bdf = bdf;
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

	base = pcie_msi_base(bdf, NULL);
	if (base == 0U) {
		return false;
	}

	return arch_pcie_msi_vector_connect(vector, routine, parameter, flags);
}

#endif /* CONFIG_PCIE_MSI_MULTI_VECTOR */

#ifdef CONFIG_PCIE_MSI_X

static void enable_msix(pcie_bdf_t bdf,
			msi_vector_t *vectors,
			uint8_t n_vector,
			uint32_t base,
			unsigned int irq)
{
	uint32_t mcr;
	int i;

	for (i = 0; i < n_vector; i++) {
		uint32_t map = pcie_msi_map(irq, &vectors[i], 1);
		uint32_t mdr = pcie_msi_mdr(irq, &vectors[i]);

		vectors[i].msix_vector->msg_addr = map;
		vectors[i].msix_vector->msg_up_addr = 0;
		vectors[i].msix_vector->msg_data = mdr;
		vectors[i].msix_vector->vector_ctrl = 0;
	}

	mcr = pcie_conf_read(bdf, base + PCIE_MSIX_MCR);
	mcr |= PCIE_MSIX_MCR_EN;
	pcie_conf_write(bdf, base + PCIE_MSIX_MCR, mcr);
}

#else
#define enable_msix(...)
#endif /* CONFIG_PCIE_MSI_X */

static void disable_msi(pcie_bdf_t bdf,
			uint32_t base)
{
	uint32_t mcr;

	mcr = pcie_conf_read(bdf, base + PCIE_MSI_MCR);
	mcr &= ~PCIE_MSI_MCR_EN;
	pcie_conf_write(bdf, base + PCIE_MSI_MCR, mcr);
}

static void enable_msi(pcie_bdf_t bdf,
		       msi_vector_t *vectors,
		       uint8_t n_vector,
		       uint32_t base,
		       unsigned int irq)
{
	uint32_t mcr;
	uint32_t map;
	uint32_t mdr;
	uint32_t mme;

	map = pcie_msi_map(irq, vectors, n_vector);
	pcie_conf_write(bdf, base + PCIE_MSI_MAP0, map);

	mdr = pcie_msi_mdr(irq, vectors);
	mcr = pcie_conf_read(bdf, base + PCIE_MSI_MCR);
	if ((mcr & PCIE_MSI_MCR_64) != 0U) {
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
}

bool pcie_msi_enable(pcie_bdf_t bdf,
		     msi_vector_t *vectors,
		     uint8_t n_vector,
		     unsigned int irq)
{
	uint32_t base;
	bool msi;

	base = pcie_msi_base(bdf, &msi);
	if (base == 0U) {
		return false;
	}

	if (!msi && IS_ENABLED(CONFIG_PCIE_MSI_X)) {
		disable_msi(bdf, base);
		enable_msix(bdf, vectors, n_vector, base, irq);
	} else {
		enable_msi(bdf, vectors, n_vector, base, irq);
	}

	pcie_set_cmd(bdf, PCIE_CONF_CMDSTAT_MASTER, true);

	return true;
}

bool pcie_is_msi(pcie_bdf_t bdf)
{
	return (pcie_msi_base(bdf, NULL) != 0);
}
