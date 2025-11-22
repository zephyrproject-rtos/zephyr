/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_APLIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_RISCV_APLIC_H_

#include <zephyr/device.h>
#include <zephyr/types.h>

/* APLIC (MSI mode) register offsets (per TRM) */
#define APLIC_DOMAINCFG      0x0000
#define APLIC_SOURCECFG_BASE 0x0004
#define APLIC_SETIP_BASE     0x1C00
#define APLIC_SETIPNUM       0x1CDC
#define APLIC_CLRIP_BASE     0x1D00
#define APLIC_CLRIPNUM       0x1DDC
#define APLIC_SETIE_BASE     0x1E00
#define APLIC_SETIENUM       0x1EDC
#define APLIC_CLRIE_BASE     0x1F00
#define APLIC_CLRIENUM       0x1FDC
#define APLIC_MSIADDRCFG     0x1BC0
#define APLIC_MSIADDRCFGH    0x1BC4
#define APLIC_SMSIADDRCFG    0x1BC8
#define APLIC_SMSIADDRCFGH   0x1BCC
#define APLIC_GENMSI         0x3000
#define APLIC_TARGET_BASE    0x3004

/* domaincfg bits */
#define APLIC_DOMAINCFG_IE BIT(8)
#define APLIC_DOMAINCFG_DM BIT(2)
#define APLIC_DOMAINCFG_BE BIT(0)

/* MSIADDRCFGH geometry fields - used by APLIC to calculate per-hart MSI addresses */
#define APLIC_MSIADDRCFGH_L_BIT      31 /* Lock bit */
#define APLIC_MSIADDRCFGH_HHXS_SHIFT 24 /* Higher Hart Index Shift */
#define APLIC_MSIADDRCFGH_HHXS_MASK  0x1F
#define APLIC_MSIADDRCFGH_LHXS_SHIFT 20 /* Lower Hart Index Shift */
#define APLIC_MSIADDRCFGH_LHXS_MASK  0x7
#define APLIC_MSIADDRCFGH_HHXW_SHIFT 16 /* Higher Hart Index Width */
#define APLIC_MSIADDRCFGH_HHXW_MASK  0x7
#define APLIC_MSIADDRCFGH_LHXW_SHIFT 12 /* Lower Hart Index Width */
#define APLIC_MSIADDRCFGH_LHXW_MASK  0xF
#define APLIC_MSIADDRCFGH_BAPPN_MASK 0xFFF /* Upper address bits */

/* sourcecfg bits */
#define APLIC_SOURCECFG_D_BIT   10
#define APLIC_SOURCECFG_SM_MASK 0x7 /* Source mode field mask (bits 2:0) */
#define APLIC_SM_INACTIVE       0x0
#define APLIC_SM_DETACHED       0x1
#define APLIC_SM_EDGE_RISE      0x4
#define APLIC_SM_EDGE_FALL      0x5
#define APLIC_SM_LEVEL_HIGH     0x6
#define APLIC_SM_LEVEL_LOW      0x7

/* TARGET register fields (for MSI routing) - ARC-V APLIC format */
#define APLIC_TARGET_HART_SHIFT 18      /* Hart index starts at bit 18 */
#define APLIC_TARGET_HART_MASK  0x3FFF  /* 14-bit hart index (bits 31:18) */
#define APLIC_TARGET_MSI_DEL    BIT(11) /* MSI delivery mode: 0=DMSI, 1=MMSI */
#define APLIC_TARGET_EIID_MASK  0x7FF   /* 11-bit EIID (bits 10:0) */

/* GENMSI register fields (for software-triggered MSI) - ARC-V APLIC TRM Table 6-37 */
#define APLIC_GENMSI_HART_SHIFT    18      /* Hart index starts at bit 18 */
#define APLIC_GENMSI_HART_MASK     0x3FFF  /* 14-bit hart index (bits 31:18) */
#define APLIC_GENMSI_CONTEXT_SHIFT 13      /* Context/Guest field (bits 17:13) */
#define APLIC_GENMSI_CONTEXT_MASK  0x1F    /* 5-bit context field (for DMSI) */
#define APLIC_GENMSI_BUSY          BIT(12) /* Busy bit (read-only status) */
#define APLIC_GENMSI_MMSI_MODE     BIT(11) /* MSI delivery: 0=DMSI, 1=MMSI */
#define APLIC_GENMSI_EIID_MASK     0x7FF   /* 11-bit EIID (bits 10:0) */

static inline uint32_t aplic_sourcecfg_off(unsigned int src)
{
	return APLIC_SOURCECFG_BASE + (src - 1U) * 4U;
}

static inline uint32_t aplic_target_off(unsigned int src)
{
	return APLIC_TARGET_BASE + (src - 1U) * 4U;
}

/* Forward declarations */
const struct device *riscv_aplic_get_dev(void);

/* APLIC API functions (implemented by drivers) */
int riscv_aplic_msi_global_enable(const struct device *dev, bool enable);
int riscv_aplic_msi_config_src(const struct device *dev, unsigned int src, unsigned int sm);
int riscv_aplic_msi_route(const struct device *dev, unsigned int src, uint32_t hart, uint32_t eiid);
int riscv_aplic_msi_enable_src(const struct device *dev, unsigned int src, bool enable);
int riscv_aplic_inject_software_interrupt(const struct device *dev, uint32_t eiid, uint32_t hart_id,
					  uint32_t context);

/* Convenience wrappers */
static inline void riscv_aplic_enable_source(unsigned int src)
{
	const struct device *dev = riscv_aplic_get_dev();

	if (dev) {
		riscv_aplic_msi_enable_src(dev, src, true);
	}
}

static inline void riscv_aplic_disable_source(unsigned int src)
{
	const struct device *dev = riscv_aplic_get_dev();

	if (dev) {
		riscv_aplic_msi_enable_src(dev, src, false);
	}
}

static inline void riscv_aplic_inject_genmsi(uint32_t hart, uint32_t eiid)
{
	const struct device *dev = riscv_aplic_get_dev();

	if (dev) {
		riscv_aplic_inject_software_interrupt(dev, eiid, hart, 0);
	}
}

#endif
