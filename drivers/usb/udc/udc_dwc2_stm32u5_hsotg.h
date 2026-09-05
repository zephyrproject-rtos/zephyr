/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_STM32U5_HSOTG_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_STM32U5_HSOTG_H

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

/* HAL SYSCFG for the embedded HS PHY, LL PWR for the USB power supply */
#include <soc.h>
#include <stm32_ll_pwr.h>

struct udc_dwc2_stm32u5_config {
	const struct device *clk_dev;
	const struct stm32_pclken *pclken;
	const struct stm32_pclken *phy_pclken;
	size_t phy_pclken_len;
	uint32_t phy_ref_clk;
};

/* Enable the USB power supply (mirrors the STM32U5 path in stm32_usb_pwr_enable()) */
static inline int udc_dwc2_stm32u5_enable_pwr(void)
{
	if (LL_PWR_GetRegulVoltageScaling() < LL_PWR_REGU_VOLTAGE_SCALE2) {
		return -EIO;
	}

	LL_PWR_EnableVddUSB();
	LL_PWR_EnableUSBPowerSupply();
	LL_PWR_EnableUSBEPODBooster();
	while (LL_PWR_IsActiveFlag_USBBOOST() != 1) {
		/* Wait for the USB EPOD booster to be ready */
	}

	return 0;
}

static inline int udc_dwc2_stm32u5_enable_clk(const struct udc_dwc2_stm32u5_config *const cfg)
{
	if (!device_is_ready(cfg->clk_dev)) {
		return -ENODEV;
	}

	return clock_control_on(cfg->clk_dev, (void *)&cfg->pclken[0]);
}

/*
 * Bring up the embedded High-Speed PHY
 * (mirrors the drivers/usb/common/stm32/phy_u5otghs.c sequence)
 */
static inline int udc_dwc2_stm32u5_enable_phy(const struct udc_dwc2_stm32u5_config *const cfg)
{
	int ret;

	/* SYSCFG holds the OTG_HS PHY configuration registers */
	__HAL_RCC_SYSCFG_CLK_ENABLE();

	/* Select the PHY reference clock frequency */
	HAL_SYSCFG_SetOTGPHYReferenceClockSelection(cfg->phy_ref_clk);

	/* Deassert the PHY reset */
	HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);

	if (cfg->phy_pclken_len > 1) {
		/* Select the PHY clock source */
		ret = clock_control_configure(cfg->clk_dev, (void *)&cfg->phy_pclken[1], NULL);
		if (ret) {
			return ret;
		}
	}

	/* Turn on the PHY clock */
	return clock_control_on(cfg->clk_dev, (void *)&cfg->phy_pclken[0]);
}

static inline void udc_dwc2_stm32u5_disable_phy(const struct udc_dwc2_stm32u5_config *const cfg)
{
	(void)clock_control_off(cfg->clk_dev, (void *)&cfg->phy_pclken[0]);
}

static inline void udc_dwc2_stm32u5_disable_clk(const struct udc_dwc2_stm32u5_config *const cfg)
{
	(void)clock_control_off(cfg->clk_dev, (void *)&cfg->pclken[0]);
}

static inline void udc_dwc2_stm32u5_disable_pwr(void)
{
	LL_PWR_DisableUSBEPODBooster();
	while (LL_PWR_IsActiveFlag_USBBOOST() != 0) {
		/* Wait for the USB EPOD booster to turn off */
	}

	LL_PWR_DisableUSBPowerSupply();
	LL_PWR_DisableVddUSB();
}

static inline int udc_dwc2_stm32u5_caps(const struct device *const dev)
{
	struct udc_data *const data = dev->data;

	data->caps.hs = true;

	return 0;
}

/*
 * Disable the controller's hardware VBUS detection and pulldown resistors and
 * force the B-session valid override so the device connects.
 */
