/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>

#include "bootloader_flash_priv.h"
#include "ulp_lp_core.h"
#include <esp_sleep.h>

void IRAM_ATTR lp_core_image_init(void)
{
	/*
	 * Skip LP core loading on deep sleep wakeup - LP core is already
	 * running and LP RAM contents are preserved.
	 */
	if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_ULP) {
		return;
	}

	const uint32_t lpcore_img_off = FIXED_PARTITION_OFFSET(slot0_lpcore_partition);
	const uint32_t lpcore_img_size = CONFIG_ESP32_ULP_COPROC_RESERVE_MEM;

	const uint8_t *data = (const uint8_t *)bootloader_mmap(lpcore_img_off, lpcore_img_size);

	if (ulp_lp_core_load_binary(data, lpcore_img_size) != 0) {
		return;
	}

	/*
	 * On C6, lp_core_ll_reset_register() is a no-op, so HP_CPU
	 * SW trigger is the only way to boot the LP core initially.
	 * The selected wakeup source is OR'd with HP_CPU here.
	 */
	uint32_t wakeup_source = ULP_LP_CORE_WAKEUP_SOURCE_HP_CPU;

#if IS_ENABLED(CONFIG_ESP32_ULP_LP_CORE_WAKEUP_SOURCE_LP_TIMER)
	wakeup_source |= ULP_LP_CORE_WAKEUP_SOURCE_LP_TIMER;
#elif IS_ENABLED(CONFIG_ESP32_ULP_LP_CORE_WAKEUP_SOURCE_LP_IO)
	wakeup_source |= ULP_LP_CORE_WAKEUP_SOURCE_LP_IO;
#endif

	ulp_lp_core_cfg_t cfg = {
		.wakeup_source = wakeup_source,
#if IS_ENABLED(CONFIG_ESP32_ULP_LP_CORE_WAKEUP_SOURCE_LP_TIMER)
		.lp_timer_sleep_duration_us = CONFIG_ESP32_ULP_LP_CORE_LP_TIMER_SLEEP_DURATION_US,
#endif
	};

	ulp_lp_core_run(&cfg);
}

void soc_late_init_hook(void)
{
	lp_core_image_init();
}
