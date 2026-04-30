/*
 * Copyright 2023-2024, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>

#include <soc.h>

#include <fsl_ccm32k.h>
#include <fsl_common.h>
#include <fsl_clock.h>
#include <fsl_cmc.h>

#define MCXW7_CMC_ADDR (CMC_Type *)DT_REG_ADDR(DT_INST(0, nxp_cmc))

extern uint32_t SystemCoreClock;
extern void nxp_nbu_init(void);

#ifdef CONFIG_NXP_MCXW7XX_BOOT_HEADER
extern char z_main_stack[];
extern char _flash_used[];

extern void z_arm_reset(void);
extern void z_arm_nmi(void);
extern void z_arm_hard_fault(void);
extern void z_arm_mpu_fault(void);
extern void z_arm_bus_fault(void);
extern void z_arm_usage_fault(void);
extern void z_arm_secure_fault(void);
extern void z_arm_svc(void);
extern void z_arm_debug_monitor(void);
extern void z_arm_pendsv(void);
extern void sys_clock_isr(void);
extern void z_arm_exc_spurious(void);

#ifdef CONFIG_USE_SWITCH
#define PENDSV_VEC z_arm_exc_spurious
#else
#define PENDSV_VEC z_arm_pendsv
#endif

__imx_boot_ivt_section void (*const image_vector_table[])(void) = {
	(void (*)())(z_main_stack + CONFIG_MAIN_STACK_SIZE), /* 0x00 */
	z_arm_reset,                                         /* 0x04 */
	z_arm_nmi,                                           /* 0x08 */
	z_arm_hard_fault,                                    /* 0x0C */
	z_arm_mpu_fault,                                     /* 0x10 */
	z_arm_bus_fault,                                     /* 0x14 */
	z_arm_usage_fault,                                   /* 0x18 */
#if defined(CONFIG_ARM_SECURE_FIRMWARE)
	z_arm_secure_fault, /* 0x1C */
#else
	z_arm_exc_spurious,
#endif                                               /* CONFIG_ARM_SECURE_FIRMWARE */
	(void (*)())((uintptr_t)_flash_used),        /* 0x20, imageLength. */
	0,                                           /* 0x24, imageType (Plain Image) */
	0,                                           /* 0x28, authBlockOffset/crcChecksum */
	z_arm_svc,                                   /* 0x2C */
	z_arm_debug_monitor,                         /* 0x30 */
	(void (*)())((uintptr_t)image_vector_table), /* 0x34, imageLoadAddress. */
	PENDSV_VEC,                                /* 0x38 */
#if defined(CONFIG_SYS_CLOCK_EXISTS) && defined(CONFIG_CORTEX_M_SYSTICK_INSTALL_ISR)
	sys_clock_isr, /* 0x3C */
#else
	z_arm_exc_spurious,
#endif
};
#endif /* CONFIG_NXP_MCXW7XX_BOOT_HEADER */

#ifndef CONFIG_SOC_MCXW70AC
static void vbat_init(void)
{
	VBAT_Type *base = (VBAT_Type *)DT_REG_ADDR(DT_NODELABEL(vbat));

	/* Write 1 to Clear POR detect status bit.
	 *
	 * Clearing this bit is acknowledement
	 * that software has recognized a power on reset.
	 *
	 * This avoids also niche issues with NVIC read/write
	 * when searching for available interrupt lines.
	 */
	base->STATUSA |= VBAT_STATUSA_POR_DET_MASK;
};
#endif

void soc_early_init_hook(void)
{
	unsigned int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

#ifndef CONFIG_SOC_MCXW70AC
	/* Smart power switch initialization */
	vbat_init();
#endif

	if (IS_ENABLED(CONFIG_PM)) {
		nxp_mcxw7x_power_init();
	}

	/* restore interrupt state */
	irq_unlock(oldLevel);
}

static int soc_nbu_init(void)
{
#if defined(CONFIG_NXP_NBU)
	nxp_nbu_init();
#elif defined(CONFIG_PM)
	/* Shutdown NBU as not used */

	/* Reset all RFMC registers and put the NBU CM3 in reset */
	RFMC->CTRL |= RFMC_CTRL_RFMC_RST(0x1U);
	/* Wait for a few microseconds before releasing the NBU reset,
	 * without this the system may hang in the loop waiting for FRO clock valid
	 */
	k_busy_wait(31U);
	/* Release NBU reset */
	RFMC->CTRL &= ~RFMC_CTRL_RFMC_RST_MASK;

	/* NBU was probably in low power before the RFMC reset, so we need to wait for the FRO clock
	 * to be valid before accessing RF_CMC
	 */
	while ((RFMC->RF2P4GHZ_STAT & RFMC_RF2P4GHZ_STAT_FRO_CLK_VLD_STAT_MASK) == 0U) {
		;
	}

	/* Force low power entry request to the radio domain */
	RF_CMC1->RADIO_LP |= RF_CMC1_RADIO_LP_CK(0x2);
	RFMC->RF2P4GHZ_CTRL |= RFMC_RF2P4GHZ_CTRL_LP_ENTER(0x1U);
#endif
#if !defined(CONFIG_SOC_MCXW716C)
	/* Allow wakeup from the debugger */
	RFMC->RF2P4GHZ_CFG |= RFMC_RF2P4GHZ_CFG_FORCE_DBG_PWRUP_ACK_MASK;
	CMC_EnableDebugOperation(MCXW7_CMC_ADDR, true);
#endif
	return 0;
}

/* soc_nbu_init may call k_busy_wait, which requires the system timer to be initialized
 * (available by early PRE_KERNEL_2).
 */
SYS_INIT(soc_nbu_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
