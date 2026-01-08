/*
 * Copyright (c) 2026 Roman Leonov <jam_roma@yahoo.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UHC_DWC2_VENDOR_QUIRKS_H
#define ZEPHYR_DRIVERS_USB_UHC_DWC2_VENDOR_QUIRKS_H

#include "uhc_dwc2.h"

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/usb/uhc.h>

#if DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_usb_otg)

#define QUIRK_ESP32_USB_OTG_DEFINE(n)	\
	static int esp32_usb_otg_init_##n(const struct device *dev)	\
	{							\
		return 0; \
	}							\
								\
	static int esp32_usb_otg_enable_phy_##n(const struct device *dev) \
	{							\
		return 0; \
	}							\
								\
	static int esp32_usb_otg_disable_phy_##n(const struct device *dev) \
	{							\
		return 0; \
	}							\
								\
	static int esp32_usb_otg_get_phy_clk_##n(const struct device *dev)	\
	{							\
		return 0;				\
	}							\
								\
	struct uhc_dwc2_vendor_quirks uhc_dwc2_vendor_quirks_##n = {	\
		.init = esp32_usb_otg_init_##n,				\
		.pre_enable = esp32_usb_otg_enable_phy_##n,		\
		.disable = esp32_usb_otg_disable_phy_##n,		\
		.get_phy_clk = esp32_usb_otg_get_phy_clk_##n,		\
	};

#define UHC_DWC2_IRQ_DT_INST_DEFINE(n)		\
	static void uhc_dwc2_irq_enable_func_##n(const struct device *dev) \
	{						\
		/* TODO: esp_intr_enable */ \
	}						\
							\
	static void uhc_dwc2_irq_disable_func_##n(const struct device *dev) \
	{						\
		/* TODO: esp_intr_disable */ \
	}

DT_INST_FOREACH_STATUS_OKAY(QUIRK_ESP32_USB_OTG_DEFINE)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_usb_otg) */

#endif /* ZEPHYR_DRIVERS_USB_UHC_DWC2_VENDOR_QUIRKS_H */
