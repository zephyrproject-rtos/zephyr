/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xilinx/AMD USB2 wrapper tuning for integrated DWC3+xHCI (Versal, ZynqMP).
 */

#include <zephyr/logging/log.h>
#include "xhci_dwc3_log.h"
#include <zephyr/sys/util.h>

#include "dwc3_xilinx.h"
#include "dwc3_regs.h"
#include "dwc3_xilinx_regs.h"

LOG_MODULE_DECLARE(uhc_dwc3);

static inline uint32_t dwc3_readl(mm_reg_t base, uint32_t offset)
{
	return sys_read32(base + offset);
}

static inline void dwc3_writel(mm_reg_t base, uint32_t offset, uint32_t val)
{
	sys_write32(val, base + offset);
}

static inline uint32_t dwc3_xilinx_usb2_readl(mm_reg_t usb2_wrapper_base, uint32_t offset)
{
	return sys_read32(usb2_wrapper_base + offset);
}

static inline void dwc3_xilinx_usb2_writel(mm_reg_t usb2_wrapper_base, uint32_t offset,
					   uint32_t val)
{
	sys_write32(val, usb2_wrapper_base + offset);
}

static void dwc3_xilinx_usb2_phy_rst_mask(mm_reg_t usb2_wrapper_base, bool mask)
{
	uint32_t reg = dwc3_xilinx_usb2_readl(usb2_wrapper_base, VERSAL_USB2_PHY_RST);

	if (mask) {
		reg &= ~VERSAL_USB2_PHY_RST_CHK_BIT;
	} else {
		reg |= VERSAL_USB2_PHY_RST_CHK_BIT;
	}
	dwc3_xilinx_usb2_writel(usb2_wrapper_base, VERSAL_USB2_PHY_RST, reg);
}

#define DWC3_XILINX_GFLADJ_WRITABLE(fadj30mhz)                                                     \
	(((fadj30mhz) & DWC3_GFLADJ_30MHZ_MASK) | (0x800U << 8))

static void dwc3_xilinx_apply_gfladj(mm_reg_t dwc3_base, uint32_t fladj30mhz)
{
	uint32_t before = dwc3_readl(dwc3_base, DWC3_GFLADJ);
	uint32_t reg = (before & ~DWC3_GFLADJ_WRITABLE_MASK) |
		       (DWC3_XILINX_GFLADJ_WRITABLE(fladj30mhz) & DWC3_GFLADJ_WRITABLE_MASK);
	uint32_t after;

	if (reg == before) {
		UHC_DWC3_DBG("GFLADJ already xHCI-aligned: 0x%08x", before);
		return;
	}

	dwc3_writel(dwc3_base, DWC3_GFLADJ, reg);
	after = dwc3_readl(dwc3_base, DWC3_GFLADJ);
	UHC_DWC3_DBG("GFLADJ tune before=0x%08x after=0x%08x", before, after);
}

static void dwc3_xilinx_apply_guctl1_host(mm_reg_t dwc3_base)
{
	uint32_t guctl1 = dwc3_readl(dwc3_base, DWC3_GUCTL1);

	if ((guctl1 & DWC3_GUCTL1_DEV_L1_EXIT_BY_HW) != 0U) {
		return;
	}

	guctl1 |= DWC3_GUCTL1_DEV_L1_EXIT_BY_HW;
	dwc3_writel(dwc3_base, DWC3_GUCTL1, guctl1);
}

static void dwc3_xilinx_clear_susphy(const struct dwc3_xilinx_config *cfg, mm_reg_t dwc3_base)
{
	uint32_t reg;

	if (dwc3_xilinx_dwc3_quirk(cfg, DWC3_QUIRK_DIS_U3_SUSPHY)) {
		reg = dwc3_readl(dwc3_base, DWC3_GUSB3PIPECTL(0));
		reg &= ~DWC3_GUSB3PIPECTL_SUSPHY;
		dwc3_writel(dwc3_base, DWC3_GUSB3PIPECTL(0), reg);
	}

	if (dwc3_xilinx_dwc3_quirk(cfg, DWC3_QUIRK_DIS_U2_SUSPHY)) {
		reg = dwc3_readl(dwc3_base, DWC3_GUSB2PHYCFG(0));
		reg &= ~DWC3_GUSB2PHYCFG_SUSPHY;
		dwc3_writel(dwc3_base, DWC3_GUSB2PHYCFG(0), reg);
	}
}

static void dwc3_xilinx_set_coherency_route(mm_reg_t usb2_wrapper_base)
{
	uint32_t reg = dwc3_xilinx_usb2_readl(usb2_wrapper_base, VERSAL_USB2_COHERENCY);

	if ((reg & VERSAL_USB2_COHERENCY_ROUTE) != 0U) {
		return;
	}

	reg |= VERSAL_USB2_COHERENCY_ROUTE;
	dwc3_xilinx_usb2_writel(usb2_wrapper_base, VERSAL_USB2_COHERENCY, reg);
}

static void dwc3_xilinx_apply_jitter_adjust(mm_reg_t usb2_wrapper_base, mm_reg_t dwc3_base)
{
	uint32_t gfl = dwc3_readl(dwc3_base, DWC3_GFLADJ) & DWC3_GFLADJ_30MHZ_MASK;
	uint32_t jit = dwc3_xilinx_usb2_readl(usb2_wrapper_base, VERSAL_USB2_JITTER_ADJUST);
	uint32_t new_jit;

	new_jit = (jit & ~VERSAL_USB2_JITTER_FLADJ_MASK) | (gfl & VERSAL_USB2_JITTER_FLADJ_MASK);

	if (new_jit == jit) {
		return;
	}

	dwc3_xilinx_usb2_writel(usb2_wrapper_base, VERSAL_USB2_JITTER_ADJUST, new_jit);
}

void dwc3_xilinx_pre_host_burst(mm_reg_t usb2_wrapper_base)
{
	if (!dwc3_xilinx_wrapper_present(usb2_wrapper_base)) {
		return;
	}

	dwc3_xilinx_usb2_phy_rst_mask(usb2_wrapper_base, false);
}

void dwc3_xilinx_host_tune_post(const struct dwc3_xilinx_config *cfg, mm_reg_t dwc3_base,
				mm_reg_t usb2_wrapper_base)
{
	uint32_t fladj30mhz;

	if (!dwc3_xilinx_wrapper_present(usb2_wrapper_base)) {
		return;
	}

	fladj30mhz = cfg->fladj;
	if (fladj30mhz == 0U) {
		fladj30mhz = VERSAL_USB2_GFLADJ_30MHZ_DEFAULT;
	}

	dwc3_xilinx_usb2_phy_rst_mask(usb2_wrapper_base, true);
	dwc3_xilinx_apply_gfladj(dwc3_base, fladj30mhz);
	dwc3_xilinx_apply_jitter_adjust(usb2_wrapper_base, dwc3_base);
	dwc3_xilinx_apply_guctl1_host(dwc3_base);
	dwc3_xilinx_clear_susphy(cfg, dwc3_base);
	dwc3_xilinx_set_coherency_route(usb2_wrapper_base);

	UHC_DWC3_DBG("xilinx host tune done wrapper=0x%08x", (unsigned int)usb2_wrapper_base);
}
