/*
 * SPDXFileCopyrightText: Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_ESP32_USB_OTG_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_ESP32_USB_OTG_H

#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

#include <esp_err.h>
#include <esp_private/usb_phy.h>
#include <hal/usb_wrap_hal.h>
#include <soc/usb_periph.h>

#include <esp_rom_gpio.h>
#include <hal/gpio_ll.h>
#include <soc/usb_pins.h>
#include <soc/gpio_sig_map.h>

struct phy_context_t {
	usb_phy_target_t target;
	usb_phy_controller_t controller;
	usb_phy_status_t status;
	usb_otg_mode_t otg_mode;
	usb_phy_speed_t otg_speed;
	usb_phy_ext_io_conf_t *iopins;
	usb_wrap_hal_context_t wrap_hal;
};

struct usb_dw_esp32_config {
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	struct phy_context_t *phy_ctx;
	void (*irq_configure)(void);
};

static void udc_dwc2_isr_handler(const struct device *dev);

static inline int esp32_usb_otg_init(const struct device *dev,
				     const struct usb_dw_esp32_config *cfg)
{
	int ret;

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);

	if (ret != 0) {
		return ret;
	}

	/* pinout config to work in USB_OTG_MODE_DEVICE */
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, USB_OTG_IDDIG_IN_IDX, false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, USB_SRP_BVALID_IN_IDX, false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, USB_OTG_VBUSVALID_IN_IDX,
				       false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ZERO_INPUT, USB_OTG_AVALID_IN_IDX, false);

	if (cfg->phy_ctx->target == USB_PHY_TARGET_INT) {
		gpio_ll_set_drive_capability(GPIO_LL_GET_HW(0), USBPHY_DM_NUM, GPIO_DRIVE_CAP_3);
		gpio_ll_set_drive_capability(GPIO_LL_GET_HW(0), USBPHY_DP_NUM, GPIO_DRIVE_CAP_3);
	}

	cfg->irq_configure();

	return 0;
}

static inline int esp32_usb_otg_enable_clk(struct phy_context_t *phy_ctx)
{
	usb_wrap_hal_init(&phy_ctx->wrap_hal);

#if USB_WRAP_LL_EXT_PHY_SUPPORTED
	usb_wrap_hal_phy_set_external(&phy_ctx->wrap_hal, (phy_ctx->target == USB_PHY_TARGET_EXT));
#endif

	return 0;
}

static inline int esp32_usb_otg_enable_phy(struct phy_context_t *phy_ctx, bool enable)
{
	LOG_MODULE_DECLARE(udc_dwc2, CONFIG_UDC_DRIVER_LOG_LEVEL);

	if (enable) {
		usb_wrap_ll_phy_enable_pad(phy_ctx->wrap_hal.dev, true);
		LOG_DBG("PHY enabled");
	} else {
		usb_wrap_ll_phy_enable_pad(phy_ctx->wrap_hal.dev, false);
		LOG_DBG("PHY disabled");
	}

	return 0;
}

static inline int esp32_usb_otg_shutdown(struct phy_context_t *phy_ctx)
{
	usb_wrap_ll_enable_bus_clock(false);

	return 0;
}

#define QUIRK_ESP32_USB_OTG_DEFINE(n)                                                              \
                                                                                                   \
	static struct phy_context_t phy_ctx_##n = {                                                \
		.target = USB_PHY_TARGET_INT,                                                      \
		.controller = USB_PHY_CTRL_OTG,                                                    \
		.otg_mode = USB_OTG_MODE_DEVICE,                                                   \
		.otg_speed = USB_PHY_SPEED_FULL,                                                   \
		.iopins = NULL,                                                                    \
		.wrap_hal = {},                                                                    \
	};                                                                                         \
                                                                                                   \
	static void udc_dwc2_irq_configure_func_##n(void);                                         \
                                                                                                   \
	static const struct usb_dw_esp32_config usb_otg_config_##n = {                             \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, offset),            \
		.phy_ctx = &phy_ctx_##n,                                                           \
		.irq_configure = udc_dwc2_irq_configure_func_##n,                                  \
	};                                                                                         \
                                                                                                   \
	static int esp32_usb_otg_init_##n(const struct device *dev)                                \
	{                                                                                          \
		return esp32_usb_otg_init(dev, &usb_otg_config_##n);                               \
	}                                                                                          \
                                                                                                   \
	static int esp32_usb_otg_enable_clk_##n(const struct device *dev)                          \
	{                                                                                          \
		return esp32_usb_otg_enable_clk(&phy_ctx_##n);                                     \
	}                                                                                          \
                                                                                                   \
	static int esp32_usb_otg_enable_phy_##n(const struct device *dev)                          \
	{                                                                                          \
		return esp32_usb_otg_enable_phy(&phy_ctx_##n, true);                               \
	}                                                                                          \
                                                                                                   \
	static int esp32_usb_otg_disable_phy_##n(const struct device *dev)                         \
	{                                                                                          \
		return esp32_usb_otg_enable_phy(&phy_ctx_##n, false);                              \
	}                                                                                          \
	static int esp32_usb_otg_shutdown_##n(const struct device *dev)                            \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		return esp32_usb_otg_shutdown(&phy_ctx_##n);                                       \
	}                                                                                          \
                                                                                                   \
	const struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {                                 \
		.init = esp32_usb_otg_init_##n,                                                    \
		.pre_enable = esp32_usb_otg_enable_clk_##n,                                        \
		.post_enable = esp32_usb_otg_enable_phy_##n,                                       \
		.disable = esp32_usb_otg_disable_phy_##n,                                          \
		.shutdown = esp32_usb_otg_shutdown_##n,                                            \
	};

#define UDC_DWC2_IRQ_DT_INST_DEFINE(n)                                                             \
	static void udc_dwc2_irq_configure_func_##n(void)                                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq), DT_INST_IRQ_BY_IDX(n, 0, priority),     \
			    udc_dwc2_isr_handler, DEVICE_DT_INST_GET(n),                           \
			    ESP_INT_FLAGS_CHECK(DT_INST_IRQ_BY_IDX(n, 0, flags)));                 \
		irq_matrix_enable(DT_INST_IRQ_BY_IDX(n, 0, irq),                                   \
				  DT_INST_IRQ_BY_IDX(n, 0, source));                               \
	}                                                                                          \
                                                                                                   \
	static void udc_dwc2_irq_enable_func_##n(const struct device *dev)                         \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));                                         \
	}                                                                                          \
                                                                                                   \
	static void udc_dwc2_irq_disable_func_##n(const struct device *dev)                        \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		irq_disable(DT_INST_IRQ_BY_IDX(n, 0, irq));                                        \
	}

DT_INST_FOREACH_STATUS_OKAY(QUIRK_ESP32_USB_OTG_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_ESP32_USB_OTG_H */
