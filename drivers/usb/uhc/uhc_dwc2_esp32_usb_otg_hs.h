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
	unsigned int irq;
	void (*irq_configure)(void);
};

struct esp32_usb_otg_hs_data {
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

	/* Connect only: the line stays disabled to avoid a spurious
	 * suspend/resume event at the enumeration phase.
	 */
	cfg->irq_configure();

	return 0;
}

static int esp32_usb_otg_hs_shutdown(const struct device *const dev)
{
	const struct esp32_usb_otg_hs_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);
	int ret;

	irq_disable(cfg->irq);

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
	const struct esp32_usb_otg_hs_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);

	irq_enable(cfg->irq);
}

static void esp32_usb_otg_hs_irq_disable_func(const struct device *const dev)
{
	const struct esp32_usb_otg_hs_config *const cfg = UHC_DWC2_QUIRK_CONFIG(dev);

	irq_disable(cfg->irq);
}

#define QUIRK_ESP32_USB_OTG_HS_INST(n)						\
										\
	static void uhc_dwc2_quirk_irq_configure_##n(void)			\
	{									\
		IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, 0), IRQ_DEFAULT_PRIORITY,	\
			    uhc_dwc2_isr_handler, DEVICE_DT_INST_GET(n), 0);	\
	}									\
										\
	static const struct esp32_usb_otg_hs_config uhc_dwc2_quirk_config_##n = {\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_subsys = (clock_control_subsys_t)			\
			DT_INST_CLOCKS_CELL(n, offset),				\
		.irq = DT_INST_IRQN_BY_IDX(n, 0),				\
		.irq_configure = uhc_dwc2_quirk_irq_configure_##n,		\
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
