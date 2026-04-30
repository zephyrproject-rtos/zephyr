/*
 * TI platform vendor quirks for Cadence CDNS3 USB Device Controller
 *
 * Copyright (C) 2026 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_VENDOR_TI_AM64_H_
#define ZEPHYR_DRIVERS_USB_UDC_VENDOR_TI_AM64_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

#include "udc_common.h"
#include "udc_cdns3.h"
#include "zephyr/devicetree.h"
#include "zephyr/drivers/firmware/tisci/tisci.h"

#include <zephyr/drivers/clock_control/tisci_clock_control.h>

/* TI USB Subsystem wrapper registers */
struct udc_cdns3_ti_usbss_regs {
	volatile uint32_t PID;           /**< Revision Register, offset: 0x0 */
	volatile uint32_t W1;            /**< Wrapper Register 1, offset: 0x4 */
	volatile uint32_t STATIC_CONFIG; /**< Static Configuration Register, offset: 0x8 */
	volatile uint8_t RESERVED[120];  /**< Reserved, offset: 0x0C - 0x83 */
};

/* Wrapper Register 1 (W1) bits */
#define CDNS3_TI_W1_USB2_ONLY        BIT(19)
#define CDNS3_TI_W1_MODESTRAP        GENMASK(18, 17)
#define CDNS3_TI_W1_MODESTRAP_DEVICE (0x2)
#define CDNS3_TI_W1_MODESTRAP_SEL    BIT(9)
#define CDNS3_TI_W1_PWRUP_RST        BIT(0)

/* Static Configuration register bits */
#define CDNS3_TI_STATIC_PLL_REF_SEL GENMASK(8, 5)
#define CDNS3_TI_STATIC_VBUS_SEL    BIT(1)

/* TI CDNS3 configuration structure */
struct udc_cdns3_ti_config {
	DEVICE_MMIO_NAMED_ROM(usbss);
	const struct device *clock_controller;
	clock_control_subsys_t clock_subsys;
	bool vbus_divider;
	bool usb2_only;
};

/* TI CDNS3 runtime data */
struct udc_cdns3_ti_data {
	DEVICE_MMIO_NAMED_RAM(usbss);
};

#define DEV_CFG(quirks)  ((const struct udc_cdns3_ti_config *)quirks->config)
#define DEV_DATA(quirks) ((struct udc_cdns3_ti_data *)quirks->data)
#define DEV_REGS(quirks) ((struct udc_cdns3_ti_usbss_regs *)DEVICE_MMIO_NAMED_GET(quirks, usbss))

/* TI-specific quirks implementation */

static int udc_cdns3_ti_pll_refclk(struct udc_cdns3_quirks *quirks)
{
	const int udc_cdns3_ti_rate_table[] = {
		9600,  10000, 12000, 19200, 20000, 24000, 25000,
		26000, 38400, 40000, 58000, 50000, 52000,
	};
	const struct udc_cdns3_ti_config *cfg = DEV_CFG(quirks);
	struct udc_cdns3_ti_usbss_regs *regs = DEV_REGS(quirks);
	uint32_t rate;
	int rv = 0;

	rv = clock_control_get_rate(cfg->clock_controller, cfg->clock_subsys, &rate);
	if (rv != 0) {
		LOG_ERR("failed to get the clock rate");
		return rv;
	}

	LOG_INF("freq = %u", rate);

	ARRAY_FOR_EACH(udc_cdns3_ti_rate_table, code) {
		if (udc_cdns3_ti_rate_table[code] == (rate / 1000)) {
			uint32_t config = regs->STATIC_CONFIG;

			LOG_INF("code = %u", code);
			config &= ~CDNS3_TI_STATIC_PLL_REF_SEL;
			config |= FIELD_PREP(CDNS3_TI_STATIC_PLL_REF_SEL, code);

			regs->STATIC_CONFIG = config;

			return 0;
		}
	}

	LOG_ERR("failed to find correct rate code for the refclk frequency");
	return -EINVAL;
}

