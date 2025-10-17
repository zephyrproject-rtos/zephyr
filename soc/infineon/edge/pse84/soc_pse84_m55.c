/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon PSOC EDGE84 soc.
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include "soc.h"
#include <cy_sysint.h>
#include <system_edge.h>
#include <ifx_cycfg_init.h>
#include "cy_pdl.h"

#define CY_IPC_MAX_ENDPOINTS (8UL)

/**
 * Config_noncacheable_region() copied from
 * hal_infineon\...\
 * COMPONENT_CM55\COMPONENT_NON_SECURE_DEVICE\ns_start_pse84.c
 */
#define MPU_SRAM1_SHARED_MEM_REG_ID   0x6
#define MPU_SRAM1_SHARED_MEM_ATTR_IDX 0x6

#define CM55_STARTUP_WAIT_MS 25u

void config_noncacheable_region(void)
{
	ARM_MPU_Disable();

	/* Program MAIR0 and MAIR1 */
	ARM_MPU_SetMemAttr(MPU_SRAM1_SHARED_MEM_ATTR_IDX,
			   ARM_MPU_ATTR(ARM_MPU_ATTR_NON_CACHEABLE, ARM_MPU_ATTR_NON_CACHEABLE));

	ARM_MPU_SetRegion(MPU_SRAM1_SHARED_MEM_REG_ID,
			  ARM_MPU_RBAR(SRAM1_NS_SAHB_SHARED_START, ARM_MPU_SH_INNER, 0UL, 1UL, 1UL),
			  ARM_MPU_RLAR((SRAM1_NS_SAHB_SHARED_START + SRAM1_SHARED_SIZE - 1UL),
				       MPU_SRAM1_SHARED_MEM_ATTR_IDX));

	ARM_MPU_Enable(4);
}

/*
 * This function will allow execute from sram region.  This is needed only for
 * this sample because by default all soc will disable the execute from SRAM.
 * An application that requires that the code be executed from SRAM will have
 * to configure the region appropriately in arm_mpu_regions.c.
 */
#ifdef CONFIG_ARM_MPU
void disable_mpu_rasr_xn(void)
{
	uint32_t index;

	/*
	 * Kept the max index as 8(irrespective of soc) because the sram would
	 * most likely be set at index 2.
	 */
	for (index = 0U; index < 8; index++) {
		MPU->RNR = index;
#if defined(CONFIG_ARMV8_M_BASELINE) || defined(CONFIG_ARMV8_M_MAINLINE)
		if (MPU->RBAR & MPU_RBAR_XN_Msk) {
			MPU->RBAR ^= MPU_RBAR_XN_Msk;
		}
#else
		if (MPU->RASR & MPU_RASR_XN_Msk) {
			MPU->RASR ^= MPU_RASR_XN_Msk;
		}
#endif /* CONFIG_ARMV8_M_BASELINE || CONFIG_ARMV8_M_MAINLINE */
	}
}
#endif /* CONFIG_ARM_MPU */

void soc_early_init_hook(void)
{
	/* Config non-cacheable region */
	config_noncacheable_region();

#ifdef CONFIG_ARM_MPU
	disable_mpu_rasr_xn();
#endif /* CONFIG_ARM_MPU */

	/* Enable Loop and branch info cache */
	__DMB();
	__ISB();
	SCB_EnableICache();
	SCB_EnableDCache();

	/* Initializes the system */
	ifx_cycfg_init();

	static cy_stc_ipc_pipe_ep_t systemIpcPipeEpArray[CY_IPC_MAX_ENDPOINTS];

	Cy_IPC_Pipe_Config(systemIpcPipeEpArray);

	/* This time is needed for m55 core to wait for the m33 to finish
	 * configuring peripherals.
	 */
	Cy_SysLib_Delay(CM55_STARTUP_WAIT_MS);
}

cy_israddress Cy_SysInt_SetVector(IRQn_Type IRQn, cy_israddress userIsr)
{
	return 0;
}
