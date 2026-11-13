/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UHC_DWC2_STM32N6_HSOTG_H
#define ZEPHYR_DRIVERS_USB_UHC_DWC2_STM32N6_HSOTG_H

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

#include <usb_dwc2_hw.h>

#include "stm32_usb_common.h"

/*
 * STM32N6 OTG_HS host quirk.
 *
 * The controller is a DWC2 core behind an embedded high-speed PHY (USBPHYC).
 * This quirk enables the supply, the controller clock and the PHY, in that
 * order, and applies the host-mode settings the generic driver does not know
 * about.
 */

/* USBPHYC control register */
#define UHC_DWC2_STM32N6_CR_RETENABLEN1 BIT(0)
#define UHC_DWC2_STM32N6_CR_CMN         BIT(2)
#define UHC_DWC2_STM32N6_CR_FSEL_MASK   GENMASK(6, 4)
#define UHC_DWC2_STM32N6_CR_FSEL_24MHZ  (0x2U << 4)
#define UHC_DWC2_STM32N6_CR_OTGDISABLE0 BIT(16)

/* GCCFG, which the generic register map calls GGPIO */
#define UHC_DWC2_STM32N6_GCCFG_PDETEN      BIT(20)
#define UHC_DWC2_STM32N6_GCCFG_SDETEN      BIT(22)
#define UHC_DWC2_STM32N6_GCCFG_VBVALOVAL   BIT(23)
#define UHC_DWC2_STM32N6_GCCFG_VBVALEXTOEN BIT(24)
#define UHC_DWC2_STM32N6_GCCFG_PULLDOWNEN  BIT(25)

struct uhc_dwc2_stm32n6_config {
	const struct device *clk_dev;
	const struct stm32_pclken *pclken;
	size_t pclken_len;
	const struct stm32_pclken *phy_pclken;
	size_t phy_pclken_len;
	mem_addr_t phy_base;
	/* External VBUS power-switch enable, optional */
	struct gpio_dt_spec vbus_gpio;
};

/* To satisfy the core quirk_data reference */
struct uhc_dwc2_stm32n6_data {
	uint32_t reserved;
};

/*
 * Enable the controller clock and the PHY.
 *
 * The core soft reset performed later in uhc_dwc2_init() only completes with a
 * running PHY clock, so this has to happen before it. The USBPHYC register
 * interface is gated by the controller clock, so that one comes first.
 */
static inline int uhc_dwc2_stm32n6_pre_init(const struct device *const dev)
{
	const struct uhc_dwc2_stm32n6_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);
	uint32_t cr;
	int ret;

	ret = stm32_usb_pwr_enable();
	if (ret != 0) {
		return ret;
	}

	if (!device_is_ready(cfg->clk_dev)) {
		return -ENODEV;
	}

	if (cfg->pclken_len > 1) {
		ret = clock_control_configure(cfg->clk_dev, (void *)&cfg->pclken[1], NULL);
		if (ret != 0) {
			return ret;
		}
	}

	ret = clock_control_on(cfg->clk_dev, (void *)&cfg->pclken[0]);
	if (ret != 0) {
		return ret;
	}

	if (cfg->phy_pclken_len > 1) {
		ret = clock_control_configure(cfg->clk_dev, (void *)&cfg->phy_pclken[1], NULL);
		if (ret != 0) {
			return ret;
		}
	}

	/* TODO: derive the input frequency selection from the PHY clock rate */
	cr = sys_read32(cfg->phy_base);
	cr &= ~UHC_DWC2_STM32N6_CR_FSEL_MASK;
	cr |= UHC_DWC2_STM32N6_CR_FSEL_24MHZ | UHC_DWC2_STM32N6_CR_OTGDISABLE0 |
	      UHC_DWC2_STM32N6_CR_CMN | UHC_DWC2_STM32N6_CR_RETENABLEN1;
	sys_write32(cr, cfg->phy_base);

	ret = clock_control_on(cfg->clk_dev, (void *)&cfg->phy_pclken[0]);
	if (ret != 0) {
		return ret;
	}

	/* Let the PHY PLL settle before the core is reset */
	k_msleep(2);

	return 0;
}

/*
 * Host mode needs the PHY's D+/D- pull-downs. The VBUS override stays off
 * because the board supplies VBUS on its own, and the charger detection is a
 * device-mode feature.
 */
