/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC3_TI_AM62_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC3_TI_AM62_H

#include <zephyr/drivers/syscon.h>
#include <zephyr/drivers/clock_control/tisci_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/device_mmio.h>

struct udc_dwc3_ti_phy_regs {
	uint8_t RESERVED[0x130]; /**< Reserved, offset: 0x0 - 0x130 */
	volatile uint32_t REG12; /**< PHY2 PLL REG12, offset: 0x130 */
};

struct udc_dwc3_ti_usbss_regs {
	uint8_t RESERVED_1[0x8];        /**< Reserved, offset: 0x0 - 0x8 */
	volatile uint32_t PHY_CONFIG;   /**< Static PHY configuration, offset: 0x8 */
	uint8_t RESERVED_2[0x10];       /**< Reserved, offset: 0x0C - 0x1C */
	volatile uint32_t MODE_CONTROL; /**< DRD role determination , offset: 0x1C */
};

#define DWC3_CFG(dev)  ((const struct udc_dwc3_config *)(dev)->config)
#define DEV_CFG(dev)   ((const struct udc_dwc3_ti_config *)(DWC3_CFG(dev)->quirk_config))
#define DEV_DATA(dev)  ((struct udc_dwc3_ti_data *)(DWC3_CFG(dev)->quirk_data))
#define DEV_USBSS(dev) ((struct udc_dwc3_ti_usbss_regs *)DEVICE_MMIO_NAMED_GET(dev, usbss))
#define DEV_PHY(dev)   ((struct udc_dwc3_ti_phy_regs *)DEVICE_MMIO_NAMED_GET(dev, phy))

/* USB syscon module registers */
#define UDC_DWC3_TI_PHY_PLL_REFCLK_MASK GENMASK(3, 0)

/* USB PHY2 PLL registers */
#define UDC_DWC3_TI_PHY_PLL_LDO_REF_EN    BIT(5)
#define UDC_DWC3_TI_PHY_PLL_LDO_REF_EN_EN BIT(4)

/* USB CFG registers */
#define UDC_DWC3_TI_CFG_PHY_CONFIG_VBUS_SEL     BIT(1)
#define UDC_DWC3_TI_CFG_MODE_CONTROL_MODE_VALID BIT(0)

struct udc_dwc3_ti_config {
	DEVICE_MMIO_NAMED_ROM(usbss);
	DEVICE_MMIO_NAMED_ROM(phy);

	const struct device *syscon_pll_refclk;
	mem_addr_t offset_pll_refclk;
	const struct device *clock_controller;
	clock_control_subsys_t clock_subsys;
	bool vbus_divider;
};

struct udc_dwc3_ti_data {
	DEVICE_MMIO_NAMED_RAM(usbss);
	DEVICE_MMIO_NAMED_RAM(phy);
};

static int udc_dwc3_ti_phy_syscon_pll_refclk(const struct device *dev)
{
	const int udc_dwc3_ti_rate_table[] = {
		9600,  10000, 12000, 19200, 20000, 24000, 25000,
		26000, 38400, 40000, 58000, 50000, 52000,
	};
	const struct udc_dwc3_ti_config *cfg = DEV_CFG(dev);
	uint32_t rate;
	int rv = 0;

	rv = clock_control_get_rate(cfg->clock_controller, cfg->clock_subsys, &rate);
	if (rv != 0) {
		LOG_ERR("failed to get the clock rate");
	}

	if (cfg->syscon_pll_refclk == NULL) {
		LOG_ERR("no syscon device for ti,syscon-phy-pll-refclk");
		return -EINVAL;
	}

	ARRAY_FOR_EACH(udc_dwc3_ti_rate_table, code) {
		if (udc_dwc3_ti_rate_table[code] == (rate / 1000)) {
			uint32_t reg = 0;

			rv = syscon_read_reg(cfg->syscon_pll_refclk, cfg->offset_pll_refclk, &reg);
			if (rv != 0) {
				LOG_ERR("failed to read syscon register");
				return rv;
			}

			reg &= ~UDC_DWC3_TI_PHY_PLL_REFCLK_MASK;
			reg |= FIELD_PREP(UDC_DWC3_TI_PHY_PLL_REFCLK_MASK, code);

			rv = syscon_write_reg(cfg->syscon_pll_refclk, cfg->offset_pll_refclk, reg);
			if (rv != 0) {
				LOG_ERR("failed to write syscon register");
				return rv;
			}

			return 0;
		}
	}

	LOG_ERR("failed to find correct rate code for the refclk frequency");
	return -EINVAL;
}

