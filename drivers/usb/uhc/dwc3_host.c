/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Synopsys DWC3 core host-mode programming before xHCI setup.
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "xhci_dwc3_log.h"
#include <zephyr/sys/util.h>

#include "dwc3_host.h"
#include "dwc3_xilinx.h"
#include "dwc3_regs.h"
#include "xhci_hw.h"

LOG_MODULE_DECLARE(uhc_dwc3);

static inline uint32_t dwc3_readl(mm_reg_t base, uint32_t offset)
{
	return sys_read32(base + offset);
}

static inline void dwc3_writel(mm_reg_t base, uint32_t offset, uint32_t val)
{
	sys_write32(val, base + offset);
}

static inline uint32_t xhci_readl(mm_reg_t base, uint32_t offset)
{
	return sys_read32(base + offset);
}

static inline void xhci_writel(mm_reg_t base, uint32_t offset, uint32_t val)
{
	sys_write32(val, base + offset);
}

void dwc3_host_susphy_post_run(const struct device *dev, bool enable)
{
	const struct uhc_dwc3_config *cfg = dev->config;
	mm_reg_t base = uhc_dwc3_core_mmio(dev);
	uint32_t reg;

	reg = dwc3_readl(base, DWC3_GUSB3PIPECTL(0));
	if (enable && !dwc3_xilinx_dwc3_quirk(&cfg->xilinx, DWC3_QUIRK_DIS_U3_SUSPHY)) {
		reg |= DWC3_GUSB3PIPECTL_SUSPHY;
	} else {
		reg &= ~DWC3_GUSB3PIPECTL_SUSPHY;
	}
	dwc3_writel(base, DWC3_GUSB3PIPECTL(0), reg);

	reg = dwc3_readl(base, DWC3_GUSB2PHYCFG(0));
	if (enable && !dwc3_xilinx_dwc3_quirk(&cfg->xilinx, DWC3_QUIRK_DIS_U2_SUSPHY)) {
		reg |= DWC3_GUSB2PHYCFG_SUSPHY;
	} else {
		reg &= ~DWC3_GUSB2PHYCFG_SUSPHY;
	}
	dwc3_writel(base, DWC3_GUSB2PHYCFG(0), reg);

	UHC_DWC3_DBG("post-xHCI-RUN SUSPHY %s", enable ? "enable" : "disable");
}

int dwc3_host_burst_init(const struct device *dev)
{
	const struct uhc_dwc3_config *cfg = dev->config;
	mm_reg_t base = uhc_dwc3_core_mmio(dev);
	mm_reg_t usb2_wrapper = uhc_dwc3_usb2_wrapper_mmio(dev);
	uint32_t reg;
	uint8_t caplength;
	mm_reg_t op_temp;
	int timeout;

	UHC_DWC3_DBG("DWC3: BURST host init, base 0x%lx", (unsigned long)base);

	dwc3_xilinx_pre_host_burst(usb2_wrapper);

	caplength = XHCI_CAPLENGTH(xhci_readl(base, XHCI_CAP_CAPBASE));
	op_temp = base + caplength;

	reg = xhci_readl(op_temp, XHCI_OP_USBCMD);
	if (reg & XHCI_USBCMD_RUN) {
		reg &= ~XHCI_USBCMD_RUN;
		xhci_writel(op_temp, XHCI_OP_USBCMD, reg);
		k_busy_wait(1000);
	}

	reg = dwc3_readl(base, DWC3_GCTL);
	reg |= DWC3_GCTL_CORESOFTRESET;
	reg &= ~DWC3_GCTL_PRTCAP_MASK;
	reg |= DWC3_GCTL_PRTCAPDIR(DWC3_GCTL_PRTCAP_HOST);
	dwc3_writel(base, DWC3_GCTL, reg);

	dwc3_writel(base, DWC3_GUSB3PIPECTL(0),
		    dwc3_readl(base, DWC3_GUSB3PIPECTL(0)) | DWC3_GUSB3PIPECTL_PHYSOFTRST);
	dwc3_writel(base, DWC3_GUSB2PHYCFG(0),
		    dwc3_readl(base, DWC3_GUSB2PHYCFG(0)) | DWC3_GUSB2PHYCFG_PHYSOFTRST);

	k_busy_wait(5000);

	dwc3_writel(base, DWC3_GUSB3PIPECTL(0),
		    dwc3_readl(base, DWC3_GUSB3PIPECTL(0)) & ~DWC3_GUSB3PIPECTL_PHYSOFTRST);
	dwc3_writel(base, DWC3_GUSB2PHYCFG(0),
		    dwc3_readl(base, DWC3_GUSB2PHYCFG(0)) & ~DWC3_GUSB2PHYCFG_PHYSOFTRST);

	k_busy_wait(5000);

	reg = dwc3_readl(base, DWC3_GCTL);
	reg &= ~DWC3_GCTL_CORESOFTRESET;
	dwc3_writel(base, DWC3_GCTL, reg);

	k_busy_wait(5000);

	caplength = XHCI_CAPLENGTH(xhci_readl(base, XHCI_CAP_CAPBASE));
	op_temp = base + caplength;

	xhci_writel(op_temp, XHCI_OP_USBCMD, XHCI_USBCMD_HCRST);

	timeout = 1000;
	while ((xhci_readl(op_temp, XHCI_OP_USBCMD) & XHCI_USBCMD_HCRST) && timeout > 0) {
		k_busy_wait(100);
		timeout--;
	}
	if (timeout == 0) {
		LOG_ERR("DWC3: HCRST timed out");
		return -ETIMEDOUT;
	}

	reg = dwc3_readl(base, DWC3_OCTL);
	reg &= ~DWC3_OCTL_PERIMODE;
	dwc3_writel(base, DWC3_OCTL, reg);

	UHC_DWC3_DBG("DWC3: host mode init done (GCTL=0x%08x)", dwc3_readl(base, DWC3_GCTL));

	dwc3_xilinx_host_tune_post(&cfg->xilinx, base, usb2_wrapper);

	return 0;
}