int udc_cdns3_ti_preinit(const struct device *dev)
{
	struct udc_cdns3_data *priv = udc_get_private(dev);
	struct udc_cdns3_quirks *quirks = priv->quirks;
	const struct udc_cdns3_ti_config *cfg = DEV_CFG(quirks);
	struct udc_cdns3_ti_usbss_regs *regs;
	int rv = 0;

	DEVICE_MMIO_NAMED_MAP(quirks, usbss, K_MEM_CACHE_NONE);

	regs = DEV_REGS(quirks);
	LOG_INF("AM64 quirk init");

	rv = udc_cdns3_ti_pll_refclk(quirks);
	if (rv != 0) {
		return rv;
	}

	/* assert reset */
	regs->W1 &= ~CDNS3_TI_W1_PWRUP_RST;

	/* set modestrap to device */
	regs->W1 &= ~CDNS3_TI_W1_MODESTRAP;
	regs->W1 |= FIELD_PREP(CDNS3_TI_W1_MODESTRAP, CDNS3_TI_W1_MODESTRAP_DEVICE);
	regs->W1 |= CDNS3_TI_W1_MODESTRAP_SEL;

	/* this has to be set if serdes is not assigned to this USB */
	if (cfg->usb2_only) {
		regs->W1 |= CDNS3_TI_W1_USB2_ONLY;
	} else {
		regs->W1 &= ~CDNS3_TI_W1_USB2_ONLY;
	}

	/* VBUS divider select */
	if (cfg->vbus_divider) {
		regs->STATIC_CONFIG |= CDNS3_TI_STATIC_VBUS_SEL;
	} else {
		regs->STATIC_CONFIG &= ~CDNS3_TI_STATIC_VBUS_SEL;
	}

	/* deassert reset */
	regs->W1 |= CDNS3_TI_W1_PWRUP_RST;

	return 0;
}

static const struct udc_cdns3_quirks_ops udc_cdns3_ti_quirks_ops = {
	.preinit = udc_cdns3_ti_preinit,
};

#define UDC_CDNS3_TI_DEFINE_CLK_SUBSYS(n)                                                          \
	COND_CODE_1(CONFIG_CLOCK_CONTROL_TISCI, (					\
		static struct tisci_clock_config tisci_refclk_##n =			\
			TISCI_GET_CLOCK_DETAILS_BY_INST(n);				\
		static const clock_control_subsys_t udc_cdns3_ti_clk_subsys_##n =	\
			&tisci_refclk_##n;						\
	), (BUILD_ASSERT(0, "Unsupported clock controller");))

#define UDC_CDNS3_TI_DEFINE_(n)                                                                    \
	UDC_CDNS3_TI_DEFINE_CLK_SUBSYS(n)                                                          \
	static const struct udc_cdns3_ti_config udc_cdns3_ti_quirks_config_##n = {                 \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(usbss, DT_DRV_INST(n)),                         \
		.vbus_divider = DT_INST_PROP_OR(n, ti_vbus_divider, false),                        \
		.usb2_only = DT_INST_PROP_OR(n, ti_usb2_only, false),                              \
		.clock_controller = TISCI_GET_CLOCK_BY_INST(n),                                    \
		.clock_subsys = udc_cdns3_ti_clk_subsys_##n,                                       \
	};                                                                                         \
	static struct udc_cdns3_ti_data udc_cdns3_ti_quirks_data_##n;                              \
	static struct udc_cdns3_quirks udc_cdns3_ti_quirks_##n = {                                 \
		.ops = &udc_cdns3_ti_quirks_ops,                                                   \
		.config = &udc_cdns3_ti_quirks_config_##n,                                         \
		.data = &udc_cdns3_ti_quirks_data_##n,                                             \
	};

#define UDC_CDNS3_TI_DEFINE(n)                                                                     \
	COND_CODE_1(DT_NODE_HAS_COMPAT(DT_DRV_INST(n), ti_am64_usb),				   \
		    (UDC_CDNS3_TI_DEFINE_(n)),							   \
		    ())

#undef DEV_REGS
#undef DEV_DATA
#undef DEV_CFG

#define DT_DRV_COMPAT cdns_usb3

DT_INST_FOREACH_STATUS_OKAY(UDC_CDNS3_TI_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_VENDOR_TI_AM64_H_ */
