/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_INFINEON_USBHS_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_INFINEON_USBHS_H

#include <cy_pdl.h>
#include <cy_sysclk.h>

struct usb_ifx_mmio_config {
	int peri_inst;
	int grp_num;
	int usb_bit_pos;
	int clk_src;
	int clk_div;
};

static int usbhs_ifx_phy_enable(const struct device *dev)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	/* Cast to the IFX USB regmap to access the wrapper registers. This
	 * regmap supersets usb_dwc2_reg
	 */
	USBHS_Type *wrapper = (USBHS_Type *)base;

	/* There's no IFX driver for this - configure registers directly */
	wrapper->SS.SUBSYSTEM_CTL &= ~USBHS_SS_SUBSYSTEM_CTL_USB_MODE_Msk;
	wrapper->SS.SUBSYSTEM_CTL |= USBHS_SS_SUBSYSTEM_CTL_AHB_MASTER_SYNC_Msk;
	wrapper->SS.SUBSYSTEM_CTL &= ~USBHS_SS_SUBSYSTEM_CTL_USB_CTRL_SCALEDOWN_MODE_Msk;
	wrapper->SS.SUBSYSTEM_CTL |= USBHS_SS_SUBSYSTEM_CTL_SS_ENABLE_Msk;
	/* SUBSYSTEM_CTL must be configured and enabled prior to configuring other
	 * subsystem registers
	 */
	wrapper->SS.PHY_FUNC_CTL_2 |= USBHS_SS_PHY_FUNC_CTL_2_EFUSE_SEL_Msk;
	wrapper->SS.PHY_FUNC_CTL_2 |= USBHS_SS_PHY_FUNC_CTL_2_RES_TUNING_SEL_Msk;

#if IS_ENABLED(CONFIG_UDC_DWC2_DMA) && IS_ENABLED(CONFIG_TRUSTED_EXECUTION_SECURE)
	/* USB's internal DMA bus has a default security state of non-secure.  If we're
	 * using USB with DMA on a secure device, we must update this to secure for the
	 * DMA to access the UDC buffers.
	 */
	uint32_t *usb_dma_bus_ctl = (uint32_t *)MS_CTL_MS15;
	*usb_dma_bus_ctl &= ~BIT(1); /* 1 = NS, 0 = S */
#endif

	return 0;
}

static int usbhs_ifx_phy_disable(const struct device *dev)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	/* Cast to the IFX USB regmap to access the wrapper registers. This
	 * regmap supersets usb_dwc2_reg
	 */
	USBHS_Type *wrapper = (USBHS_Type *)base;

	wrapper->SS.SUBSYSTEM_CTL &= ~USBHS_SS_SUBSYSTEM_CTL_SS_ENABLE_Msk;
	return 0;
}

static inline int usbhs_ifx_set_caps(const struct device *dev)
{
	struct udc_data *data = dev->data;

	data->caps.hs = true;
	return 0;
}

#define QUIRK_INFINEON_USBHS_DEFINE(n)                                                             \
                                                                                                   \
	static const struct usb_ifx_mmio_config usb_ifx_mmio_##n = {                               \
		.peri_inst = DT_INST_PROP(n, mmio_peri_inst),                                      \
		.grp_num = DT_INST_PROP(n, mmio_peri_group),                                       \
		.usb_bit_pos = DT_INST_PROP(n, mmio_reg_bit_pos),                                  \
		.clk_src = DT_INST_PROP(n, mmio_peri_hf_src),                                      \
		.clk_div = DT_INST_PROP(n, mmio_peri_div),                                         \
	};                                                                                         \
                                                                                                   \
	static int usbhs_ifx_mmio_init_##n(const struct device *dev)                               \
	{                                                                                          \
		Cy_SysClk_PeriGroupSlaveInit(usb_ifx_mmio_##n.peri_inst, usb_ifx_mmio_##n.grp_num, \
					     usb_ifx_mmio_##n.usb_bit_pos,                         \
					     usb_ifx_mmio_##n.clk_src);                            \
		Cy_SysClk_PeriGroupSetDivider(((usb_ifx_mmio_##n.peri_inst) << 8) |                \
						      (usb_ifx_mmio_##n.grp_num),                  \
					      (usb_ifx_mmio_##n.clk_div));                         \
		return 0;                                                                          \
	}                                                                                          \
                                                                                                   \
	const struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {                                 \
		.init = usbhs_ifx_mmio_init_##n,                                                   \
		.pre_enable = usbhs_ifx_phy_enable,                                                \
		.disable = usbhs_ifx_phy_disable,                                                  \
		.caps = usbhs_ifx_set_caps,                                                        \
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_INFINEON_USBHS_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_INFINEON_USBHS_H */
