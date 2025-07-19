/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "bootloader_flash_priv.h"
#include <zephyr/storage/flash_map.h>
#include "ulp_lp_core.h"
#include "lp_core_uart.h"
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

void IRAM_ATTR lp_core_image_init(void)
{
	const uint32_t lpcore_img_off = FIXED_PARTITION_OFFSET(slot0_lpcore_partition);
	const uint32_t lpcore_img_size =
		DT_REG_SIZE(DT_NODELABEL(sramlp)) - DT_REG_SIZE(DT_NODELABEL(shmlp));
	int ret = 0;

	LOG_INF("Getting LPU image at %p, size %d", (void *)lpcore_img_off, lpcore_img_size);

	const uint8_t *data = (const uint8_t *)bootloader_mmap(lpcore_img_off, lpcore_img_size);

	ret = ulp_lp_core_load_binary(data, lpcore_img_size);
	if (ret) {
		LOG_ERR("Failed to load LP core image: %d", ret);
	}

	LOG_INF("LP core image loaded");

	/* Set LP core wakeup source as the HP CPU */
	ulp_lp_core_cfg_t cfg = {
		.wakeup_source = ULP_LP_CORE_WAKEUP_SOURCE_HP_CPU,
	};

	ret = ulp_lp_core_run(&cfg);
	if (ret) {
		LOG_ERR("Failed to start LP core: %d", ret);
	}
}

void soc_late_init_hook(void)
{
	lp_core_image_init();
}
