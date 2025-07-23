/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
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

#include <zephyr/logging/log.h>

#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/drivers/clock_control.h>

#include <esp_err.h>
#include <esp_private/usb_phy.h>
#include <hal/usb_wrap_hal.h>
#include <soc/usb_dwc_cfg.h>
#include <soc/usb_dwc_periph.h>

#include <esp_rom_gpio.h>
#include <driver/gpio.h>
#include <soc/usb_pins.h>
#include <soc/gpio_sig_map.h>

struct phy_context_t {
	usb_phy_target_t target;
	usb_phy_controller_t controller;
	usb_phy_status_t status;
	usb_otg_mode_t otg_mode;
	usb_phy_speed_t otg_speed;
	usb_phy_ext_io_conf_t *ext_io_pins;
	usb_wrap_hal_context_t wrap_hal;
};

struct usb_dw_esp32_config {
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	int irq_source;
	int irq_priority;
	int irq_flags;
	struct phy_context_t *phy_ctx;
};

struct usb_dw_esp32_data {
	struct intr_handle_data_t *int_handle;
};

static void uhc_dwc2_isr_handler(const struct device *dev);

static inline int esp32_usb_otg_init(const struct device *dev,
									 const struct usb_dw_esp32_config *cfg,
									 struct usb_dw_esp32_data *data)
{
	LOG_MODULE_DECLARE(uhc_dwc2, CONFIG_UHC_DRIVER_LOG_LEVEL);

	int ret;

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);

	if (ret != 0) {
		return ret;
	}

	/* pinout config to work in USB_OTG_MODE_HOST */
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ZERO_INPUT, USB_OTG_IDDIG_IN_IDX, false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ZERO_INPUT, USB_SRP_BVALID_IN_IDX, false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, USB_OTG_VBUSVALID_IN_IDX, false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, USB_OTG_AVALID_IN_IDX, false);

	if (cfg->phy_ctx->target == USB_PHY_TARGET_INT) {
		gpio_set_drive_capability(USBPHY_DM_NUM, GPIO_DRIVE_CAP_3);
		gpio_set_drive_capability(USBPHY_DP_NUM, GPIO_DRIVE_CAP_3);
	}

	/* allocate interrupt but keep it disabled to avoid
	 * spurious suspend/resume event at enumeration phase
	 */
	ret = esp_intr_alloc(cfg->irq_source,
						 ESP_INTR_FLAG_INTRDISABLED |
						 ESP_PRIO_TO_FLAGS(cfg->irq_priority) |
								 ESP_INT_FLAGS_CHECK(cfg->irq_flags),
						 (intr_handler_t)uhc_dwc2_isr_handler, (void *)dev, &data->int_handle);

	if (ret != 0) {
		return -ECANCELED;
	}

	LOG_DBG("PHY inited");
	return 0;
}

static inline int esp32_usb_otg_enable_phy(struct phy_context_t *phy_ctx, bool enable)
{
	LOG_MODULE_DECLARE(uhc_dwc2, CONFIG_UHC_DRIVER_LOG_LEVEL);
	if (enable) {
		usb_wrap_ll_enable_bus_clock(true);
		usb_wrap_hal_init(&phy_ctx->wrap_hal);

#if USB_WRAP_LL_EXT_PHY_SUPPORTED
		usb_wrap_hal_phy_set_external(&phy_ctx->wrap_hal,
									  (phy_ctx->target == USB_PHY_TARGET_EXT));
#endif
		if (phy_ctx->target == USB_PHY_TARGET_INT) {
			/* Configure pull resistors for host */
			usb_wrap_pull_override_vals_t vals = {
				.dp_pu = false,
				.dm_pu = false,
				.dp_pd = true,
				.dm_pd = true,
			};
			usb_wrap_hal_phy_enable_pull_override(&phy_ctx->wrap_hal, &vals);
		}
		LOG_DBG("PHY enabled");
	} else {
		usb_wrap_ll_enable_bus_clock(false);
		usb_wrap_ll_phy_enable_pad(phy_ctx->wrap_hal.dev, false);

		LOG_DBG("PHY disabled");
	}
	return 0;
}

