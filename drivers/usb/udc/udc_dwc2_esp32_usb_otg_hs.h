/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC2_ESP32_USB_OTG_HS_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC2_ESP32_USB_OTG_HS_H

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

static void udc_dwc2_isr_handler(const struct device *dev);

static inline int esp32_usb_otg_hs_init(const struct device *dev,
					const struct esp32_usb_otg_hs_config *cfg,
					struct esp32_usb_otg_hs_data *data)
{
	int ret;

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);

	if (ret != 0) {
		return ret;
	}

	/* keep the interrupt disabled to avoid a spurious event during enumeration */
	ret = esp_intr_alloc(cfg->irq_source,
			     ESP_INTR_FLAG_INTRDISABLED | ESP_PRIO_TO_FLAGS(cfg->irq_priority) |
				     ESP_INT_FLAGS_CHECK(cfg->irq_flags),
			     (intr_handler_t)udc_dwc2_isr_handler, (void *)dev, &data->int_handle);

	return ret;
}

static inline int esp32_usb_otg_hs_enable_clk(struct esp32_usb_otg_hs_data *data)
{
	usb_utmi_hal_init(&data->utmi_hal);

	/* Device mode requires the D+/D- 15k pulldown resistors to be
	 * disconnected, otherwise the host never detects the attached
	 * device. The HAL gates the access on the silicon revision.
	 */
	usb_utmi_hal_enable_data_pulldowns(false);

	return 0;
}

static inline int esp32_usb_otg_hs_post_enable(mem_addr_t base_addr)
{
	struct usb_dwc2_reg *const base = (struct usb_dwc2_reg *)base_addr;

	/* The controller reports HSPHYTYPE as both UTMI+ and ULPI, so the
	 * generic driver selects ULPI. Only the internal UTMI PHY exists,
	 * so force UTMI after init_controller has set everything else.
	 */
	sys_clear_bits((mem_addr_t)&base->gusbcfg, USB_DWC2_GUSBCFG_ULPI_UTMI_SEL_ULPI);

	return 0;
}

static inline int esp32_usb_otg_hs_caps(const struct device *dev)
{
	struct udc_data *data = dev->data;

	data->caps.hs = true;

	return 0;
}

static inline int esp32_usb_otg_hs_disable(struct esp32_usb_otg_hs_data *data)
{
	usb_utmi_hal_disable();

	return 0;
}

static inline int esp32_usb_otg_hs_shutdown(const struct esp32_usb_otg_hs_config *cfg,
					    struct esp32_usb_otg_hs_data *data)
{
	usb_utmi_hal_disable();
	esp_intr_free(data->int_handle);

	return clock_control_off(cfg->clock_dev, cfg->clock_subsys);
}

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

#define QUIRK_ESP32_USB_OTG_HS_INST(n)                                                             \
                                                                                                   \
	static const struct esp32_usb_otg_hs_config usb_otg_hs_config_##n = {                      \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, offset),            \
		.irq_source = DT_INST_IRQ_BY_IDX(n, 0, irq),                                       \
		.irq_priority = DT_INST_IRQ_BY_IDX(n, 0, priority),                                \
		.irq_flags = DT_INST_IRQ_BY_IDX(n, 0, flags),                                      \
	};                                                                                         \
                                                                                                   \
	static struct esp32_usb_otg_hs_data usb_otg_data_##n;                                      \
                                                                                                   \
	static int esp32_usb_otg_hs_init_##n(const struct device *dev)                             \
	{                                                                                          \
		return esp32_usb_otg_hs_init(dev, &usb_otg_hs_config_##n, &usb_otg_data_##n);      \
	}                                                                                          \
                                                                                                   \
	static int esp32_usb_otg_hs_enable_clk_##n(const struct device *dev)                       \
	{                                                                                          \
		return esp32_usb_otg_hs_enable_clk(&usb_otg_data_##n);                             \
	}                                                                                          \
                                                                                                   \
	static int esp32_usb_otg_hs_post_enable_##n(const struct device *dev)                      \
	{                                                                                          \
		return esp32_usb_otg_hs_post_enable(DT_INST_REG_ADDR(n));                          \
	}                                                                                          \
                                                                                                   \
	static int esp32_usb_otg_hs_disable_##n(const struct device *dev)                          \
	{                                                                                          \
		return esp32_usb_otg_hs_disable(&usb_otg_data_##n);                                \
	}                                                                                          \
                                                                                                   \
	static int esp32_usb_otg_hs_shutdown_##n(const struct device *dev)                         \
	{                                                                                          \
		return esp32_usb_otg_hs_shutdown(&usb_otg_hs_config_##n, &usb_otg_data_##n);       \
	}                                                                                          \
                                                                                                   \
	const struct dwc2_vendor_quirks dwc2_vendor_quirks_##n = {                                 \
		.init = esp32_usb_otg_hs_init_##n,                                                 \
		.pre_enable = esp32_usb_otg_hs_enable_clk_##n,                                     \
		.post_enable = esp32_usb_otg_hs_post_enable_##n,                                   \
		.caps = esp32_usb_otg_hs_caps,                                                     \
		.disable = esp32_usb_otg_hs_disable_##n,                                           \
		.shutdown = esp32_usb_otg_hs_shutdown_##n,                                         \
	};

/* Only expand for nodes that carry the HS vendor compatible, so this quirk
 * can coexist with the FS quirk when both controllers are enabled.
 */
#define QUIRK_ESP32_USB_OTG_HS_DEFINE(n)                                                           \
	COND_CODE_1(DT_INST_NODE_HAS_COMPAT(n, espressif_esp32_usb_otg_hs),                        \
		    (QUIRK_ESP32_USB_OTG_HS_INST(n)), ())

DT_INST_FOREACH_STATUS_OKAY(QUIRK_ESP32_USB_OTG_HS_DEFINE)

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC2_ESP32_USB_OTG_HS_H */
