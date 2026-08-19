/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_SYNA_SR100_USB_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_SYNA_SR100_USB_H

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/reset.h>

struct syna_sr100_usb_clk {
	const struct device *const dev;
	const clock_control_subsys_t id;
	const struct reset_dt_spec reset;
};

static inline int syna_sr100_usb_pre_enable(const struct syna_sr100_usb_clk *const clk)
{
	if (device_is_ready(clk->reset.dev)) {
		int ret = reset_line_toggle(clk->reset.dev, clk->reset.id);

		if (ret != 0) {
			return ret;
		}
	}

	if (!device_is_ready(clk->dev)) {
		return -ENODEV;
	}

	return clock_control_on(clk->dev, clk->id);
}

static inline int syna_sr100_usb_init_caps(const struct device *dev)
{
	struct udc_data *data = dev->data;

	data->caps.hs = true;

	return 0;
}

#define QUIRK_SYNA_SR100_USB_DEFINE(n)						\
	static const struct syna_sr100_usb_clk syna_sr100_usb_clk_##n = {	\
		.dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
		.id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, clkid),	\
		.reset = RESET_DT_SPEC_INST_GET(n),				\
	};									\
										\
	static int syna_sr100_usb_pre_enable_##n(const struct device *dev)	\
	{									\
		return syna_sr100_usb_pre_enable(&syna_sr100_usb_clk_##n);	\
	}									\
										\
	struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {			\
		.caps = syna_sr100_usb_init_caps,				\
		.pre_enable = syna_sr100_usb_pre_enable_##n,			\
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_SYNA_SR100_USB_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_SYNA_SR100_USB_H */
