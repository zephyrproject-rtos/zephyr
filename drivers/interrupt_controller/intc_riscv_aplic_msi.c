/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT riscv_aplic_msi

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/interrupt_controller/riscv_aplic.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(intc_riscv_aplic_msi, CONFIG_LOG_DEFAULT_LEVEL);

struct aplic_cfg {
	uintptr_t base;
	uint32_t num_sources;
};

/*
 * No runtime data struct needed - APLIC-MSI driver is stateless.
 * All configuration is handled through direct register access.
 */

static inline uint32_t rd32(uintptr_t base, uint32_t off)
{
	return sys_read32(base + off);
}

static inline void wr32(uintptr_t base, uint32_t off, uint32_t v)
{
	sys_write32(v, base + off);
}

int riscv_aplic_domain_enable(const struct device *dev, bool enable)
{
	const struct aplic_cfg *cfg = dev->config;
	uint32_t v = rd32(cfg->base, APLIC_DOMAINCFG);

	if (enable) {
		/* Enable domain; keep delivery mode as configured by platform.
		 * Do not modify DM bit - respect hardware/props configuration.
		 */
		v |= APLIC_DOMAINCFG_IE;
	} else {
		v &= ~APLIC_DOMAINCFG_IE;
	}
	wr32(cfg->base, APLIC_DOMAINCFG, v);

	LOG_DBG("APLIC DOMAINCFG: wrote 0x%08x", v);

	return 0;
}

int riscv_aplic_config_src(const struct device *dev, unsigned int src, unsigned int sm)
{
	const struct aplic_cfg *cfg = dev->config;

	if (src == 0 || src > cfg->num_sources) {
		return -EINVAL;
	}
	/* Validate sm parameter - 0x2 and 0x3 are reserved (ARC-V APLIC TRM Table 6-24) */
	if (sm == 0x2 || sm == 0x3) {
		return -EINVAL;
	}
	uintptr_t off = aplic_sourcecfg_off(src);
	uint32_t v = rd32(cfg->base, off);

	v &= ~APLIC_SOURCECFG_SM_MASK; /* Clear source mode field */
	v |= (sm & APLIC_SOURCECFG_SM_MASK);
	wr32(cfg->base, off, v);
	return 0;
}

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
	 * TARGET register format (ARC-V APLIC):
	 *   Bits [31:18]: Hart index
	 *   Bit  [11]:    MSI_DEL (0=DMSI, 1=MMSI)
	 *   Bits [10:0]:  EIID
	 */
	uint32_t val = ((hart & APLIC_TARGET_HART_MASK) << APLIC_TARGET_HART_SHIFT) |
		       APLIC_TARGET_MSI_DEL | (eiid & APLIC_TARGET_EIID_MASK);
	wr32(cfg->base, aplic_target_off(src), val);
	return 0;
}

int riscv_aplic_enable_src(const struct device *dev, unsigned int src, bool enable)
{
	const struct aplic_cfg *cfg = dev->config;

	if (src == 0 || src > cfg->num_sources) {
		return -EINVAL;
	}

	wr32(cfg->base, enable ? APLIC_SETIENUM : APLIC_CLRIENUM, src);
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

	/* GENMSI register format (ARC-V APLIC TRM Table 6-37):
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

	wr32(cfg->base, APLIC_GENMSI, genmsi_val);

	uint32_t readback = rd32(cfg->base, APLIC_GENMSI);

	LOG_DBG("GENMSI injection: hart=%u context=%u eiid=%u, wrote=0x%08x readback=0x%08x",
		hart_id, context, eiid, genmsi_val, readback);

	return 0;
}

static int aplic_msi_init(const struct device *dev)
{
	const struct aplic_cfg *cfg = dev->config;

	/* Get IMSIC address from device tree msi-parent property */
#if !DT_INST_NODE_HAS_PROP(0, msi_parent)
#error "APLIC node must have msi-parent property pointing to IMSIC device"
#endif

	uint32_t imsic_addr = DT_REG_ADDR(DT_INST_PHANDLE(0, msi_parent));

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

		/* Read back after write */
		msi_low = rd32(cfg->base, APLIC_MSIADDRCFG);
		msi_high = rd32(cfg->base, APLIC_MSIADDRCFGH);
	} else {
		/* Registers already configured (read-only pin/props interface) */
		LOG_DBG("MSI address registers pre-configured (read-only interface)");
	}

	uint32_t smsi_low = rd32(cfg->base, APLIC_SMSIADDRCFG);
	uint32_t smsi_high = rd32(cfg->base, APLIC_SMSIADDRCFGH);

	LOG_DBG("APLIC MSI address configuration:");
	LOG_DBG("  Expected IMSIC: 0x%08x (PPN: 0x%08x)", imsic_addr, imsic_ppn);
	LOG_DBG("  MSIADDR:  0x%08x%08x", msi_high, msi_low);
	LOG_DBG("  SMSIADDR: 0x%08x%08x", smsi_high, smsi_low);

	/* Enable MSI mode + IE in DOMAINCFG */
	riscv_aplic_domain_enable(dev, true);

	LOG_DBG("APLIC MSI init complete at 0x%lx, sources=%u", (unsigned long)cfg->base,
		cfg->num_sources);
	return 0;
}

#define APLIC_INIT(inst)                                                                           \
	static const struct aplic_cfg aplic_cfg_##inst = {                                         \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.num_sources = DT_INST_PROP(inst, riscv_num_sources),                              \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, aplic_msi_init, NULL, NULL, &aplic_cfg_##inst, PRE_KERNEL_1,   \
			      CONFIG_INTC_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(APLIC_INIT)

/* Global device pointer for easy access */
static const struct device *aplic_device;

/* Zephyr integration functions */
const struct device *riscv_aplic_get_dev(void)
{
	if (!aplic_device) {
		aplic_device = DEVICE_DT_GET_ANY(riscv_aplic_msi);
	}
	return aplic_device;
}

uint32_t riscv_aplic_get_num_sources(const struct device *dev)
{
	if (!dev) {
		return 0;
	}
	const struct aplic_cfg *cfg = dev->config;

	return cfg->num_sources;
}
