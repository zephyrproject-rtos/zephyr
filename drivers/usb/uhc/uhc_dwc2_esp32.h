/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Roman Leonov <jam_roma@yahoo.com>
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

struct uhc_dwc2_quirk_data {
	int placeholder;
};

static void uhc_dwc2_isr_handler(const struct device *const dev);

#define UHC_DWC2_IRQ_DT_INST_DEFINE(n)						\
	static void uhc_dwc2_irq_enable_func_##n(const struct device *const dev)\
	{									\
		(void)uhc_dwc2_isr_handler;					\
		/* TODO: esp_intr_enable */					\
	}									\
										\
	static void uhc_dwc2_irq_disable_func_##n(const struct device *const dev)\
	{									\
		/* TODO: esp_intr_enable */					\
	}

#define UHC_DWC2_QUIRK_DEFINE(n)						\
	static struct uhc_dwc2_quirk_data uhc_dwc2_quirk_data_##n = {		\
		.placeholder = 0x1234,						\
	};
