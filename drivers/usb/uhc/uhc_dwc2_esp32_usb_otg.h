/*
 * Copyright (c) 2026 Roman Leonov <jam_roma@yahoo.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UHC_DWC2_ESP32_USB_OTG_H
#define ZEPHYR_DRIVERS_USB_UHC_DWC2_ESP32_USB_OTG_H

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

struct esp32_usb_otg_config {
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	int irq_source;
	int irq_priority;
	int irq_flags;
	usb_phy_target_t phy_target;
};

struct esp32_usb_otg_data {
	struct intr_handle_data_t *int_handle;
	usb_wrap_hal_context_t wrap_hal;
};

static void uhc_dwc2_isr_handler(const struct device *dev);

static inline int esp32_usb_otg_init(const struct device *dev)
{
	const struct esp32_usb_otg_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);
	struct esp32_usb_otg_data *const data = UHC_DWC2_QUIRK_DATA(dev);

	int ret;

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);

	if (ret != 0) {
		return ret;
	}

	/* pinout config to work in USB_OTG_MODE_HOST */
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ZERO_INPUT,
					USB_OTG_IDDIG_IN_IDX,
					false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ZERO_INPUT,
					USB_SRP_BVALID_IN_IDX,
					false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT,
					USB_OTG_VBUSVALID_IN_IDX,
					false);
	esp_rom_gpio_connect_in_signal(GPIO_MATRIX_CONST_ONE_INPUT,
					USB_OTG_AVALID_IN_IDX,
					false);

	/* allocate interrupt but keep it disabled to avoid
	 * spurious suspend/resume event at enumeration phase
	 */
	ret = esp_intr_alloc(cfg->irq_source,
			ESP_INTR_FLAG_INTRDISABLED |
			ESP_PRIO_TO_FLAGS(cfg->irq_priority) |
					ESP_INT_FLAGS_CHECK(cfg->irq_flags),
			(intr_handler_t)uhc_dwc2_isr_handler,
			(void *)dev,
			&data->int_handle);

	return ret;
}

static int esp32_usb_otg_enable_phy(const struct device *const dev)
{
	const struct esp32_usb_otg_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);
	struct esp32_usb_otg_data *const data = UHC_DWC2_QUIRK_DATA(dev);

	usb_wrap_hal_init(&data->wrap_hal);
	usb_wrap_ll_phy_enable_pad(data->wrap_hal.dev, true);

#if USB_WRAP_LL_EXT_PHY_SUPPORTED
	if (cfg->phy_target == USB_PHY_TARGET_INT) {
		usb_wrap_hal_phy_set_external(&data->wrap_hal, false);
	} else {
		/* If External PHY target is supported, set external to true */
		LOG_WRN("esp32 phy: External phy support isn't implemented");
		return -EINVAL;
	}
#endif

	if (cfg->phy_target == USB_PHY_TARGET_INT) {
		/* Set drive capabilities for DM and DP */
		gpio_ll_set_drive_capability(GPIO_LL_GET_HW(0), USBPHY_DM_NUM, GPIO_DRIVE_CAP_3);
		gpio_ll_set_drive_capability(GPIO_LL_GET_HW(0), USBPHY_DP_NUM, GPIO_DRIVE_CAP_3);
		/* Configure pull resistors for Host DM and DP */
		usb_wrap_pull_override_vals_t vals = {
			.dp_pu = false,
			.dm_pu = false,
			.dp_pd = true,
			.dm_pd = true,
		};
		usb_wrap_hal_phy_enable_pull_override(&data->wrap_hal, &vals);
	}

	LOG_DBG("PHY enabled");
	return 0;
}

static int esp32_usb_otg_disable_phy(const struct device *const dev)
{
	struct esp32_usb_otg_data *const data = UHC_DWC2_QUIRK_DATA(dev);

	usb_wrap_hal_disable();
	usb_wrap_ll_phy_enable_pad(data->wrap_hal.dev, false);

	LOG_DBG("PHY disabled");
	return 0;
}

static int esp32_usb_otg_pre_init(const struct device *const dev)
{
	int ret;

	/* Power up PHY */
	ret = esp32_usb_otg_enable_phy(dev);

	if (ret) {
		LOG_ERR("Unable to power on esp32 usb otg PHY");
		return ret;
	}

	/* Enable clock, configure pins and init interrupt */
	return esp32_usb_otg_init(dev);
}

static int esp32_usb_otg_shutdown(const struct device *const dev)
{
	/* TODO: Disable clock, disconnect gpio signals and deregister the interrupt */

	/* Power down the PHY */
	return esp32_usb_otg_disable_phy(dev);
}

static void esp32_usb_otg_irq_enable_func(const struct device *const dev)
{
	struct esp32_usb_otg_data *const data = UHC_DWC2_QUIRK_DATA(dev);

	esp_intr_enable(data->int_handle);
}

static void esp32_usb_otg_irq_disable_func(const struct device *const dev)
{
	struct esp32_usb_otg_data *const data = UHC_DWC2_QUIRK_DATA(dev);

	esp_intr_disable(data->int_handle);
}

#define QUIRK_ESP32_USB_OTG_DEFINE(n)						\
										\
	static const struct esp32_usb_otg_config uhc_dwc2_quirk_config_##n = {	\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_subsys = (clock_control_subsys_t)			\
			DT_INST_CLOCKS_CELL(n, offset),				\
		.irq_source = DT_INST_IRQ_BY_IDX(n, 0, irq),			\
		.irq_priority = DT_INST_IRQ_BY_IDX(n, 0, priority),		\
		.irq_flags = DT_INST_IRQ_BY_IDX(n, 0, flags),			\
		.phy_target = USB_PHY_TARGET_INT,				\
	};									\
										\
	static struct esp32_usb_otg_data uhc_dwc2_quirk_data_##n;		\
										\
	const struct uhc_dwc2_vendor_quirks uhc_dwc2_vendor_quirks_##n = {	\
		.pre_init = esp32_usb_otg_pre_init,				\
		.shutdown = esp32_usb_otg_shutdown,				\
	};

#define UHC_DWC2_IRQ_DT_INST_DEFINE(n)						\
	static void uhc_dwc2_irq_enable_func_##n(const struct device *dev)	\
	{									\
		esp32_usb_otg_irq_enable_func(dev);				\
	}									\
										\
	static void uhc_dwc2_irq_disable_func_##n(const struct device *dev)	\
	{									\
		esp32_usb_otg_irq_disable_func(dev);				\
	}

DT_INST_FOREACH_STATUS_OKAY(QUIRK_ESP32_USB_OTG_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UHC_DWC2_ESP32_USB_OTG_H */
