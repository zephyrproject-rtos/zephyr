/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pcie/msi.h>
#include <zephyr/drivers/pcie/cap.h>
#include <zephyr/drivers/pcie/controller.h>

#include "pcie_priv.h"

#ifdef CONFIG_PCIE_CONTROLLER
struct msi_config {
	uint32_t base;
	uint32_t mcr;
	uint32_t available;
	bool msix;
};
#endif

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

#endif

#ifndef CONFIG_PCIE_CONTROLLER
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

#include <zephyr/kernel/mm.h>

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
	struct pcie_bar bar;
	uintptr_t mapped_table;
	int i;

	table_offset = pcie_conf_read(bdf, base + PCIE_MSIX_TR);
	table_bir = table_offset & PCIE_MSIX_TR_BIR;
	table_offset &= PCIE_MSIX_TR_OFFSET;

	if (!pcie_get_mbar(bdf, table_bir, &bar)) {
		return false;
	}

	device_map(&mapped_table,
			    bar.phys_addr + table_offset,
			    n_vector * PCIE_MSIR_TABLE_ENTRY_SIZE,
			    K_MEM_CACHE_NONE | K_MEM_PERM_RW);

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
		vectors[req_vectors].ctrl = NULL;
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
		uint64_t map = pcie_msi_map(irq, &vectors[i], 1);
		uint32_t mdr = pcie_msi_mdr(irq, &vectors[i]);

		sys_write32((uint32_t)map, (mm_reg_t)&vectors[i].msix_vector->msg_addr);
		sys_write32((uint32_t)(map >> 32), (mm_reg_t)&vectors[i].msix_vector->msg_up_addr);
		sys_write32(mdr, (mm_reg_t) &vectors[i].msix_vector->msg_data);
		sys_write32(0, (mm_reg_t) &vectors[i].msix_vector->vector_ctrl);
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

static bool enable_msi(pcie_bdf_t bdf, msi_vector_t *vectors, uint8_t n_vector, uint32_t base,
		       unsigned int irq)
{
	uint32_t mcr;
	uint64_t map;
	uint32_t mdr;
	uint32_t mme;

	mcr = pcie_conf_read(bdf, base + PCIE_MSI_MCR);
	map = pcie_msi_map(irq, vectors, n_vector);
	if ((mcr & PCIE_MSI_MCR_64) == 0U && (map >> 32) != 0U) {
		return false;
	}

	mdr = pcie_msi_mdr(irq, vectors);
	pcie_conf_write(bdf, base + PCIE_MSI_MAP0, (uint32_t)map);
	if ((mcr & PCIE_MSI_MCR_64) != 0U) {
		pcie_conf_write(bdf, base + PCIE_MSI_MAP1_64, (uint32_t)(map >> 32));
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

	return true;
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
	} else if (!enable_msi(bdf, vectors, n_vector, base, irq)) {
		return false;
	}

	pcie_set_cmd(bdf, PCIE_CONF_CMDSTAT_MASTER, true);

	return true;
}

bool pcie_is_msi(pcie_bdf_t bdf)
{
	return (pcie_msi_base(bdf, NULL) != 0);
}

#else

static int msi_config_get(const struct pcie_endpoint *ep, struct msi_config *cfg)
{
	int ret;

	*cfg = (struct msi_config){0};
	if (IS_ENABLED(CONFIG_PCIE_MSI_X)) {
		ret = pcie_endpoint_get_cap(ep, PCI_CAP_ID_MSIX, &cfg->base);
		if (ret != 0) {
			return ret;
		}
		cfg->msix = cfg->base != 0U;
	}
	if (!cfg->msix) {
		ret = pcie_endpoint_get_cap(ep, PCI_CAP_ID_MSI, &cfg->base);
		if (ret != 0 || cfg->base == 0U) {
			return ret != 0 ? ret : -ENOTSUP;
		}
	}
	ret = pcie_endpoint_conf_read(ep, cfg->base, &cfg->mcr);
	if (ret == 0) {
		cfg->available = cfg->msix ?
			((cfg->mcr & PCIE_MSIX_MCR_TSIZE) >> PCIE_MSIX_MCR_TSIZE_SHIFT) + 1U :
			1U << ((cfg->mcr & PCIE_MSI_MCR_MMC) >> PCIE_MSI_MCR_MMC_SHIFT);
	}
	return ret;
}

