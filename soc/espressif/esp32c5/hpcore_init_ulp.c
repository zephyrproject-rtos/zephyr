/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>

#include "bootloader_flash_priv.h"
#include "ulp_lp_core.h"
#include "ulp_lp_core_memory_shared.h"
#include <esp_sleep.h>
#include <hal/lp_core_ll.h>
#include <esp_private/esp_pmu.h>

void IRAM_ATTR lp_core_image_init(void)
{
	/*
	 * Skip LP core loading on deep sleep wakeup - LP core is already
	 * running and LP RAM contents are preserved.
	 */
	uint32_t causes = esp_sleep_get_wakeup_causes();

	if (causes & BIT(ESP_SLEEP_WAKEUP_ULP)) {
		return;
	}

	const uint32_t lpcore_img_off = FIXED_PARTITION_OFFSET(slot0_lpcore_partition);
	const uint32_t lpcore_img_size = CONFIG_ESP32_ULP_COPROC_RESERVE_MEM;

	const uint8_t *data = (const uint8_t *)bootloader_mmap(lpcore_img_off, lpcore_img_size);

	if (ulp_lp_core_load_binary(data, lpcore_img_size) != 0) {
		return;
	}

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

	/*
	 * pmu_init() is never called in Zephyr (skipped due to
	 * CONFIG_BOOTLOADER_MCUBOOT). Call it here to configure
	 * PMU power parameters needed for LP->HP wakeup.
	 */
	pmu_init();

	ulp_lp_core_run(&cfg);

	/* Disable stall/reset so LP core survives HP deep sleep */
	lp_core_ll_stall_at_sleep_request(false);
	lp_core_ll_rst_at_sleep_enable(false);
}

void soc_late_init_hook(void)
{
	lp_core_image_init();
}
