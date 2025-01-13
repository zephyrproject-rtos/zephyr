/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_INCLUDE_MIC_PRIVACY_H__
#define __ZEPHYR_INCLUDE_MIC_PRIVACY_H__

#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/devicetree.h>

enum mic_privacy_policy {
	MIC_PRIVACY_DISABLED = 0,
	MIC_PRIVACY_HW_MANAGED = 1,
	MIC_PRIVACY_FW_MANAGED = 2,
	MIC_PRIVACY_FORCE_MIC_DISABLED = 3,
};

/* has to match HW register definition
 * (DZLS bit field in DFMICPVCP register)
 */
union mic_privacy_mask {
	uint32_t value;
	struct {
		uint32_t sndw:7;
		uint32_t dmic:1;
	};
};

struct intel_adsp_mic_priv_data {
	uint8_t rsvd;
};

struct intel_adsp_mic_priv_cfg {
	uint32_t base;
	uint32_t regblock_size;
};

struct mic_privacy_api_funcs {
	void (*enable_fw_managed_irq)(bool enable_irq, const void *fn);
	void (*clear_fw_managed_irq)();
	void (*enable_dmic_irq)(bool enable_irq, const void *fn);
	bool (*get_dmic_irq_status)(void);
	void (*clear_dmic_irq_status)(void);
	enum mic_privacy_policy (*get_policy)();
	uint32_t (*get_privacy_policy_register_raw_value)();
	uint32_t (*get_dma_data_zeroing_wait_time)();
	uint32_t (*get_dma_data_zeroing_link_select)();
	uint32_t (*get_dmic_mic_disable_status)(void);
	uint32_t (*get_fw_managed_mic_disable_status)();
	void (*set_fw_managed_mode)(bool is_fw_managed_enabled);
	void (*set_fw_mic_disable_status)(bool fw_mic_disable_status);
	uint32_t (*get_fw_mic_disable_status)();
};

#endif
