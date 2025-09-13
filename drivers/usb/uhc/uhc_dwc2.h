/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_H

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb/uhc.h>
#include <usb_dwc2_hw.h>


/* Vendor quirks per driver instance */
struct uhc_dwc2_vendor_quirks {
	/* Called at the beginning of uhc_dwc2_init() */
	int (*init)(const struct device *dev);
	/* Called on uhc_dwc2_enable() before the controller is initialized */
	int (*pre_enable)(const struct device *dev);
	/* Called on uhc_dwc2_enable() after the controller is initialized */
	int (*post_enable)(const struct device *dev);
	/* Called at the end of uhc_dwc2_disable() */
	int (*disable)(const struct device *dev);
	/* Called at the end of uhc_dwc2_shutdown() */
	int (*shutdown)(const struct device *dev);
	/* Enable interrupts function */
	int (*irq_enable_func)(const struct device *dev);
	/* Disable interrupts function */
	int (*irq_disable_func)(const struct device *dev);
	/* Called at the end of IRQ handling */
	int (*irq_clear)(const struct device *dev);
	/* Called on driver pre-init */
	int (*caps)(const struct device *dev);
	/* Called on PHY pre-select */
	int (*phy_pre_select)(const struct device *dev);
	/* Called on PHY post-select and core reset */
	int (*phy_post_select)(const struct device *dev);
	/* Called while waiting for bits that require PHY to be clocked */
	int (*is_phy_clk_off)(const struct device *dev);
	/* PHY get clock */
	int (*get_phy_clk)(const struct device *dev);
	/* Called after hibernation entry sequence */
	int (*post_hibernation_entry)(const struct device *dev);
	/* Called before hibernation exit sequence */
	int (*pre_hibernation_exit)(const struct device *dev);
};

#include "uhc_dwc2_vendor_quirks.h"

/* Driver configuration per instance */
struct uhc_dwc2_config {
	/* Pointer to base address of DWC_OTG registers */
	struct usb_dwc2_reg *const base;
	/* Pointer to vendor quirks or NULL */
	const struct uhc_dwc2_vendor_quirks *const quirks;
	struct pinctrl_dev_config *const pcfg;

	void (*make_thread)(const struct device *dev);
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);

	/* TODO: Peripheral driver public parameters? */
};

#define UHC_DWC2_VENDOR_QUIRK_GET(n)                                            \
	COND_CODE_1(DT_NODE_VENDOR_HAS_IDX(DT_DRV_INST(n), 1),                      \
			(&uhc_dwc2_vendor_quirks_##n),                                      \
			(NULL))


#define DWC2_QUIRK_FUNC_DEFINE(fname)                                           \
static inline int uhc_dwc2_quirk_##fname(const struct device *dev)              \
{                                                                               \
	const struct uhc_dwc2_config *const config = dev->config;                   \
	const struct uhc_dwc2_vendor_quirks *const quirks =                         \
		COND_CODE_1(IS_EQ(DT_NUM_INST_STATUS_OKAY(snps_dwc2), 1),               \
			(UHC_DWC2_VENDOR_QUIRK_GET(0); ARG_UNUSED(config);),                \
			(config->quirks;))                                                  \
																				\
	if (quirks != NULL && quirks->fname != NULL) {                              \
		return quirks->fname(dev);                                              \
	}                                                                           \
																				\
	return 0;                                                                   \
}

DWC2_QUIRK_FUNC_DEFINE(init)
DWC2_QUIRK_FUNC_DEFINE(pre_enable)
DWC2_QUIRK_FUNC_DEFINE(post_enable)
DWC2_QUIRK_FUNC_DEFINE(disable)
DWC2_QUIRK_FUNC_DEFINE(shutdown)
DWC2_QUIRK_FUNC_DEFINE(irq_enable_func)
DWC2_QUIRK_FUNC_DEFINE(irq_disable_func)
DWC2_QUIRK_FUNC_DEFINE(irq_clear)
DWC2_QUIRK_FUNC_DEFINE(caps)
DWC2_QUIRK_FUNC_DEFINE(phy_pre_select)
DWC2_QUIRK_FUNC_DEFINE(phy_post_select)
DWC2_QUIRK_FUNC_DEFINE(is_phy_clk_off)
DWC2_QUIRK_FUNC_DEFINE(get_phy_clk)
DWC2_QUIRK_FUNC_DEFINE(post_hibernation_entry)
DWC2_QUIRK_FUNC_DEFINE(pre_hibernation_exit)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_H */
