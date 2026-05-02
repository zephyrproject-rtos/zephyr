/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "intc_riscv_aplic_priv.h"

LOG_MODULE_DECLARE(intc_riscv_aplic, CONFIG_LOG_DEFAULT_LEVEL);

int riscv_aplic_msi_route(const struct device *dev, unsigned int src, uint32_t hart, uint32_t eiid)
{
	const struct aplic_cfg *cfg = dev->config;

	if (src == 0 || src > cfg->num_sources) {
		return -EINVAL;
	}

	/* Validate hart parameter - must be valid CPU index */
	if (hart >= CONFIG_MP_MAX_NUM_CPUS) {
		return -EINVAL;
	}

	/* Validate EIID parameter - consistent with IMSIC driver bounds */
	if (eiid == 0 || eiid >= CONFIG_NUM_IRQS) {
		return -EINVAL;
	}

	/* Route for MMSI delivery (memory-mapped MSI write to IMSIC).
	 * TARGET register format (RISC-V AIA spec, section 4.5.4):
	 *   Bits [31:18]: Hart index
	 *   Bit  [11]:    MSI_DEL (0=DMSI, 1=MMSI)
	 *   Bits [10:0]:  EIID
	 */
	uint32_t val = ((hart & APLIC_TARGET_HART_MASK) << APLIC_TARGET_HART_SHIFT) |
		       APLIC_TARGET_MSI_DEL | (eiid & APLIC_TARGET_EIID_MASK);
	wr32(cfg->base, aplic_target_off(src), val);
	return 0;
}

int riscv_aplic_msi_inject_software_interrupt(const struct device *dev, uint32_t eiid,
					      uint32_t hart_id, uint32_t context)
{
	const struct aplic_cfg *cfg = dev->config;

	/* Validate EIID parameter - consistent with IMSIC driver bounds */
	if (eiid == 0 || eiid >= CONFIG_NUM_IRQS) {
		return -EINVAL;
	}

	/* Validate hart_id parameter - must be valid CPU index */
	if (hart_id >= CONFIG_MP_MAX_NUM_CPUS) {
		return -EINVAL;
	}

	/* GENMSI register format (RISC-V AIA spec, section 4.5.5):
	 * - Hart_Index (bits 31:18)
	 * - Context/Guest (bits 17:13) - for DMSI only
	 * - Busy (bit 12) - read-only status
	 * - MSI_DEL (bit 11) - 0=DMSI, 1=MMSI
	 * - EIID (bits 10:0)
	 *
	 * For MMSI delivery, set bit 11 and provide EIID.
	 */
	uint32_t genmsi_val =
		((hart_id & APLIC_GENMSI_HART_MASK) << APLIC_GENMSI_HART_SHIFT) |
		((context & APLIC_GENMSI_CONTEXT_MASK) << APLIC_GENMSI_CONTEXT_SHIFT) |
		APLIC_GENMSI_MMSI_MODE | (eiid & APLIC_GENMSI_EIID_MASK);

	struct aplic_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	wr32(cfg->base, APLIC_GENMSI, genmsi_val);

	k_spin_unlock(&data->lock, key);

	LOG_DBG("GENMSI injection: hart=%u context=%u eiid=%u, wrote=0x%08x readback=0x%08x",
		hart_id, context, eiid, genmsi_val, rd32(cfg->base, APLIC_GENMSI));

	return 0;
}

