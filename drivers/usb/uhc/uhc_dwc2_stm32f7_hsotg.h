/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UHC_DWC2_STM32F7_HSOTG_H
#define ZEPHYR_DRIVERS_USB_UHC_DWC2_STM32F7_HSOTG_H

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <usb_dwc2_hw.h>

/* Include LL clock control for the USBPHYC/ULPI clocks */
#include <stm32_ll_bus.h>

/* STM32F7 OTG_HS host quirk configuration */
struct uhc_dwc2_stm32f7_config {
	const struct device *clk_dev;
	const struct stm32_pclken *pclken;
	size_t pclken_len;
	/* ULPI pin control, NULL for the embedded HS PHY */
	const struct pinctrl_dev_config *pcfg;
	/* External ULPI PHY reset, optional */
	struct gpio_dt_spec phy_reset_gpio;
	/* External VBUS power-switch enable, optional */
	struct gpio_dt_spec vbus_gpio;
};

/* To satisfy the core quirk_data reference */
struct uhc_dwc2_stm32f7_data {
	uint32_t reserved;
};

static inline struct usb_dwc2_reg *uhc_dwc2_stm32f7_base(const struct device *const dev)
{
	const struct uhc_dwc2_config *const config = dev->config;

	return config->base;
}

static inline int uhc_dwc2_stm32f7_enable_clk(const struct uhc_dwc2_stm32f7_config *const cfg)
{
	int ret;

	if (!device_is_ready(cfg->clk_dev)) {
		return -ENODEV;
	}

	if (cfg->pclken_len > 1) {
		/* Select the peripheral clock source (kernel clock) */
		ret = clock_control_configure(cfg->clk_dev, (void *)&cfg->pclken[1], NULL);
		if (ret) {
			return ret;
		}
	}

	return clock_control_on(cfg->clk_dev, (void *)&cfg->pclken[0]);
}

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_usbphyc)

/* Embedded USBPHYC configuration */

#include <soc.h>

/*
 * USBPHYC PLL input frequency selection, derived from the HSE frequency.
 * The field values mirror the HAL USB_HS_PHYCInit() runtime code (0/2/3/4/5/7
 * for 12/12.5/16/24/25/32 MHz), which differ from the PLLSEL_xxMHZ symbolic
 * macros.
 */
#define UHC_DWC2_STM32_HSE_HZ	DT_PROP(DT_NODELABEL(clk_hse), clock_frequency)

#if UHC_DWC2_STM32_HSE_HZ == 12000000
#define UHC_DWC2_USBPHYC_PLLSEL		(0x0UL << 1)
#elif UHC_DWC2_STM32_HSE_HZ == 12500000
#define UHC_DWC2_USBPHYC_PLLSEL		(0x2UL << 1)
#elif UHC_DWC2_STM32_HSE_HZ == 16000000
#define UHC_DWC2_USBPHYC_PLLSEL		(0x3UL << 1)
#elif UHC_DWC2_STM32_HSE_HZ == 24000000
#define UHC_DWC2_USBPHYC_PLLSEL		(0x4UL << 1)
#elif UHC_DWC2_STM32_HSE_HZ == 25000000
#define UHC_DWC2_USBPHYC_PLLSEL		(0x5UL << 1)
#elif UHC_DWC2_STM32_HSE_HZ == 32000000
#define UHC_DWC2_USBPHYC_PLLSEL		(0x7UL << 1)
#else
#error "Unsupported HSE frequency for the STM32F7 USBPHYC"
#endif

/* High-Speed PHY tuning value (from the STM32F7 HAL LL USB header) */
#define UHC_DWC2_USBPHYC_TUNE_VALUE	0x00000F13UL

/*
 * Bring up the embedded High-Speed PHY (USBPHYC). Select the UTMI+ interface
 * and run the PLL bring-up (mirrors the HAL USB_HS_PHYCInit() sequence).
 */