static inline void uhc_dwc2_stm32n6_host_cfg(const struct device *const dev)
{
	struct usb_dwc2_reg *const base = uhc_dwc2_get_base(dev);
	uint32_t gccfg;
	uint32_t gahbcfg;

	gahbcfg = sys_read32((mem_addr_t)&base->gahbcfg);
	gahbcfg &= ~USB_DWC2_GAHBCFG_HBSTLEN_MASK;
	gahbcfg |= usb_dwc2_set_gahbcfg_hbstlen(USB_DWC2_GAHBCFG_HBSTLEN_INCR4);
	sys_write32(gahbcfg, (mem_addr_t)&base->gahbcfg);

	gccfg = sys_read32((mem_addr_t)&base->ggpio);
	gccfg |= UHC_DWC2_STM32N6_GCCFG_PULLDOWNEN;
	gccfg &= ~(UHC_DWC2_STM32N6_GCCFG_VBVALOVAL | UHC_DWC2_STM32N6_GCCFG_VBVALEXTOEN |
		   UHC_DWC2_STM32N6_GCCFG_PDETEN | UHC_DWC2_STM32N6_GCCFG_SDETEN);
	sys_write32(gccfg, (mem_addr_t)&base->ggpio);
}

static inline int uhc_dwc2_stm32n6_pre_enable(const struct device *const dev)
{
	uhc_dwc2_stm32n6_host_cfg(dev);

	return 0;
}

/*
 * The core is soft reset between the two enable hooks, so apply the host
 * configuration again, then switch VBUS on for the attached device.
 */
static inline int uhc_dwc2_stm32n6_post_enable(const struct device *const dev)
{
	const struct uhc_dwc2_stm32n6_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);

	uhc_dwc2_stm32n6_host_cfg(dev);

	if (cfg->vbus_gpio.port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->vbus_gpio)) {
		return -ENODEV;
	}

	return gpio_pin_configure_dt(&cfg->vbus_gpio, GPIO_OUTPUT_ACTIVE);
}

static inline int uhc_dwc2_stm32n6_disable(const struct device *const dev)
{
	const struct uhc_dwc2_stm32n6_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);

	if (cfg->vbus_gpio.port != NULL) {
		(void)gpio_pin_set_dt(&cfg->vbus_gpio, 0);
	}

	return 0;
}

static inline int uhc_dwc2_stm32n6_shutdown(const struct device *const dev)
{
	const struct uhc_dwc2_stm32n6_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);

	(void)clock_control_off(cfg->clk_dev, (void *)&cfg->phy_pclken[0]);

	return clock_control_off(cfg->clk_dev, (void *)&cfg->pclken[0]);
}

#define UHC_DWC2_STM32N6_PHY(n) DT_INST_PHANDLE(n, phys)

#define QUIRK_STM32N6_HSOTG_DEFINE(n)							\
	static const struct stm32_pclken uhc_dwc2_pclken_##n[] =			\
		STM32_DT_INST_CLOCKS(n);						\
	static const struct stm32_pclken uhc_dwc2_phy_pclken_##n[] =			\
		STM32_DT_CLOCKS(UHC_DWC2_STM32N6_PHY(n));				\
											\
	static const struct uhc_dwc2_stm32n6_config uhc_dwc2_quirk_config_##n = {	\
		.clk_dev = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),			\
		.pclken = uhc_dwc2_pclken_##n,						\
		.pclken_len = DT_INST_NUM_CLOCKS(n),					\
		.phy_pclken = uhc_dwc2_phy_pclken_##n,					\
		.phy_pclken_len = DT_NUM_CLOCKS(UHC_DWC2_STM32N6_PHY(n)),		\
		.phy_base = (mem_addr_t)DT_REG_ADDR(UHC_DWC2_STM32N6_PHY(n)),		\
		.vbus_gpio = GPIO_DT_SPEC_INST_GET_OR(n, vbus_gpios, {0}),		\
	};										\
											\
	static struct uhc_dwc2_stm32n6_data uhc_dwc2_quirk_data_##n;			\
											\
	static const struct uhc_dwc2_vendor_quirks uhc_dwc2_vendor_quirks_##n = {	\
		.pre_init = uhc_dwc2_stm32n6_pre_init,					\
		.pre_enable = uhc_dwc2_stm32n6_pre_enable,				\
		.post_enable = uhc_dwc2_stm32n6_post_enable,				\
		.disable = uhc_dwc2_stm32n6_disable,					\
		.shutdown = uhc_dwc2_stm32n6_shutdown,					\
	};

/* Define the quirks only for instances that use this compatible */
#define QUIRK_STM32N6_HSOTG_DEFINE_COMPAT(n)						\
	IF_ENABLED(DT_NODE_HAS_COMPAT(DT_DRV_INST(n), st_stm32n6_hsotg),		\
		   (QUIRK_STM32N6_HSOTG_DEFINE(n)))

DT_INST_FOREACH_STATUS_OKAY(QUIRK_STM32N6_HSOTG_DEFINE_COMPAT)

#endif /* ZEPHYR_DRIVERS_USB_UHC_DWC2_STM32N6_HSOTG_H */
