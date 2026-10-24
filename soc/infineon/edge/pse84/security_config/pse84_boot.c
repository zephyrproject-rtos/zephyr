/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pse84_boot.h"

#if defined(CONFIG_SOC_PSE84_M55_RAM_LOAD)
#include <string.h>

/*
 * Destination for the copy below: the "zephyr,boot-ram-load-region" chosen
 * node, set by a board/application devicetree overlay rather than
 * hardcoded here. m33 and m55 build as two independent Zephyr images with
 * two independent devicetrees, so whoever owns both board files must keep
 * this chosen node equal to m55's own zephyr,flash region (ROM half only --
 * m55's zephyr,sram is a separate region m33 never touches, populated by
 * m55's own z_data_copy()/arch_bss_zero() at its own boot). Keeping ROM
 * and RAM as two separate regions, rather than one combined region, is
 * what lets the MPU mark m55's code execute+read-only and its data
 * write+execute-never -- a W^X boundary one combined region would lose.
 */
#define CM55_RAM_LOAD_DST DT_CHOSEN(zephyr_boot_ram_load_region)
#endif /* CONFIG_SOC_PSE84_M55_RAM_LOAD */

#if defined(CONFIG_SOC_PSE84_M55_ENABLE)
void ifx_pse84_cm55_startup(void)
{
	/* Setup System Control Block */
	SysCtrlBlk_Setup();
	/* Setup NS NVIC interrupts */
	NVIC_NS_Setup();

#if defined(__FPU_USED) && (__FPU_USED == 1U) && defined(TZ_FPU_NS_USAGE) && (TZ_FPU_NS_USAGE == 1U)
	/* FPU initialization */
	initFPU();
#endif

	/* Enable global interrupts */
	__enable_irq();

	/* Enables PD1 power domain */
	Cy_System_EnablePD1();

	/* Enables APP_MMIO_TCM memory for CM55 core */
	Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_CM55_TCM_512K_PERI_NR, CY_MMIO_CM55_TCM_512K_GROUP_NR,
				     CY_MMIO_CM55_TCM_512K_SLAVE_NR,
				     CY_MMIO_CM55_TCM_512K_CLK_HF_NR);

	Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_SMIF0_PERI_NR, CY_MMIO_SMIF0_GROUP_NR,
				     CY_MMIO_SMIF0_SLAVE_NR, CY_MMIO_SMIF0_CLK_HF_NR);

	/* Configure Memory Protection Controller for Non-Secure */
	cy_mpc_init();

	/* Reduce deepsleep wakeup time in hardware */
	cy_pd_pdcm_clear_dependency(CY_PD_PDCM_APPCPUSS, CY_PD_PDCM_SYSCPU);

	/* Clear SYSCPU and APPCPU power domain dependency set by boot code */
	cy_pd_pdcm_clear_dependency(CY_PD_PDCM_APPCPU, CY_PD_PDCM_SYSCPU);

#if defined(CONFIG_SOC_PSE84_M55_RAM_LOAD)
	/*
	 * SoCMEM is a separate power/clock domain from PD1 (system_edge.h's
	 * own comment above Cy_SysEnableSOCMEM()); without this the memcpy
	 * below silently writes into a block that isn't powered yet.
	 */
	Cy_SysEnableSOCMEM(true);

	/*
	 * m55's image is stored in m55_xip (flash0_sahb) but linked to execute
	 * from CM55_RAM_LOAD_DST instead. Copy it there now, while this core
	 * is still executing from flash0_sahb's own XIP window and can read
	 * m55_xip's bytes with a plain memcpy. Once m55 is launched from this
	 * copy it never executes from external flash again, which is what
	 * lets soc_pse84_m55.c safely leave SMIF0's XIP mode for good.
	 */
	uint32_t cm55_start_address = DT_REG_ADDR(CM55_RAM_LOAD_DST);

	memcpy((void *)cm55_start_address, (const void *)DT_REG_ADDR(DT_NODELABEL(m55_xip)),
	       DT_REG_SIZE(CM55_RAM_LOAD_DST));

	/* Write-buffer barrier so the copy above is guaranteed complete before
	 * Cy_SysEnableCM55() below releases m55 to read it.
	 */
	__DSB();
#else
	uint32_t cm55_start_address = DT_REG_ADDR(DT_NODELABEL(m55_xip));
#endif

	/* Enable CM55 */
	Cy_SysEnableCM55(MXCM55, cm55_start_address, CM55_BOOT_WAIT_TIME_USEC);

	/* System Domain Idle Power Mode Configuration */
	Cy_SysPm_SetDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);

	/* SoCMEM Idle Power Mode Configuration */
	Cy_SysPm_SetSOCMEMDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);

	/* Configure Peripheral Protection Controller for Non-Secure */
	cy_ppc0_init();
	cy_ppc1_init();

#ifdef CONFIG_CORTEX_M_SYSTICK
	sys_clock_disable();
#endif

#if defined(CONFIG_SOC_PSE84_M55_RAM_LOAD)
	/*
	 * Actually enter deep sleep rather than busy-looping. CM33 and CM55 share
	 * the same physical SMIF0/external-flash chip (via different bus aliases),
	 * so a busy loop here keeps this core fetching its own next instruction
	 * from that chip indefinitely -- a confound when testing CM55's flash
	 * writes for XIP-vs-write hazards. Deep sleep stops the core clock
	 * entirely instead. Only safe to do once CM55 no longer needs this core
	 * for anything else, which is specifically the case this Kconfig option
	 * describes; leave the regular case's plain busy loop alone otherwise.
	 */
	for (;;) {
		(void)Cy_SysPm_CpuEnterDeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
	}
#else
	for (;;) {
	}
#endif
}
#endif