static inline int udc_dwc2_stm32u5_post_enable(const struct device *const dev)
{
	struct usb_dwc2_reg *const base = dwc2_get_base(dev);
	const mem_addr_t ggpio = (mem_addr_t)&base->ggpio;

	sys_clear_bits(ggpio, USB_OTG_GCCFG_PULLDOWNEN | USB_OTG_GCCFG_VBDEN);
	sys_set_bits(ggpio, USB_OTG_GCCFG_VBVALEXTOEN | USB_OTG_GCCFG_VBVALOVAL);

	return 0;
}

/*
 * The PHY node holds the reference clock selection and the PHY clocks. The
 * SYSCFG_OTG_HS_PHY_CLK_SELECT_<n> values go from 1 to N, whereas DT_ENUM_IDX()
 * is zero-based, hence the UTIL_INC().
 */
#define UDC_DWC2_STM32U5_PHY_NODE(n)	DT_INST_PHANDLE(n, phys)

#define UDC_DWC2_STM32U5_PHY_REF_CLK(n)							\
	CONCAT(SYSCFG_OTG_HS_PHY_CLK_SELECT_,						\
	       UTIL_INC(DT_ENUM_IDX(UDC_DWC2_STM32U5_PHY_NODE(n), clock_reference)))

#define QUIRK_STM32U5_HSOTG_DEFINE(n)							\
	static const struct stm32_pclken pclken_##n[] = STM32_DT_INST_CLOCKS(n);	\
	static const struct stm32_pclken phy_pclken_##n[] =				\
		STM32_DT_CLOCKS(UDC_DWC2_STM32U5_PHY_NODE(n));				\
											\
	static const struct udc_dwc2_stm32u5_config udc_dwc2_stm32u5_config_##n = {	\
		.clk_dev = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),			\
		.pclken = pclken_##n,							\
		.phy_pclken = phy_pclken_##n,						\
		.phy_pclken_len = DT_NUM_CLOCKS(UDC_DWC2_STM32U5_PHY_NODE(n)),		\
		.phy_ref_clk = UDC_DWC2_STM32U5_PHY_REF_CLK(n),				\
	};										\
											\
	static int udc_dwc2_stm32u5_pre_enable_##n(const struct device *dev)		\
	{										\
		int ret;								\
											\
		ARG_UNUSED(dev);							\
											\
		ret = udc_dwc2_stm32u5_enable_pwr();					\
		if (ret) {								\
			return ret;							\
		}									\
											\
		ret = udc_dwc2_stm32u5_enable_clk(&udc_dwc2_stm32u5_config_##n);	\
		if (ret) {								\
			return ret;							\
		}									\
											\
		return udc_dwc2_stm32u5_enable_phy(&udc_dwc2_stm32u5_config_##n);	\
	}										\
											\
	static int udc_dwc2_stm32u5_shutdown_##n(const struct device *dev)		\
	{										\
		ARG_UNUSED(dev);							\
											\
		udc_dwc2_stm32u5_disable_phy(&udc_dwc2_stm32u5_config_##n);		\
		udc_dwc2_stm32u5_disable_clk(&udc_dwc2_stm32u5_config_##n);		\
		udc_dwc2_stm32u5_disable_pwr();						\
											\
		return 0;								\
	}										\
											\
	const struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {			\
		.caps = udc_dwc2_stm32u5_caps,						\
		.pre_enable = udc_dwc2_stm32u5_pre_enable_##n,				\
		.post_enable = udc_dwc2_stm32u5_post_enable,				\
		.shutdown = udc_dwc2_stm32u5_shutdown_##n,				\
	};

/* Define the quirks only for instances that use this compatible */
#define QUIRK_STM32U5_HSOTG_DEFINE_COMPAT(n)						\
	IF_ENABLED(DT_NODE_HAS_COMPAT(DT_DRV_INST(n), st_stm32u5_hsotg),		\
		   (QUIRK_STM32U5_HSOTG_DEFINE(n)))

DT_INST_FOREACH_STATUS_OKAY(QUIRK_STM32U5_HSOTG_DEFINE_COMPAT)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_STM32U5_HSOTG_H */
