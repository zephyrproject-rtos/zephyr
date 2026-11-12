/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * Initial contents based on soc/arm/mps2/soc.c which is:
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/linker/linker-defs.h>

#ifdef CONFIG_SOC_AN524

/* (Secure System Control) Base Address */
#define SSE_200_SYSTEM_CTRL_S_BASE     (0x50021000UL)
#define SSE_200_SYSTEM_CTRL_INITSVTOR1 (SSE_200_SYSTEM_CTRL_S_BASE + 0x114)
#define SSE_200_SYSTEM_CTRL_CPU_WAIT   (SSE_200_SYSTEM_CTRL_S_BASE + 0x118)
#define SSE_200_CPU_ID_UNIT_BASE       (0x5001F000UL)

/* The QSPI offset of the CPU1 application image. Matches the
 * Non-Secure partition defined by TF-M for the MPS3 AN524 platform.
 */
#define CPU1_FLASH_ADDRESS (0xC0000)

/* The memory map offset between the Secure and Non-Secure aliases,
 * used to determine the location of the CPU1 reset vector.
 */
#define CPU1_FLASH_OFFSET (0x10000000)

/**
 * @brief Wake up CPU 1 from another CPU, this is platform specific.
 */
void wakeup_cpu1(void)
{
	/* Set the Initial Secure Reset Vector Register for CPU 1 */
	*(uint32_t *)(SSE_200_SYSTEM_CTRL_INITSVTOR1) =
		(uint32_t)_vector_start + CPU1_FLASH_ADDRESS - CPU1_FLASH_OFFSET;

	/* Set the CPU Boot wait control after reset */
	*(uint32_t *)(SSE_200_SYSTEM_CTRL_CPU_WAIT) = 0;
}

/**
 * @brief Get the current CPU ID, this is platform specific.
 *
 * @return Current CPU ID
 */
uint32_t sse_200_platform_get_cpu_id(void)
{
	volatile uint32_t *p_cpu_id = (volatile uint32_t *)SSE_200_CPU_ID_UNIT_BASE;

	return (uint32_t)*p_cpu_id;
}

#endif /* CONFIG_SOC_AN524 */
