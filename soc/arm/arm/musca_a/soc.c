/*
 * Copyright (c) 2018-2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/cpu.h>
#include <init.h>
#include <soc.h>

/* (Secure System Control) Base Address */
#define SSE_200_SYSTEM_CTRL_S_BASE	(0x50021000UL)
#define SSE_200_SYSTEM_CTRL_INITSVTOR1	(SSE_200_SYSTEM_CTRL_S_BASE + 0x114)
#define SSE_200_SYSTEM_CTRL_CPU_WAIT	(SSE_200_SYSTEM_CTRL_S_BASE + 0x118)
#define SSE_200_CPU_ID_UNIT_BASE	(0x5001F000UL)

#define NON_SECURE_FLASH_ADDRESS	(192 * 1024)
#define NON_SECURE_IMAGE_HEADER	(0x400)
#define NON_SECURE_FLASH_OFFSET	(0x10000000)

/**
 * @brief Wake up CPU 1 from another CPU, this is plaform specific.
 *
 */
void wakeup_cpu1(void)
{
	/* Set the Initial Secure Reset Vector Register for CPU 1 */
	*(u32_t *)(SSE_200_SYSTEM_CTRL_INITSVTOR1) =
					CONFIG_FLASH_BASE_ADDRESS +
					NON_SECURE_FLASH_ADDRESS +
					NON_SECURE_IMAGE_HEADER -
					NON_SECURE_FLASH_OFFSET;

	/* Set the CPU Boot wait control after reset */
	*(u32_t *)(SSE_200_SYSTEM_CTRL_CPU_WAIT) = 0;
}

/**
 * @brief Get the current CPU ID, this is plaform specific.
 *
 * @return Current CPU ID
 */
u32_t sse_200_platform_get_cpu_id(void)
{
	volatile u32_t *p_cpu_id = (volatile u32_t *)SSE_200_CPU_ID_UNIT_BASE;

	return (u32_t)*p_cpu_id;
}

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * @return 0
 */
static int arm_musca_init(struct device *arg)
{
	ARG_UNUSED(arg);

	/*
	 * Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	return 0;
}

SYS_INIT(arm_musca_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
