/*
 * Copyright (c) 2017 Linaro Limited
 *
 * Initial contents based on soc/arm/ti_lm3s6965/soc.c which is:
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/gpio/gpio_mmio32.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/linker/linker-defs.h>


/* Setup GPIO drivers for accessing FPGAIO registers */
#define FPGAIO_NODE(n) DT_INST(n, arm_mps2_fpgaio_gpio)
#define FPGAIO_INIT(n)						\
	GPIO_MMIO32_INIT(FPGAIO_NODE(n),			\
			DT_REG_ADDR(FPGAIO_NODE(n)),		\
			BIT_MASK(DT_PROP(FPGAIO_NODE(n), ngpios)))

/* We expect there to be 3 arm,mps2-fpgaio-gpio devices:
 * led0, button, and misc
 */
FPGAIO_INIT(0);
FPGAIO_INIT(1);
FPGAIO_INIT(2);

/* (Secure System Control) Base Address */
#define SSE_200_SYSTEM_CTRL_S_BASE	(0x50021000UL)
#define SSE_200_SYSTEM_CTRL_INITSVTOR1	(SSE_200_SYSTEM_CTRL_S_BASE + 0x114)
#define SSE_200_SYSTEM_CTRL_CPU_WAIT	(SSE_200_SYSTEM_CTRL_S_BASE + 0x118)
#define SSE_200_CPU_ID_UNIT_BASE	(0x5001F000UL)

/* The base address that the application image will start at on the secondary
 * (non-TrustZone) Cortex-M33 mcu.
 */
#define CPU1_FLASH_ADDRESS      (0x38B000)

/* The memory map offset for the application image, which is used
 * to determine the location of the reset vector at startup.
 */
#define CPU1_FLASH_OFFSET       (0x10000000)

/**
 * @brief Wake up CPU 1 from another CPU, this is platform specific.
 */
void wakeup_cpu1(void)
{
	/* Set the Initial Secure Reset Vector Register for CPU 1 */
	*(uint32_t *)(SSE_200_SYSTEM_CTRL_INITSVTOR1) =
		(uint32_t)_vector_start +
		CPU1_FLASH_ADDRESS -
		CPU1_FLASH_OFFSET;

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

/**
 * @brief Perform basic hardware initialization at boot.
 *
 * @return 0
 */
static int arm_mps2_init(void)
{

	/*
	 * Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	return 0;
}

SYS_INIT(arm_mps2_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
