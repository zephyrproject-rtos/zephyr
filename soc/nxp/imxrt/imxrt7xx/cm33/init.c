/*
 * Copyright 2024,2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for NXP RT7XX platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the RT7XX platforms.
 */

#include <zephyr/init.h>
#include <zephyr/autoconf.h>
#include <zephyr/cache.h>
#include <soc.h>


#if defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_SOC_MIMXRT798S_CM33_CPU0)
#include <zephyr_image_info.h>
/* Memcpy macro to copy segments from secondary core image stored in flash
 * to RAM section that secondary core boots from.
 * n is the segment number, as defined in zephyr_image_info.h
 */
#define MEMCPY_SEGMENT(n, _)                                                                       \
	memcpy((uint32_t *)((SEGMENT_LMA_ADDRESS_##n) - ADJUSTED_LMA),                             \
	       (uint32_t *)(SEGMENT_LMA_ADDRESS_##n), (SEGMENT_SIZE_##n))
#endif

void soc_early_init_hook(void)
{
#if defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_SOC_MIMXRT798S_CM33_CPU0)
	/**
	 * Copy CM33 CPU1 core from flash to memory. Note that depending on where the
	 * user decided to store CPU1 code, this is likely going to read from the
	 * XSPI while using XIP. Provided we DO NOT WRITE TO THE XSPI,
	 * this operation is safe.
	 *
	 * Note that this copy MUST occur before enabling the CPU0 caching to
	 * ensure the data is written directly to RAM (since the CPU1 core will use it)
	 */
	LISTIFY(SEGMENT_NUM, MEMCPY_SEGMENT, (;));
#endif

	/* Enable data cache */
	sys_cache_data_enable();

	__ISB();
	__DSB();
}

void soc_reset_hook(void)
{
	SystemInit();
}

#if defined(CONFIG_LV_USE_DRAW_VG_LITE) || !defined(CONFIG_LV_USE_VG_LITE_DRIVER)
static int gpu_init_imxrt7xx(const struct device *dev)
{
	ARG_UNUSED(dev);

	POWER_DisablePD(kPDRUNCFG_APD_GPU);
	POWER_DisablePD(kPDRUNCFG_PPD_GPU);
	POWER_ApplyPD();

	/* VGPU clock enablement and divider selection */
	CLOCK_EnableClock(kCLOCK_MediaAccessRamArbiter0);
	CLOCK_EnableClock(kCLOCK_MediaAccessRamArbiter1);
	CLOCK_EnableClock(kCLOCK_Gpu);

	CLOCK_AttachClk(kFRO0_DIV1_to_VGPU);
	CLOCK_SetClkDiv(kCLOCK_DivVgpuClk, 1U);

	/* Enable GPU interrupt */
	RESET_ClearPeripheralReset(kVGPU_RST_SHIFT_RSTn);
	NVIC_SetPriority(VGPU_IRQn, 3);
	EnableIRQ((IRQn_Type)VGPU_IRQn);

	return 0;
}

DEVICE_DT_DEFINE(DT_INST(0, vsi_gpu_gc555), gpu_init_imxrt7xx, NULL,
		NULL, NULL, POST_KERNEL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

void gpu_cache_flush_range(void *addr, size_t size)
{
	sys_cache_data_flush_range(addr, size);
}
#endif /* CONFIG_LV_USE_DRAW_VG_LITE || !CONFIG_LV_USE_VG_LITE_DRIVER */
