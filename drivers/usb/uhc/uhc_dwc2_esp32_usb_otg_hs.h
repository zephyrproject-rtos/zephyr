/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UHC_DWC2_ESP32_USB_OTG_HS_H
#define ZEPHYR_DRIVERS_USB_UHC_DWC2_ESP32_USB_OTG_HS_H

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/logging/log.h>

#include <hal/usb_utmi_hal.h>

struct esp32_usb_otg_hs_config {
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	int irq_source;
	int irq_priority;
	int irq_flags;
};

struct esp32_usb_otg_hs_data {
	struct intr_handle_data_t *int_handle;
	usb_utmi_hal_context_t utmi_hal;
};

static void uhc_dwc2_isr_handler(const struct device *dev);

static inline int esp32_usb_otg_hs_pre_init(const struct device *dev)
{
	const struct esp32_usb_otg_hs_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);
	struct esp32_usb_otg_hs_data *const data = UHC_DWC2_QUIRK_DATA(dev);
	int ret;

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (ret != 0) {
		return ret;
	}

	/* Bring up the UTMI PHY before the core is reset and configured. The
	 * HAL init resets the PHY, so it must not run once the host port is
	 * powered.
	 */
	usb_utmi_hal_init(&data->utmi_hal);

	/* Host mode requires the 15k pulldown resistors on D+/D-, otherwise
	 * the PHY never reports a line state change and no connection is
	 * detected. The HAL gates the access on the silicon revision.
	 */
	usb_utmi_hal_enable_data_pulldowns(true);

	/* allocate interrupt but keep it disabled to avoid
	 * spurious suspend/resume event at enumeration phase
	 */
	ret = esp_intr_alloc(cfg->irq_source,
			     ESP_INTR_FLAG_INTRDISABLED | ESP_PRIO_TO_FLAGS(cfg->irq_priority) |
				     ESP_INT_FLAGS_CHECK(cfg->irq_flags),
			     (intr_handler_t)uhc_dwc2_isr_handler, (void *)dev, &data->int_handle);

	return ret;
}

static int esp32_usb_otg_hs_shutdown(const struct device *const dev)
{
	const struct esp32_usb_otg_hs_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);
	struct esp32_usb_otg_hs_data *const data = UHC_DWC2_QUIRK_DATA(dev);
	int ret;

	if (data->int_handle != NULL) {
		ret = esp_intr_disable(data->int_handle);
		if (ret != 0) {
			LOG_ERR("Unable to disable interrupt: %d", ret);
			return ret;
		}

		ret = esp_intr_free(data->int_handle);
		if (ret != 0) {
			LOG_ERR("Unable to free interrupt: %d", ret);
			return ret;
		}

		data->int_handle = NULL;
	}

	usb_utmi_hal_enable_data_pulldowns(false);
	usb_utmi_hal_disable();

	ret = clock_control_off(cfg->clock_dev, cfg->clock_subsys);
	if (ret != 0) {
		LOG_ERR("Unable to off the clock: %d", ret);
	}

	return ret;
}

static void esp32_usb_otg_hs_irq_enable_func(const struct device *const dev)
{
	struct esp32_usb_otg_hs_data *const data = UHC_DWC2_QUIRK_DATA(dev);

	esp_intr_enable(data->int_handle);
}

static void esp32_usb_otg_hs_irq_disable_func(const struct device *const dev)
{
	struct esp32_usb_otg_hs_data *const data = UHC_DWC2_QUIRK_DATA(dev);

	esp_intr_disable(data->int_handle);
}

#define QUIRK_ESP32_USB_OTG_HS_INST(n)						\
										\
	static const struct esp32_usb_otg_hs_config uhc_dwc2_quirk_config_##n = {\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_subsys = (clock_control_subsys_t)			\
			DT_INST_CLOCKS_CELL(n, offset),				\
		.irq_source = DT_INST_IRQ_BY_IDX(n, 0, irq),			\
		.irq_priority = DT_INST_IRQ_BY_IDX(n, 0, priority),		\
		.irq_flags = DT_INST_IRQ_BY_IDX(n, 0, flags),			\
	};									\
										\
	static struct esp32_usb_otg_hs_data uhc_dwc2_quirk_data_##n;		\
										\
	static const struct uhc_dwc2_vendor_quirks uhc_dwc2_vendor_quirks_##n = {\
		.pre_init = esp32_usb_otg_hs_pre_init,				\
		.shutdown = esp32_usb_otg_hs_shutdown,				\
	};

#define UHC_DWC2_IRQ_DT_INST_DEFINE(n)						\
	static void uhc_dwc2_irq_enable_func_##n(const struct device *dev)	\
	{									\
		esp32_usb_otg_hs_irq_enable_func(dev);				\
	}									\
										\
	static void uhc_dwc2_irq_disable_func_##n(const struct device *dev)	\
	{									\
		esp32_usb_otg_hs_irq_disable_func(dev);				\
	}

#define QUIRK_ESP32_USB_OTG_HS_DEFINE(n)					\
	COND_CODE_1(DT_INST_NODE_HAS_COMPAT(n, espressif_esp32_usb_otg_hs),	\
		    (QUIRK_ESP32_USB_OTG_HS_INST(n)), ())

DT_INST_FOREACH_STATUS_OKAY(QUIRK_ESP32_USB_OTG_HS_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UHC_DWC2_ESP32_USB_OTG_HS_H */
