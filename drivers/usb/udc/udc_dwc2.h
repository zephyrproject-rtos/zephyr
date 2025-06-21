/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_H

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb/udc.h>
#include <usb_dwc2_hw.h>

/* Vendor quirks per driver instance */
struct dwc2_vendor_quirks {
	/* Called at the beginning of udc_dwc2_init() */
	int (*init)(const struct device *dev);
	/* Called on udc_dwc2_enable() before the controller is initialized */
	int (*pre_enable)(const struct device *dev);
	/* Called on udc_dwc2_enable() after the controller is initialized */
	int (*post_enable)(const struct device *dev);
	/* Called at the end of udc_dwc2_disable() */
	int (*disable)(const struct device *dev);
	/* Called at the end of udc_dwc2_shutdown() */
	int (*shutdown)(const struct device *dev);
	/* Called at the end of IRQ handling */
	int (*irq_clear)(const struct device *dev);
	/* Called on driver pre-init */
	int (*caps)(const struct device *dev);
	/* Called while waiting for bits that require PHY to be clocked */
	int (*is_phy_clk_off)(const struct device *dev);
	/* Called after hibernation entry sequence */
	int (*post_hibernation_entry)(const struct device *dev);
	/* Called before hibernation exit sequence */
	int (*pre_hibernation_exit)(const struct device *dev);
};

/* Driver configuration per instance */
struct udc_dwc2_config {
	size_t num_in_eps;
	size_t num_out_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	struct usb_dwc2_reg *const base;
	/* Pointer to pin control configuration or NULL */
	struct pinctrl_dev_config *const pcfg;
	/* Pointer to vendor quirks or NULL */
	const struct dwc2_vendor_quirks *const quirks;
	void (*make_thread)(const struct device *dev);
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
	uint32_t ghwcfg1;
	uint32_t ghwcfg2;
	uint32_t ghwcfg4;
};

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_H */
