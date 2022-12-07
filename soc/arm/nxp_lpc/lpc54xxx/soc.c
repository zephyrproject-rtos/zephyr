/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_lpc54114 platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_lpc54114 platform.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <aarch32/cortex_m/exc.h>
#include <fsl_power.h>
#include <fsl_clock.h>
#include <fsl_common.h>
#include <fsl_device_registers.h>
#ifdef CONFIG_GPIO_MCUX_LPC
#include <fsl_pint.h>
#endif
#if  defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_SOC_LPC54114_M4)
#include <zephyr_image_info.h>
/* Memcpy macro to copy segments from secondary core image stored in flash
 * to RAM section that secondary core boots from.
 * n is the segment number, as defined in zephyr_image_info.h
 */
#define MEMCPY_SEGMENT(n, _)							\
	memcpy((uint32_t *)((SEGMENT_LMA_ADDRESS_ ## n) - ADJUSTED_LMA),	\
		(uint32_t *)(SEGMENT_LMA_ADDRESS_ ## n),			\
		(SEGMENT_SIZE_ ## n))
#endif

/**
 *
 * @brief Initialize the system clock
 *
 */
#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

static ALWAYS_INLINE void clock_init(void)
{

#ifdef CONFIG_SOC_LPC54114_M4
	/* Set up the clock sources */

	/* Ensure FRO is on */
	POWER_DisablePD(kPDRUNCFG_PD_FRO_EN);

	/*
	 * Switch to FRO 12MHz first to ensure we can change voltage without
	 * accidentally being below the voltage for current speed.
	 */
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	/* Set FLASH wait states for core */
	CLOCK_SetFLASHAccessCyclesForFreq(CPU_FREQ);

	/* Set up high frequency FRO output to selected frequency */
	CLOCK_SetupFROClocking(CPU_FREQ);

	/* Set up dividers */
	/* Set AHBCLKDIV divider to value 1 */
	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false);

	/* Set up clock selectors - Attach clocks to the peripheries */
	/* Switch MAIN_CLK to FRO_HF */
	CLOCK_AttachClk(kFRO_HF_to_MAIN_CLK);

	/* Attach 12 MHz clock to FLEXCOMM0 */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM0);

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm4), nxp_lpc_i2c, okay)
	/* attach 12 MHz clock to FLEXCOMM4 */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM4);

	/* reset FLEXCOMM for I2C */
	RESET_PeripheralReset(kFC4_RST_SHIFT_RSTn);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm5), nxp_lpc_spi, okay)
	/* Attach 12 MHz clock to FLEXCOMM5 */
	CLOCK_AttachClk(kFRO_HF_to_FLEXCOMM5);

	/* reset FLEXCOMM for SPI */
	RESET_PeripheralReset(kFC5_RST_SHIFT_RSTn);
#endif

#endif /* CONFIG_SOC_LPC54114_M4 */
}

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */

static int nxp_lpc54114_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	/* old interrupt lock level */
	unsigned int oldLevel;

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Initialize FRO/system clock to 48 MHz */
	clock_init();

#ifdef CONFIG_GPIO_MCUX_LPC
	/* Turn on PINT device*/
	PINT_Init(PINT);
#endif

	/*
	 * install default handler that simply resets the CPU if configured in
	 * the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);

	return 0;
}

SYS_INIT(nxp_lpc54114_init, PRE_KERNEL_1, 0);


#if defined(CONFIG_SECOND_CORE_MCUX) && defined(CONFIG_SOC_LPC54114_M4)

#define CORE_M0_BOOT_ADDRESS ((void *)CONFIG_SECOND_CORE_BOOT_ADDRESS_MCUX)

/**
 *
 * @brief Slave Init
 *
 * This routine boots the secondary core
 *
 * @retval 0 on success.
 *
 */
/* This function is also called at deep sleep resume. */
int _slave_init(const struct device *arg)
{
	int32_t temp;

	ARG_UNUSED(arg);

	/* Enable SRAM2, used by other core */
	SYSCON->AHBCLKCTRLSET[0] = SYSCON_AHBCLKCTRL_SRAM2_MASK;

	/* Copy second core image to SRAM */
	LISTIFY(SEGMENT_NUM, MEMCPY_SEGMENT, (;));

	/* Setup the reset handler pointer (PC) and stack pointer value.
	 * This is used once the second core runs its startup code.
	 * The second core first boots from flash (address 0x00000000)
	 * and then detects its identity (Cortex-M0, slave) and checks
	 * registers CPBOOT and CPSTACK and use them to continue the
	 * boot process.
	 * Make sure the startup code for the current core (Cortex-M4) is
	 * appropriate and shareable with the Cortex-M0 core!
	 */
	SYSCON->CPBOOT = SYSCON_CPBOOT_BOOTADDR(
			*(uint32_t *)((uint8_t *)CORE_M0_BOOT_ADDRESS + 0x4));
	SYSCON->CPSTACK = SYSCON_CPSTACK_STACKADDR(
			*(uint32_t *)CORE_M0_BOOT_ADDRESS);

	/* Reset the secondary core and start its clocks */
	temp = SYSCON->CPUCTRL;
	temp |= 0xc0c48000;
	SYSCON->CPUCTRL = (temp | SYSCON_CPUCTRL_CM0CLKEN_MASK
					| SYSCON_CPUCTRL_CM0RSTEN_MASK);
	SYSCON->CPUCTRL = (temp | SYSCON_CPUCTRL_CM0CLKEN_MASK)
					& (~SYSCON_CPUCTRL_CM0RSTEN_MASK);

	return 0;
}

SYS_INIT(_slave_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /*CONFIG_SECOND_CORE_MCUX && CONFIG_SOC_LPC54114_M4 */