static inline int uhc_dwc2_stm32f7_enable_phy(const struct device *const dev)
{
	struct usb_dwc2_reg *const base = uhc_dwc2_stm32f7_base(dev);
	const k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(100));
	uint32_t gusbcfg;

	/* Select the embedded HS UTMI+ PHY */
	gusbcfg = sys_read32((mem_addr_t)&base->gusbcfg);
	gusbcfg &= ~(USB_DWC2_GUSBCFG_PHYSEL_USB11 |
		     USB_DWC2_GUSBCFG_ULPIFSLS |
		     USB_DWC2_GUSBCFG_ULPIEVBUSD |
		     USB_DWC2_GUSBCFG_ULPIEVBUSI |
		     USB_DWC2_GUSBCFG_ULPI_UTMI_SEL_ULPI);
	sys_write32(gusbcfg, (mem_addr_t)&base->gusbcfg);

	/* Power down the core's own FS transceiver */
	sys_clear_bits((mem_addr_t)&base->ggpio, USB_DWC2_GGPIO_STM32_PWRDWN);

	/*
	 * GCCFG.PHYHSEN routes the USBPHYC clock into the core. Without it the
	 * core soft reset never completes.
	 */
	sys_set_bits((mem_addr_t)&base->ggpio, USB_DWC2_GGPIO_STM32_PHYHSEN);

	/*
	 * The USBPHYC needs both the OTGHSULPI and OTGPHYC clocks enabled,
	 * even though the ULPI interface itself is not used (see the common
	 * drivers/usb/common/stm32/phy_usbphyc_f7.c).
	 */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_OTGPHYC);

	/* Enable the PHY LDO and wait until it is ready */
	USB_HS_PHYC->USB_HS_PHYC_LDO |= USB_HS_PHYC_LDO_ENABLE;
	while ((USB_HS_PHYC->USB_HS_PHYC_LDO & USB_HS_PHYC_LDO_STATUS) == 0U) {
		if (sys_timepoint_expired(timepoint)) {
			LOG_ERR("USBPHYC LDO not ready");
			return -ETIMEDOUT;
		}
	}

	/* Select the PLL input frequency, apply tuning, then enable the PLL */
	USB_HS_PHYC->USB_HS_PHYC_PLL = UHC_DWC2_USBPHYC_PLLSEL;
	USB_HS_PHYC->USB_HS_PHYC_TUNE |= UHC_DWC2_USBPHYC_TUNE_VALUE;
	USB_HS_PHYC->USB_HS_PHYC_PLL |= USB_HS_PHYC_PLL_PLLEN;

	/* Wait for the PHY clock to stabilize */
	k_msleep(2);

	return 0;
}

static inline void uhc_dwc2_stm32f7_disable_phy(const struct device *const dev)
{
	struct usb_dwc2_reg *const base = uhc_dwc2_stm32f7_base(dev);

	USB_HS_PHYC->USB_HS_PHYC_PLL &= ~USB_HS_PHYC_PLL_PLLEN;
	USB_HS_PHYC->USB_HS_PHYC_LDO &= ~USB_HS_PHYC_LDO_ENABLE;

	LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_OTGPHYC);
	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_OTGHSULPI);

	sys_clear_bits((mem_addr_t)&base->ggpio, USB_DWC2_GGPIO_STM32_PHYHSEN);
}

#else

/* External ULPI PHY configuration */

BUILD_ASSERT(DT_HAS_COMPAT_STATUS_OKAY(usb_ulpi_phy),
	     "STM32F7 OTG_HS requires a high-speed PHY: either an embedded "
	     "st,stm32-usbphyc node or an external usb-ulpi-phy node");

/* Bring up an external ULPI PHY */
static inline int uhc_dwc2_stm32f7_enable_phy(const struct device *const dev)
{
	const struct uhc_dwc2_stm32f7_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);
	struct usb_dwc2_reg *const base = uhc_dwc2_stm32f7_base(dev);
	uint32_t gusbcfg;
	int ret;

	/* Select the external ULPI PHY */
	gusbcfg = sys_read32((mem_addr_t)&base->gusbcfg);
	gusbcfg &= ~(USB_DWC2_GUSBCFG_PHYSEL_USB11 |
		     USB_DWC2_GUSBCFG_ULPIFSLS |
		     USB_DWC2_GUSBCFG_ULPIEVBUSD |
		     USB_DWC2_GUSBCFG_ULPIEVBUSI);
	gusbcfg |= USB_DWC2_GUSBCFG_ULPI_UTMI_SEL_ULPI;
	sys_write32(gusbcfg, (mem_addr_t)&base->gusbcfg);

	/* Power down the core's own FS transceiver */
	sys_clear_bits((mem_addr_t)&base->ggpio, USB_DWC2_GGPIO_STM32_PWRDWN);

	if (cfg->pcfg != NULL) {
		ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
	}

	/* Enable the ULPI clock domain */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_OTGHSULPI);

	if (cfg->phy_reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&cfg->phy_reset_gpio)) {
			LOG_ERR("ULPI PHY reset GPIO not ready");
			return -ENODEV;
		}

		/* Pulse the PHY reset */
		ret = gpio_pin_configure_dt(&cfg->phy_reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret != 0) {
			return ret;
		}

		k_msleep(1);
		gpio_pin_set_dt(&cfg->phy_reset_gpio, 0);
		k_msleep(1);
	}

	return 0;
}

static inline void uhc_dwc2_stm32f7_disable_phy(const struct device *const dev)
{
	ARG_UNUSED(dev);

	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_OTGHSULPI);
}

