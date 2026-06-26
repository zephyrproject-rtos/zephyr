/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_ESP32_USB_OTG_FS_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_ESP32_USB_OTG_FS_H

#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

#include <esp_err.h>
#include <esp_private/usb_phy.h>
#include <hal/usb_wrap_hal.h>
#include <soc/usb_periph.h>

#include <esp_rom_gpio.h>
#include <hal/gpio_ll.h>
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

struct esp32_usb_otg_fs_config {
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	int irq_source;
	int irq_priority;
	int irq_flags;
	int phy_dp_pin;
	int phy_dm_pin;
	struct phy_context_t *phy_ctx;
};

struct esp32_usb_otg_fs_data {
	struct intr_handle_data_t *int_handle;
};

static void udc_dwc2_isr_handler(const struct device *dev);

static inline int esp32_usb_otg_init(const struct device *dev,
				     const struct esp32_usb_otg_fs_config *cfg,
				     struct esp32_usb_otg_fs_data *data)
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
#if defined(CONFIG_SOC_SERIES_ESP32P4)
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, USB_OTG11_IDDIG_PAD_IN_IDX,
				       false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, USB_SRP_BVALID_PAD_IN_IDX,
				       false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, USB_OTG11_VBUSVALID_PAD_IN_IDX,
				       false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ZERO_INPUT, USB_OTG11_AVALID_PAD_IN_IDX,
				       false);
#else
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, USB_OTG_IDDIG_IN_IDX, false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, USB_SRP_BVALID_IN_IDX, false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT, USB_OTG_VBUSVALID_IN_IDX,
				       false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ZERO_INPUT, USB_OTG_AVALID_IN_IDX, false);
#endif

	if (cfg->phy_ctx->target == USB_PHY_TARGET_INT) {
		gpio_ll_set_drive_capability(GPIO_LL_GET_HW(0), cfg->phy_dm_pin, GPIO_DRIVE_CAP_3);
		gpio_ll_set_drive_capability(GPIO_LL_GET_HW(0), cfg->phy_dp_pin, GPIO_DRIVE_CAP_3);
	}

	/* allocate interrupt but keep it disabled to avoid
	 * spurious suspend/resume event at enumeration phase
	 */
	ret = esp_intr_alloc(cfg->irq_source,
			     ESP_INTR_FLAG_INTRDISABLED | ESP_PRIO_TO_FLAGS(cfg->irq_priority) |
				     ESP_INT_FLAGS_CHECK(cfg->irq_flags),
			     (intr_handler_t)udc_dwc2_isr_handler, (void *)dev, &data->int_handle);

	return ret;
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

static inline int esp32_usb_otg_shutdown(const struct esp32_usb_otg_fs_config *cfg,
					 struct esp32_usb_otg_fs_data *data)
{
	usb_wrap_hal_disable();
	esp_intr_free(data->int_handle);

	return clock_control_off(cfg->clock_dev, cfg->clock_subsys);
}

#define QUIRK_ESP32_USB_OTG_INST(n)                                                                \
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
	static const struct esp32_usb_otg_fs_config usb_otg_config_##n = {                         \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, offset),            \
		.irq_source = DT_INST_IRQ_BY_IDX(n, 0, irq),                                       \
		.irq_priority = DT_INST_IRQ_BY_IDX(n, 0, priority),                                \
		.irq_flags = DT_INST_IRQ_BY_IDX(n, 0, flags),                                      \
		.phy_dp_pin = DT_INST_PROP(n, phy_dp_pin),                                         \
		.phy_dm_pin = DT_INST_PROP(n, phy_dm_pin),                                         \
		.phy_ctx = &phy_ctx_##n,                                                           \
	};                                                                                         \
                                                                                                   \
	static struct esp32_usb_otg_fs_data usb_otg_data_##n;                                      \
                                                                                                   \
	static int esp32_usb_otg_init_##n(const struct device *dev)                                \
	{                                                                                          \
		return esp32_usb_otg_init(dev, &usb_otg_config_##n, &usb_otg_data_##n);            \
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
		return esp32_usb_otg_shutdown(&usb_otg_config_##n, &usb_otg_data_##n);             \
	}                                                                                          \
                                                                                                   \
	const struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {                                 \
		.init = esp32_usb_otg_init_##n,                                                    \
		.pre_enable = esp32_usb_otg_enable_clk_##n,                                        \
		.post_enable = esp32_usb_otg_enable_phy_##n,                                       \
		.disable = esp32_usb_otg_disable_phy_##n,                                          \
		.shutdown = esp32_usb_otg_shutdown_##n,                                            \
	};

#ifndef UDC_DWC2_IRQ_DT_INST_DEFINE
#define UDC_DWC2_IRQ_DT_INST_DEFINE(n)                                                             \
	static void udc_dwc2_irq_enable_func_##n(const struct device *dev)                         \
	{                                                                                          \
		esp_intr_enable(usb_otg_data_##n.int_handle);                                      \
	}                                                                                          \
                                                                                                   \
	static void udc_dwc2_irq_disable_func_##n(const struct device *dev)                        \
	{                                                                                          \
		esp_intr_disable(usb_otg_data_##n.int_handle);                                     \
	}
#endif

/* Only expand for nodes that carry the full-speed vendor compatible, so this
 * quirk can coexist with the high-speed quirk when both controllers are enabled.
 */
#define QUIRK_ESP32_USB_OTG_DEFINE(n)                                                              \
	COND_CODE_1(DT_INST_NODE_HAS_COMPAT(n, espressif_esp32_usb_otg_fs),	\
		    (QUIRK_ESP32_USB_OTG_INST(n)), ())

DT_INST_FOREACH_STATUS_OKAY(QUIRK_ESP32_USB_OTG_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_ESP32_USB_OTG_FS_H */