#ifdef CONFIG_PCIE_MSI_MULTI_VECTOR
static void msi_vectors_init(const struct pcie_endpoint *ep, msi_vector_t *vectors,
			     uint8_t count, bool msix)
{
	for (uint8_t i = 0U; i < count; i++) {
		vectors[i].ctrl = ep->ctrl;
		vectors[i].bdf = ep->bdf;
		vectors[i].arch.address = 0U;
#ifdef CONFIG_PCIE_MSI_X
		vectors[i].msix = msix;
		vectors[i].msix_vector = NULL;
#endif
	}
}

static int msi_map_table(const struct pcie_endpoint *ep, uint32_t base,
			 msi_vector_t *vectors, uint8_t count, mm_reg_t *mapping)
{
#ifdef CONFIG_PCIE_MSI_X
	struct pcie_bar bar;
	uint32_t table;
	size_t size = count * sizeof(struct msix_vector);
	int ret;

	ret = pcie_endpoint_conf_read(ep, base + PCIE_MSIX_TR, &table);
	if (ret != 0) {
		return ret;
	}
	ret = pcie_endpoint_get_mbar(ep, table & PCIE_MSIX_TR_BIR, &bar);
	if (ret != 0) {
		return ret;
	}
	table &= PCIE_MSIX_TR_OFFSET;
	if (table > bar.size || size > bar.size - table ||
	    bar.phys_addr > UINTPTR_MAX - table ||
	    bar.phys_addr + table > UINTPTR_MAX - (size - 1U)) {
		return -EINVAL;
	}
#ifdef CONFIG_MMU
	uint8_t *virt;

	/* A failed request must not unmap another allocation's direct mapping. */
	k_mem_map_phys_bare(&virt, bar.phys_addr + table, size, K_MEM_CACHE_NONE | K_MEM_PERM_RW);
	*mapping = (mm_reg_t)virt;
#else
	device_map(mapping, bar.phys_addr + table, size, K_MEM_CACHE_NONE | K_MEM_PERM_RW);
#endif
	for (uint8_t i = 0U; i < count; i++) {
		vectors[i].msix_vector =
			(struct msix_vector *)(*mapping + i * sizeof(struct msix_vector));
	}
	return 0;
#else
	return -ENOTSUP;
#endif
}

uint8_t pcie_msi_vectors_allocate(pcie_bdf_t bdf, unsigned int priority,
				msi_vector_t *vectors, uint8_t n_vector)
{
	const struct pcie_endpoint ep = z_pcie_legacy_endpoint(bdf);
	struct msi_config cfg;
	mm_reg_t mapping = 0U;
	uint8_t allocated;

	if (vectors == NULL || n_vector == 0U || msi_config_get(&ep, &cfg) != 0) {
		return 0U;
	}
	n_vector = MIN(n_vector, cfg.available);
	msi_vectors_init(&ep, vectors, n_vector, cfg.msix);
	if (cfg.msix && msi_map_table(&ep, cfg.base, vectors, n_vector, &mapping) != 0) {
		return 0U;
	}
	/* Legacy providers may return a partial allocation. */
	allocated = arch_pcie_msi_vectors_allocate(priority, vectors, n_vector);
	if (allocated == 0U && cfg.msix) {
		device_unmap(mapping, n_vector * sizeof(struct msix_vector));
		memset(vectors, 0, n_vector * sizeof(*vectors));
	}
	return allocated;
}

bool pcie_msi_vector_connect(pcie_bdf_t bdf, msi_vector_t *vector,
			     void (*routine)(const void *parameter),
			     const void *parameter, uint32_t flags)
{
	const struct pcie_endpoint ep = z_pcie_legacy_endpoint(bdf);
	struct msi_config cfg;

	if (vector == NULL || routine == NULL ||
	    (vector->ctrl != NULL && vector->ctrl != ep.ctrl) || msi_config_get(&ep, &cfg) != 0) {
		return false;
	}
	return arch_pcie_msi_vector_connect(vector, routine, parameter, flags);
}
#endif /* CONFIG_PCIE_MSI_MULTI_VECTOR */