static inline int esp32_usb_otg_get_phy_clock(struct phy_context_t *phy_ctx)
{
	if (phy_ctx->otg_speed == USB_PHY_SPEED_FULL) {
		return MHZ(48);
	} else if (phy_ctx->otg_speed == USB_PHY_SPEED_LOW) {
		/* PHY has implicit divider of 8 when in low speed */
		return MHZ(48) / 8;
	} else {
		/* non supported speed */
		return 0;
	}
	return 0;
}

#define QUIRK_ESP32_USB_OTG_DEFINE(n)                                          \
																				\
	static struct phy_context_t phy_ctx_##n = {                                 \
		.target = USB_PHY_TARGET_INT,                                           \
		.controller = USB_PHY_CTRL_OTG,                                         \
		.otg_mode = USB_OTG_MODE_HOST,                                          \
		.otg_speed = USB_PHY_SPEED_UNDEFINED,                                   \
		.ext_io_pins = NULL,                                                    \
		.wrap_hal = {},                                                         \
	};                                                                          \
																				\
	static const struct usb_dw_esp32_config usb_otg_config_##n = {              \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                     \
		.clock_subsys = (clock_control_subsys_t)                                \
			DT_INST_CLOCKS_CELL(n, offset),                                     \
		.irq_source = DT_INST_IRQ_BY_IDX(n, 0, irq),                            \
		.irq_priority = DT_INST_IRQ_BY_IDX(n, 0, priority),                     \
		.irq_flags = DT_INST_IRQ_BY_IDX(n, 0, flags),                           \
		.phy_ctx = &phy_ctx_##n,                                                \
	};                                                                          \
																				\
	static struct usb_dw_esp32_data usb_otg_data_##n;                           \
																				\
	static int esp32_usb_otg_init_##n(const struct device *dev)                 \
	{                                                                           \
		return esp32_usb_otg_init(dev,                                          \
			&usb_otg_config_##n, &usb_otg_data_##n);                            \
	}                                                                           \
																				\
	static int esp32_usb_otg_enable_phy_##n(const struct device *dev)           \
	{                                                                           \
		return esp32_usb_otg_enable_phy(&phy_ctx_##n, true);                    \
	}                                                                           \
																				\
	static int esp32_usb_otg_disable_phy_##n(const struct device *dev)          \
	{                                                                           \
		return esp32_usb_otg_enable_phy(&phy_ctx_##n, false);                   \
	}                                                                           \
	static int esp32_usb_int_enable_func_##n(const struct device *dev)          \
	{                                                                           \
		return esp_intr_enable(usb_otg_data_##n.int_handle);                    \
	}                                                                           \
																				\
	static int esp32_usb_int_disable_func_##n(const struct device *dev)         \
	{                                                                           \
		return esp_intr_disable(usb_otg_data_##n.int_handle);                   \
	}                                                                           \
																				\
	static int esp32_usb_get_phy_clock_##n(const struct device *dev)            \
	{                                                                           \
		return esp32_usb_otg_get_phy_clock(&phy_ctx_##n);                       \
	}                                                                           \
																				\
	struct uhc_dwc2_vendor_quirks uhc_dwc2_vendor_quirks_##n = {                \
		.init = esp32_usb_otg_init_##n,                                         \
		.pre_enable = esp32_usb_otg_enable_phy_##n,                             \
		.disable = esp32_usb_otg_disable_phy_##n,                               \
		.irq_enable_func = esp32_usb_int_enable_func_##n,                       \
		.irq_disable_func = esp32_usb_int_disable_func_##n,                     \
		.get_phy_clk = esp32_usb_get_phy_clock_##n,                             \
	};

DT_INST_FOREACH_STATUS_OKAY(QUIRK_ESP32_USB_OTG_DEFINE)

#endif /*DT_HAS_COMPAT_STATUS_OKAY(espressif_esp32_usb_otg) */

/* Add next vendor quirks definition above this line */

#endif /* ZEPHYR_DRIVERS_USB_UHC_DWC2_VENDOR_QUIRKS_H */