#endif

/*
 * Enable the OTG_HS clock and bring up the high-speed PHY.
 *
 * The DWC2 core soft reset, dwc2_core_soft_reset(), performed later in
 * uhc_dwc2_init() only completes with a running PHY clock, and the driver
 * selects the PHY only after that reset. So the PHY must be selected and
 * clocked here first.
 */
static inline int uhc_dwc2_stm32f7_pre_init(const struct device *const dev)
{
	const struct uhc_dwc2_stm32f7_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);
	int ret;

	ret = uhc_dwc2_stm32f7_enable_clk(cfg);
	if (ret) {
		return ret;
	}

	return uhc_dwc2_stm32f7_enable_phy(dev);
}

static inline int uhc_dwc2_stm32f7_shutdown(const struct device *const dev)
{
	uhc_dwc2_stm32f7_disable_phy(dev);

	return 0;
}

static inline int uhc_dwc2_stm32f7_post_enable(const struct device *const dev)
{
	const struct uhc_dwc2_stm32f7_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);

	if (cfg->vbus_gpio.port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->vbus_gpio)) {
		LOG_ERR("VBUS enable GPIO not ready");
		return -ENODEV;
	}

	return gpio_pin_configure_dt(&cfg->vbus_gpio, GPIO_OUTPUT_ACTIVE);
}

static inline int uhc_dwc2_stm32f7_disable(const struct device *const dev)
{
	const struct uhc_dwc2_stm32f7_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);

	if (cfg->vbus_gpio.port != NULL) {
		(void)gpio_pin_set_dt(&cfg->vbus_gpio, 0);
	}

	return 0;
}

#define UHC_DWC2_STM32F7_PHY_IS_ULPI(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, phys),					\
		(DT_NODE_HAS_COMPAT(DT_INST_PHANDLE(n, phys), usb_ulpi_phy)),		\
		(0))

#define UHC_DWC2_STM32F7_PHY_RESET(n)							\
	COND_CODE_1(UHC_DWC2_STM32F7_PHY_IS_ULPI(n),					\
		    (GPIO_DT_SPEC_GET_OR(DT_INST_PHANDLE(n, phys), reset_gpios, {0})),	\
		    ({0}))

#define UHC_DWC2_STM32F7_PCFG(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0),				\
		    (PINCTRL_DT_INST_DEV_CONFIG_GET(n)), (NULL))

#define QUIRK_STM32F7_HSOTG_DEFINE(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, pinctrl_0),				\
		    (PINCTRL_DT_INST_DEFINE(n);), ())					\
	static const struct stm32_pclken pclken_##n[] = STM32_DT_INST_CLOCKS(n);	\
											\
	static const struct uhc_dwc2_stm32f7_config uhc_dwc2_quirk_config_##n = {	\
		.clk_dev = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),			\
		.pclken = pclken_##n,							\
		.pclken_len = DT_INST_NUM_CLOCKS(n),					\
		.pcfg = UHC_DWC2_STM32F7_PCFG(n),					\
		.phy_reset_gpio = UHC_DWC2_STM32F7_PHY_RESET(n),			\
		.vbus_gpio = GPIO_DT_SPEC_INST_GET_OR(n, vbus_gpios, {0}),		\
	};										\
											\
	static struct uhc_dwc2_stm32f7_data uhc_dwc2_quirk_data_##n;			\
											\
	static const struct uhc_dwc2_vendor_quirks uhc_dwc2_vendor_quirks_##n = {	\
		.pre_init = uhc_dwc2_stm32f7_pre_init,					\
		.post_enable = uhc_dwc2_stm32f7_post_enable,				\
		.disable = uhc_dwc2_stm32f7_disable,					\
		.shutdown = uhc_dwc2_stm32f7_shutdown,					\
	};

/* Define the quirks only for instances that use this compatible */
#define QUIRK_STM32F7_HSOTG_DEFINE_COMPAT(n)						\
	IF_ENABLED(DT_NODE_HAS_COMPAT(DT_DRV_INST(n), st_stm32f7_hsotg),		\
		   (QUIRK_STM32F7_HSOTG_DEFINE(n)))

#define QUIRK_STM32F7_HSOTG_DEFINE_DR(n)						\
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, dr_mode, peripheral),			\
		    (/* skip peripheral role controller */),				\
		    (QUIRK_STM32F7_HSOTG_DEFINE_COMPAT(n)))

DT_INST_FOREACH_STATUS_OKAY(QUIRK_STM32F7_HSOTG_DEFINE_DR)

#endif /* ZEPHYR_DRIVERS_USB_UHC_DWC2_STM32F7_HSOTG_H */