int aplic_msi_init(const struct device *dev)
{
	const struct aplic_cfg *cfg = dev->config;
	uint32_t imsic_addr = cfg->imsic_addr;

	LOG_DBG("APLIC: Got IMSIC address from DT msi-parent: 0x%08x", imsic_addr);

	/* Configure MSI target address registers per RISC-V AIA spec.
	 * MSIADDRCFG holds the base PAGE NUMBER (address >> 12), not full address!
	 * MSIADDRCFGH holds geometry fields that tell APLIC how to calculate
	 * per-hart IMSIC addresses from the base PPN.
	 *
	 * The APLIC builds the final MSI target address as:
	 *   addr = (base_ppn | hart_bits | guest_bits) << 12
	 *
	 * For SMP with per-hart IMSICs at 4KB offsets:
	 * - Hart 0: base_addr (e.g., 0x24000000)
	 * - Hart 1: base_addr + 0x1000
	 * - Hart N: base_addr + (N * 0x1000)
	 *
	 * Geometry fields (MSIADDRCFGH):
	 * - LHXS: Position of lower hart index bits within PPN
	 * - LHXW: Width of lower hart index field (log2(num_harts))
	 * - HHXS: Position of higher hart index bits
	 * - HHXW: Width of higher hart index field (0 for single group)
	 */
	uint32_t imsic_ppn = imsic_addr >> 12; /* Convert address to page number */
	uint32_t num_harts = CONFIG_MP_MAX_NUM_CPUS;
	uint32_t lhxw = 0;

	if (num_harts > 1) {
		/* Calculate log2(num_harts) for LHXW */
		lhxw = 32 - __builtin_clz(num_harts - 1); /* Bits needed to encode num_harts */
	}

	/* For 4KB IMSIC spacing: Hart index at bit 0 of PPN (bit 12 of phys addr)
	 * This allows Hart N to be at base_ppn + N
	 */
	uint32_t lhxs = 0;  /* Hart index at bit 0 of PPN */
	uint32_t hhxs = 24; /* Higher bits start at 24 (standard) */
	uint32_t hhxw = 0;  /* No higher hart group for uniprocessor/small SMP */
	uint32_t msi_geom =
		(lhxs << APLIC_MSIADDRCFGH_LHXS_SHIFT) | (lhxw << APLIC_MSIADDRCFGH_LHXW_SHIFT) |
		(hhxs << APLIC_MSIADDRCFGH_HHXS_SHIFT) | (hhxw << APLIC_MSIADDRCFGH_HHXW_SHIFT);

	LOG_DBG("SMP MSI geometry: num_harts=%u, LHXS=%u, LHXW=%u, HHXS=%u, HHXW=%u", num_harts,
		lhxs, lhxw, hhxs, hhxw);

	/* Read MSI address registers to check if they're already configured.
	 * Some platforms have writable registers, others have read-only registers
	 * configured via hardware pins or props files.
	 */
	uint32_t msi_low = rd32(cfg->base, APLIC_MSIADDRCFG);
	uint32_t msi_high = rd32(cfg->base, APLIC_MSIADDRCFGH);

	/* If registers read as zero, try writing (writable registers) */
	if (msi_low == 0 && msi_high == 0) {
		LOG_DBG("MSI address registers uninitialized, configuring...");
		wr32(cfg->base, APLIC_MSIADDRCFG, imsic_ppn);
		wr32(cfg->base, APLIC_MSIADDRCFGH, msi_geom);
		wr32(cfg->base, APLIC_SMSIADDRCFG, imsic_ppn);
		wr32(cfg->base, APLIC_SMSIADDRCFGH, msi_geom);
	} else {
		/* Registers already configured (read-only pin/props interface) */
		LOG_DBG("MSI address registers pre-configured (read-only interface)");
	}

	LOG_DBG("APLIC MSI address configuration:");
	LOG_DBG("  Expected IMSIC: 0x%08x (PPN: 0x%08x)", imsic_addr, imsic_ppn);
	LOG_DBG("  MSIADDR:  0x%08x%08x",
		rd32(cfg->base, APLIC_MSIADDRCFGH), rd32(cfg->base, APLIC_MSIADDRCFG));
	LOG_DBG("  SMSIADDR: 0x%08x%08x",
		rd32(cfg->base, APLIC_SMSIADDRCFGH), rd32(cfg->base, APLIC_SMSIADDRCFG));

	/* Enable MSI mode + IE in DOMAINCFG */
	riscv_aplic_domain_enable(dev, true);

	LOG_DBG("APLIC MSI init complete at 0x%lx, sources=%u", (unsigned long)cfg->base,
		cfg->num_sources);
	return 0;
}
