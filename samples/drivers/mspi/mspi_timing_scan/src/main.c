/*
 * Copyright (c) 2025 Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>
#include <string.h>

#include "mspi_ambiq.h"

#if CONFIG_FLASH_MSPI
#define TARGET_DEVICE DT_ALIAS(flash0)
#elif CONFIG_MEMC_MSPI
#define TARGET_DEVICE DT_ALIAS(psram0)
#else
#error "Unsupported device type!!"
#endif

#define MSPI_AMBIQ_TIMING_CONFIG_DT(n)                                                       \
	{                                                                                    \
		.ui8WriteLatency    = DT_PROP_BY_IDX(n, ambiq_timing_config, 0),             \
		.ui8TurnAround      = DT_PROP_BY_IDX(n, ambiq_timing_config, 1),             \
		.bTxNeg             = DT_PROP_BY_IDX(n, ambiq_timing_config, 2),             \
		.bRxNeg             = DT_PROP_BY_IDX(n, ambiq_timing_config, 3),             \
		.bRxCap             = DT_PROP_BY_IDX(n, ambiq_timing_config, 4),             \
		.ui32TxDQSDelay     = DT_PROP_BY_IDX(n, ambiq_timing_config, 5),             \
		.ui32RxDQSDelay     = DT_PROP_BY_IDX(n, ambiq_timing_config, 6),             \
	}

#define MSPI_AMBIQ_TIMING_CONFIG_MASK_DT(n) DT_PROP(n, ambiq_timing_config_mask)

int main(void)
{
	const struct device *tar_bus = DEVICE_DT_GET(DT_BUS(TARGET_DEVICE));
	const struct device *tar_dev = DEVICE_DT_GET(TARGET_DEVICE);
	struct mspi_dev_id dev_id = MSPI_DEVICE_ID_DT(TARGET_DEVICE);
#if CONFIG_MEMC_MSPI
	struct mspi_xip_cfg tar_xip_cfg = MSPI_XIP_CONFIG_DT(TARGET_DEVICE);
#endif
	struct mspi_ambiq_timing_cfg tar_timing_cfg = MSPI_AMBIQ_TIMING_CONFIG_DT(TARGET_DEVICE);
	uint32_t timing_cfg_mask = MSPI_AMBIQ_TIMING_CONFIG_MASK_DT(TARGET_DEVICE);
	struct mspi_ambiq_timing_scan scan;

	if (!device_is_ready(tar_dev)) {
		printf("%s: device not ready.\n", tar_dev->name);
		return 1;
	}

#if CONFIG_MEMC_MSPI
	if (!tar_xip_cfg.enable) {
		printf("Need to enable XIP for timing scan.\n");
		return 1;
	}
#endif

	memset(&scan, 0, sizeof(struct mspi_ambiq_timing_scan));
	scan.range.txdqs_end  = 10;
	scan.range.rxdqs_end  = 31;
#if CONFIG_FLASH_MSPI
	scan.scan_type        = MSPI_AMBIQ_TIMING_SCAN_FLASH;
#elif CONFIG_MEMC_MSPI
	uint32_t base_addr    = DT_REG_ADDR_BY_IDX(DT_BUS(TARGET_DEVICE), 1);

	scan.scan_type        = MSPI_AMBIQ_TIMING_SCAN_MEMC;
	scan.device_addr      = base_addr +
				tar_xip_cfg.address_offset +
				tar_xip_cfg.size / 2;
#endif
	scan.min_window       = 6;
	printf("Starting MSPI Timing Scan.\n");
	if (mspi_ambiq_timing_scan(tar_dev, tar_bus, &dev_id, timing_cfg_mask,
				   (void *)&tar_timing_cfg, &scan)) {
		printf("Failed to timing scan\n");
		return 1;
	}
	printf("MSPI Timing Scan is successful.\n");
	printf("==========================\n");
	return 0;
}