static int msi_enable(const struct pcie_endpoint *ep, const struct msi_config *cfg,
		      msi_vector_t *vectors, uint8_t count, unsigned int irq, bool strict)
{
	uint32_t mcr = cfg->mcr;
	uint32_t cmd;
	int ret;

	if (vectors == NULL || count == 0U || count > cfg->available) {
		return -EINVAL;
	}
#ifdef CONFIG_PCIE_MSI_X
	for (uint8_t i = 0U; i < count; i++) {
		if (vectors[i].msix != cfg->msix ||
		    (cfg->msix && vectors[i].msix_vector == NULL)) {
			return -EINVAL;
		}
	}
#endif
	if (cfg->msix) {
#ifdef CONFIG_PCIE_MSI_X
		uint32_t base_msi, mcr_msi;

		ret = pcie_endpoint_get_cap(ep, PCI_CAP_ID_MSI, &base_msi);
		if (ret != 0) {
			return ret;
		}
		if (base_msi != 0U) {
			ret = pcie_endpoint_conf_read(ep, base_msi, &mcr_msi);
			if (ret == 0) {
				ret = pcie_endpoint_conf_write(ep, base_msi,
							       mcr_msi & ~PCIE_MSI_MCR_EN);
			}
			if (ret != 0) {
				return ret;
			}
		}
		ret = pcie_endpoint_conf_write(ep, cfg->base, mcr | PCIE_MSIX_MCR_FMASK);
		if (ret != 0) {
			return ret;
		}
		for (uint8_t i = 0U; i < count; i++) {
			pcie_msix_write_entry(vectors[i].msix_vector,
					      pcie_msi_map(irq, &vectors[i], 1U),
					      pcie_msi_mdr(irq, &vectors[i]));
		}
		mcr = (mcr | PCIE_MSIX_MCR_EN) & ~PCIE_MSIX_MCR_FMASK;
#endif
	} else {
		uint64_t address = pcie_msi_map(irq, vectors, count);
		uint32_t data = pcie_msi_mdr(irq, vectors);
		uint32_t mme = 0U;

		if (!is_power_of_two(count)) {
			return -EINVAL;
		}
		if (strict) {
			if ((data & (count - 1U)) != 0U) {
				return -EINVAL;
			}
			for (uint8_t i = 1U; i < count; i++) {
				if (pcie_msi_map(irq, &vectors[i], 1U) != address ||
				    pcie_msi_mdr(irq, &vectors[i]) != data + i) {
					return -EINVAL;
				}
			}
		}
		if ((mcr & PCIE_MSI_MCR_64) == 0U && (address >> 32) != 0U) {
			return -ENOTSUP;
		}
		ret = pcie_endpoint_conf_write(ep, cfg->base + PCIE_MSI_MAP0, (uint32_t)address);
		if (ret == 0 && (mcr & PCIE_MSI_MCR_64) != 0U) {
			ret = pcie_endpoint_conf_write(ep, cfg->base + PCIE_MSI_MAP1_64,
						       (uint32_t)(address >> 32));
		}
		if (ret == 0) {
			uint32_t offset = (mcr & PCIE_MSI_MCR_64) != 0U ?
				PCIE_MSI_MDR_64 : PCIE_MSI_MDR_32;

			ret = pcie_endpoint_conf_write(ep, cfg->base + offset, data);
		}
		if (ret != 0) {
			return ret;
		}
		for (uint8_t n = count; n > 1U; n >>= 1) {
			mme++;
		}
		mcr = (mcr & ~PCIE_MSI_MCR_MME) | (mme << PCIE_MSI_MCR_MME_SHIFT) | PCIE_MSI_MCR_EN;
	}
	ret = pcie_endpoint_conf_write(ep, cfg->base, mcr);
	if (ret == 0) {
		ret = pcie_endpoint_conf_read(ep, PCIE_CONF_CMDSTAT, &cmd);
	}
	if (ret == 0) {
		ret = pcie_endpoint_conf_write(ep, PCIE_CONF_CMDSTAT,
					       (cmd & 0xffffU) | PCIE_CONF_CMDSTAT_MASTER);
	}
	return ret;
}

bool pcie_msi_enable(pcie_bdf_t bdf, msi_vector_t *vectors, uint8_t n_vector, unsigned int irq)
{
	const struct pcie_endpoint ep = z_pcie_legacy_endpoint(bdf);
	struct msi_config cfg;

	if (vectors == NULL || msi_config_get(&ep, &cfg) != 0) {
		return false;
	}
	for (uint8_t i = 0U; i < n_vector; i++) {
		if (vectors[i].ctrl != NULL && vectors[i].ctrl != ep.ctrl) {
			return false;
		}
	}
	return msi_enable(&ep, &cfg, vectors, n_vector, irq, false) == 0;
}

bool pcie_is_msi(pcie_bdf_t bdf)
{
	const struct pcie_endpoint ep = z_pcie_legacy_endpoint(bdf);
	struct msi_config cfg;

	return msi_config_get(&ep, &cfg) == 0;
}
#endif /* CONFIG_PCIE_CONTROLLER */

