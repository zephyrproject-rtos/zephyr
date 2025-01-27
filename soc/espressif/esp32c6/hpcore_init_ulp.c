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
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

void IRAM_ATTR lp_core_image_init(void)
{
	const uint32_t lpcore_img_off = FIXED_PARTITION_OFFSET(lpcore_partition);
	const uint32_t lpcore_img_size = FIXED_PARTITION_SIZE(lpcore_partition);
	int ret = 0;

	printf("Getting LPU image at %p, size %d\n", (void *)lpcore_img_off, lpcore_img_size);

	const uint8_t *data = (const uint8_t *)bootloader_mmap(lpcore_img_off, lpcore_img_size);

	ret = ulp_lp_core_load_binary(data, lpcore_img_size);
	if (ret) {
		printf("Failed to load LP core image: %d\n", ret);
	}

	printf("LP core image loaded\n");

	/* Set LP core wakeup source as the HP CPU */
	ulp_lp_core_cfg_t cfg = {
		.wakeup_source = ULP_LP_CORE_WAKEUP_SOURCE_HP_CPU,
	};

	ret = ulp_lp_core_run(&cfg);
	if (ret) {
		printf("Failed to start LP core: %d\n", ret);
	}
}

void soc_late_init_hook(void)
{
	lp_core_image_init();
}
