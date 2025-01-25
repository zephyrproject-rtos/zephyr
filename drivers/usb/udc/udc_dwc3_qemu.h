/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_UDC_DWC3_QEMU_H
#define ZEPHYR_DRIVERS_USB_UDC_DWC3_QEMU_H

struct udc_dwc3_qemu_data {
	uint32_t custom_content;
};

struct udc_dwc3_qemu_config {
	uint32_t custom_content;
};

int udc_dwc3_qemu_init(const struct device *const dev)
{
	const struct udc_dwc3_config *const config = dev->config;
	struct udc_dwc3_qemu_data *const quirk_data = config->quirk_data;
	const struct udc_dwc3_qemu_config *const quirk_config = config->quirk_config;

	LOG_DBG("Example of quirk for init(): data=0x%x config=0x%x",
		quirk_data->custom_content, quirk_config->custom_content);

	return 0;
}

const struct udc_dwc3_vendor_quirks udc_dwc3_vendor_quirks = {
	.init = udc_dwc3_qemu_init,
};

#define UDC_DWC3_QUIRK_DEFINE(n)						\
	static struct udc_dwc3_qemu_config udc_dwc3_quirk_config_##n = {	\
		.custom_content = 0x1234					\
	};									\
	static struct udc_dwc3_qemu_data udc_dwc3_quirk_data_##n = {		\
		.custom_content = 0x1234					\
	};

#endif /* ZEPHYR_DRIVERS_USB_UDC_DWC3_QEMU_H */
