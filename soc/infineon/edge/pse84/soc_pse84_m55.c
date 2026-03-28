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

#define CM55_STARTUP_WAIT_MS 50u

void soc_early_init_hook(void)
{

	/* Enable Loop and branch info cache */
	__DMB();
	__ISB();
	SCB_EnableICache();
	SCB_EnableDCache();

	/* Initializes the system */
	ifx_cycfg_init();

	/* Initialize SystemCoreClock variable. */
	SystemCoreClockUpdate();

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
