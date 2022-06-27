/*
 * Copyright (c) 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_src_rev2

#include <soc.h>
#include <zephyr/drivers/hwinfo.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <fsl_soc_src.h>

#ifdef CONFIG_CPU_CORTEX_M7
#define MCUX_RESET_PIN_FLAG kSRC_M7CoreIppUserResetFlag
#define MCUX_RESET_SOFTWARE_FLAG kSRC_M7CoreM7LockUpResetFlag
#define MCUX_RESET_POR_FLAG kSRC_M7CoreIppResetFlag
#define MCUX_RESET_WATCHDOG_FLAG (kSRC_M7CoreWdogResetFlag |			\
				kSRC_M7CoreWdog3ResetFlag |			\
				kSRC_M7CoreWdog4ResetFlag)
#define MCUX_RESET_DEBUG_FLAG kSRC_M7CoreJtagResetFlag
#define MCUX_RESET_SECURITY_FLAG kSRC_M7CoreCSUResetFlag
#define MCUX_RESET_TEMPERATURE_FLAG kSRC_M7CoreTempsenseResetFlag
#define MCUX_RESET_USER_FLAG kSRC_M7CoreM7RequestResetFlag
#elif defined(CONFIG_CPU_CORTEX_M4)
#define MCUX_RESET_PIN_FLAG kSRC_M4CoreIppUserResetFlag
#define MCUX_RESET_SOFTWARE_FLAG kSRC_M4CoreM7LockUpResetFlag
#define MCUX_RESET_POR_FLAG kSRC_M4CoreIppResetFlag
#define MCUX_RESET_WATCHDOG_FLAG (kSRC_M4CoreWdogResetFlag |			\
				kSRC_M4CoreWdog3ResetFlag |			\
				kSRC_M4CoreWdog4ResetFlag)
#define MCUX_RESET_DEBUG_FLAG kSRC_M4CoreJtagResetFlag
#define MCUX_RESET_SECURITY_FLAG kSRC_M4CoreCSUResetFlag
#define MCUX_RESET_TEMPERATURE_FLAG kSRC_M4CoreTempsenseResetFlag
#define MCUX_RESET_USER_FLAG kSRC_M4CoreM7RequestResetFlag
#else
/* The SOCs currently supported have an M7 or M4 core */
#error "MCUX SRC driver not supported for this CPU!"
#endif

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "No nxp,imx-src compatible device found");

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	uint32_t flags = 0;
	uint32_t reason = SRC_GetResetStatusFlags((SRC_Type *)DT_INST_REG_ADDR(0));

	if (reason & (MCUX_RESET_PIN_FLAG)) {
		flags |= RESET_PIN;
	}
	if (reason & (MCUX_RESET_SOFTWARE_FLAG)) {
		flags |= RESET_SOFTWARE;
	}
	if (reason & (MCUX_RESET_POR_FLAG)) {
		flags |= RESET_POR;
	}
	if (reason & (MCUX_RESET_WATCHDOG_FLAG)) {
		flags |= RESET_WATCHDOG;
	}
	if (reason & (MCUX_RESET_DEBUG_FLAG)) {
		flags |= RESET_DEBUG;
	}
	if (reason & (MCUX_RESET_SECURITY_FLAG)) {
		flags |= RESET_SECURITY;
	}
	if (reason & (MCUX_RESET_TEMPERATURE_FLAG)) {
		flags |= RESET_TEMPERATURE;
	}
	if (reason & (MCUX_RESET_USER_FLAG)) {
		flags |= RESET_USER;
	}

	*cause = flags;

	return 0;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	uint32_t reason = SRC_GetResetStatusFlags((SRC_Type *)DT_INST_REG_ADDR(0));

	SRC_ClearGlobalSystemResetStatus((SRC_Type *)DT_INST_REG_ADDR(0), reason);

	return 0;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_WATCHDOG
		      | RESET_DEBUG
		      | RESET_TEMPERATURE
		      | RESET_PIN
		      | RESET_SOFTWARE
		      | RESET_POR
		      | RESET_SECURITY
		      | RESET_USER
		      );

	return 0;
}