int pcie_endpoint_msi_vectors_allocate(const struct pcie_endpoint *ep, unsigned int priority,
				       msi_vector_t *vectors, uint8_t n_vector,
				       uint8_t *n_allocated)
{
	if (n_allocated != NULL) {
		*n_allocated = 0U;
	}
	if (ep == NULL || ep->ctrl == NULL || vectors == NULL || n_vector == 0U ||
	    n_allocated == NULL) {
		return -EINVAL;
	}
#if defined(CONFIG_PCIE_CONTROLLER) && defined(CONFIG_PCIE_MSI_MULTI_VECTOR)
	const struct pcie_ctrl_driver_api *api;
	struct msi_config cfg;
	mm_reg_t mapping = 0U;
	uint8_t allocated = 0U;
	int ret;

	if (!DEVICE_API_IS(pcie_ctrl, ep->ctrl)) {
		return -ENOTSUP;
	}
	api = DEVICE_API_GET(pcie_ctrl, ep->ctrl);
	if (api->msi_device_setup_errno == NULL) {
		return -ENOTSUP;
	}
	ret = msi_config_get(ep, &cfg);
	if (ret != 0) {
		return ret;
	}
	if (!cfg.msix && !is_power_of_two(n_vector)) {
		return -EINVAL;
	}
	if (n_vector > cfg.available) {
		return -ENOSPC;
	}
	/* The output array must not overlap storage owned by an earlier allocation. */
	memset(vectors, 0, n_vector * sizeof(*vectors));
	msi_vectors_init(ep, vectors, n_vector, cfg.msix);
	if (cfg.msix) {
		ret = msi_map_table(ep, cfg.base, vectors, n_vector, &mapping);
		if (ret != 0) {
			goto fail;
		}
	}
	ret = api->msi_device_setup_errno(ep->ctrl, priority, vectors, n_vector, &allocated);
	if ((ret == 0 && allocated != n_vector) || (ret != 0 && allocated != 0U)) {
		/* Ownership is unknown after a backend violates the allocation contract. */
		k_panic();
	}
	if (ret != 0) {
		if (cfg.msix) {
			device_unmap(mapping, n_vector * sizeof(struct msix_vector));
		}
		goto fail;
	}
	*n_allocated = allocated;
	return 0;

fail:
	memset(vectors, 0, n_vector * sizeof(*vectors));
	return ret;
#else
	return -ENOTSUP;
#endif
}

int pcie_endpoint_msi_vector_connect(const struct pcie_endpoint *ep, msi_vector_t *vector,
				     void (*routine)(const void *parameter),
				     const void *parameter, uint32_t flags)
{
	if (ep == NULL || ep->ctrl == NULL || vector == NULL || routine == NULL ||
	    vector->ctrl != ep->ctrl || vector->bdf != ep->bdf) {
		return -EINVAL;
	}
#if defined(CONFIG_PCIE_CONTROLLER) && defined(CONFIG_PCIE_MSI_MULTI_VECTOR)
	struct msi_config cfg;
	int ret = msi_config_get(ep, &cfg);

	if (ret != 0) {
		return ret;
	}
#ifdef CONFIG_PCIE_MSI_X
	if (vector->msix != cfg.msix) {
		return -ENOTSUP;
	}
#endif
	return arch_pcie_msi_vector_connect(vector, routine, parameter, flags) ? 0 : -ENOTSUP;
#else
	return -ENOTSUP;
#endif
}

int pcie_endpoint_msi_enable(const struct pcie_endpoint *ep, msi_vector_t *vectors,
			     uint8_t n_vector, unsigned int irq)
{
	if (ep == NULL || ep->ctrl == NULL || vectors == NULL || n_vector == 0U) {
		return -EINVAL;
	}
	for (uint8_t i = 0U; i < n_vector; i++) {
		if (vectors[i].ctrl != ep->ctrl || vectors[i].bdf != ep->bdf) {
			return -EINVAL;
		}
	}
#if defined(CONFIG_PCIE_CONTROLLER) && defined(CONFIG_PCIE_MSI_MULTI_VECTOR)
	struct msi_config cfg;
	int ret = msi_config_get(ep, &cfg);

	if (ret != 0) {
		return ret;
	}
	return msi_enable(ep, &cfg, vectors, n_vector, irq, true);
#else
	return -ENOTSUP;
#endif
}
