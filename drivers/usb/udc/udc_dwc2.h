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
	int (*clk_enable)(const struct device *dev);
	int (*clk_disable)(const struct device *dev);
	int (*pwr_on)(const struct device *dev);
	int (*pwr_off)(const struct device *dev);
	int (*irq_clear)(const struct device *dev);
};

/* Driver configuration per instance */
struct udc_dwc2_config {
	size_t num_of_eps;
	struct udc_ep_config *ep_cfg_in;
	struct udc_ep_config *ep_cfg_out;
	int speed_idx;
	struct usb_dwc2_reg *const base;
	/* Pointer to pin control configuration or NULL */
	struct pinctrl_dev_config *const pcfg;
	/* Pointer to vendor quirks or NULL */
	struct dwc2_vendor_quirks *const quirks;
	void (*make_thread)(const struct device *dev);
	void (*irq_enable_func)(const struct device *dev);
	void (*irq_disable_func)(const struct device *dev);
};

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_H */
