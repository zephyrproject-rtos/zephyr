/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_DEVICE_USB_DC_DW_STM32_H
#define ZEPHYR_DRIVERS_USB_DEVICE_USB_DC_DW_STM32_H

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include <usb_dwc2_hw.h>

struct usb_dw_stm32_clk {
	const struct device *const dev;
	const struct stm32_pclken *const pclken;
	size_t pclken_len;
};

static inline int clk_enable_st_stm32f4_fsotg(const struct usb_dw_stm32_clk *const clk)
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

static inline int pwr_on_st_stm32f4_fsotg(struct usb_dwc2_reg *const base)
{
	base->ggpio |= USB_DWC2_GGPIO_STM32_PWRDWN | USB_DWC2_GGPIO_STM32_VBDEN;

	return 0;
}

#define QUIRK_ST_STM32F4_FSOTG_DEFINE(n)					\
	static const struct stm32_pclken pclken_##n[] = STM32_DT_INST_CLOCKS(n);\
										\
	static const struct usb_dw_stm32_clk stm32f4_clk_##n = {		\
		.dev = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),			\
		.pclken = pclken_##n,						\
		.pclken_len = DT_INST_NUM_CLOCKS(n),				\
	};									\
										\
	static int clk_enable_st_stm32f4_fsotg_##n(void)			\
	{									\
		return clk_enable_st_stm32f4_fsotg(&stm32f4_clk_##n);		\
	}

#define USB_DW_QUIRK_ST_STM32F4_FSOTG_DEFINE(n)					\
	COND_CODE_1(DT_NODE_HAS_COMPAT(DT_DRV_INST(n), st_stm32f4_fsotg),	\
		    (QUIRK_ST_STM32F4_FSOTG_DEFINE(n)), ())

#endif /* ZEPHYR_DRIVERS_USB_DEVICE_USB_DC_DW_STM32_H */
