/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_VENDOR_QUIRKS_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_VENDOR_QUIRKS_H

#include "udc_dwc2.h"

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <usb_dwc2_hw.h>

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_fsotg)

struct usb_dw_stm32_clk {
	const struct device *const dev;
	const struct stm32_pclken *const pclken;
	size_t pclken_len;
};

#define DT_DRV_COMPAT snps_dwc2

static inline int clk_enable_stm32f4_fsotg(const struct usb_dw_stm32_clk *const clk)
{
	int ret;

	if (!device_is_ready(clk->dev)) {
		return -ENODEV;
	}

	if (clk->pclken_len > 1) {
		uint32_t clk_rate;

		ret = clock_control_configure(clk->dev,
					      (void *)&clk->pclken[1],
					      NULL);
		if (ret) {
			return ret;
		}

		ret = clock_control_get_rate(clk->dev,
					     (void *)&clk->pclken[1],
					     &clk_rate);
		if (ret) {
			return ret;
		}

		if (clk_rate != MHZ(48)) {
			return -ENOTSUP;
		}
	}

	return clock_control_on(clk->dev, (void *)&clk->pclken[0]);
}

static inline int pwr_on_stm32f4_fsotg(const struct device *dev)
{
	const struct udc_dwc2_config *const config = dev->config;
	mem_addr_t ggpio_reg = (mem_addr_t)&config->base->ggpio;

	sys_set_bits(ggpio_reg, USB_DWC2_GGPIO_STM32_PWRDWN | USB_DWC2_GGPIO_STM32_VBDEN);

	return 0;
}

#define QUIRK_STM32F4_FSOTG_DEFINE(n)						\
	static const struct stm32_pclken pclken_##n[] = STM32_DT_INST_CLOCKS(n);\
										\
	static const struct usb_dw_stm32_clk stm32f4_clk_##n = {		\
		.dev = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),			\
		.pclken = pclken_##n,						\
		.pclken_len = DT_INST_NUM_CLOCKS(n),				\
	};									\
										\
	static int clk_enable_stm32f4_fsotg_##n(const struct device *dev)	\
	{									\
		return clk_enable_stm32f4_fsotg(&stm32f4_clk_##n);		\
	}									\
										\
	struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {			\
		.clk_enable = clk_enable_stm32f4_fsotg_##n,			\
		.pwr_on = pwr_on_stm32f4_fsotg,					\
		.irq_clear = NULL,						\
	};


DT_INST_FOREACH_STATUS_OKAY(QUIRK_STM32F4_FSOTG_DEFINE)

#undef DT_DRV_COMPAT

#endif /*DT_HAS_COMPAT_STATUS_OKAY(st_stm32f4_fsotg) */

/* Add next vendor quirks definition above this line */

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_VENDOR_QUIRKS_H */
