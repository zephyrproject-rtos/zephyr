/*
 * Copyright 2023 NXP
 *
 * SPDX-Licesnse-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control/clock_control_mcux_ccm_rev3.h>
#include <fsl_clock.h>
#include <errno.h>

#define IMX8QM_LPCG_REGMAP_SIZE 0x10000

static struct imx_ccm_clock clocks[] = {
	{
		.name = "adma_lpuart0_clock",
		.id = kCLOCK_DMA_Lpuart0,
		.lpcg_regmap_phys = DMA__LPCG_LPUART0_BASE,
		.lpcg_regmap_size = IMX8QM_LPCG_REGMAP_SIZE,
	},
	{
		.name = "adma_lpuart1_clock",
		.id = kCLOCK_DMA_Lpuart1,
		.lpcg_regmap_phys = DMA__LPCG_LPUART1_BASE,
		.lpcg_regmap_size = IMX8QM_LPCG_REGMAP_SIZE,
	},
	{
		.name = "adma_lpuart2_clock",
		.id = kCLOCK_DMA_Lpuart2,
		.lpcg_regmap_phys = DMA__LPCG_LPUART2_BASE,
		.lpcg_regmap_size = IMX8QM_LPCG_REGMAP_SIZE,
	},
};

struct imx_ccm_clock_config clock_config = {
	.clock_num = ARRAY_SIZE(clocks),
	.clocks = clocks,
};