int udc_dwc3_ti_preinit(const struct device *dev)
{
	const struct udc_dwc3_ti_config *cfg = DEV_CFG(dev);
	struct udc_dwc3_ti_phy_regs *phy;
	struct udc_dwc3_ti_usbss_regs *usbss;
	int rv = 0;

	DEVICE_MMIO_NAMED_MAP(dev, usbss, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, phy, K_MEM_CACHE_NONE);

	rv = udc_dwc3_ti_phy_syscon_pll_refclk(dev);
	if (rv != 0) {
		return rv;
	}

	phy = DEV_PHY(dev);
	usbss = DEV_USBSS(dev);

	/* Workaround for Errata i2409 */
	phy->REG12 |= (UDC_DWC3_TI_PHY_PLL_LDO_REF_EN | UDC_DWC3_TI_PHY_PLL_LDO_REF_EN_EN);

	/* VBUS divider select */
	if (cfg->vbus_divider) {
		usbss->PHY_CONFIG |= UDC_DWC3_TI_CFG_PHY_CONFIG_VBUS_SEL;
	} else {
		usbss->PHY_CONFIG &= ~UDC_DWC3_TI_CFG_PHY_CONFIG_VBUS_SEL;
	}

	/* Set current mode (role) as valid */
	usbss->MODE_CONTROL |= UDC_DWC3_TI_CFG_MODE_CONTROL_MODE_VALID;

	return 0;
}

const struct udc_dwc3_vendor_quirks udc_dwc3_vendor_quirks = {
	.preinit = udc_dwc3_ti_preinit,
};

#define UDC_DWC3_TI_DEFINE_CLK_SUBSYS(n)                                                           \
	COND_CODE_1(CONFIG_CLOCK_CONTROL_TISCI, (						   \
		static struct tisci_clock_config tisci_refclk_##n =                                \
			TISCI_GET_CLOCK_DETAILS_BY_INST(n);                                        \
		static const clock_control_subsys_t udc_dwc3_ti_clk_subsys_##n =                   \
			&tisci_refclk_##n;                                                         \
	), (BUILD_ASSERT(0, "Unsupported clock controller");))

#define UDC_DWC3_QUIRK_DEFINE(n)                                                                   \
	UDC_DWC3_TI_DEFINE_CLK_SUBSYS(n)                                                           \
	static const struct udc_dwc3_ti_config udc_dwc3_quirk_config_##n = {                       \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(usbss, DT_DRV_INST(n)),                         \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(phy, DT_DRV_INST(n)),                           \
		.vbus_divider = DT_INST_PROP(n, ti_vbus_divider),                                  \
		.clock_controller = TISCI_GET_CLOCK_BY_INST(n),                                    \
		.clock_subsys = &udc_dwc3_ti_clk_subsys_##n,                                       \
		UDC_DWC3_TI_PHY_SYSCON_PLL_REFCLK_INIT(n),                                         \
	};                                                                                         \
	static struct udc_dwc3_ti_data udc_dwc3_quirk_data_##n;

#undef DEV_PHY
#undef DEV_USBSS
#undef DEV_DATA
#undef DEV_CFG
#undef DWC3_CFG

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC3_VENDOR_QUIRKS_TI_AM62_H */
