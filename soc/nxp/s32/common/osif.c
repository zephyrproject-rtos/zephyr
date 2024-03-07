/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <OsIf.h>
#include <OsIf_Cfg_TypesDef.h>

#if defined(CONFIG_SOC_SERIES_S32K1)
/* Aliases needed to build with different SoC-specific HAL versions */
#define CPXNUM                  CPxNUM
#define MSCM_CPXNUM_CPN_MASK    MSCM_CPxNUM_CPN_MASK
#endif

/* Required by OsIf timer initialization but not used with Zephyr, so no values configured */
static const OsIf_ConfigType osif_config;
const OsIf_ConfigType *const OsIf_apxPredefinedConfig[OSIF_MAX_COREIDX_SUPPORTED] = {
	&osif_config
};

/*
 * OsIf call to get the processor number of the core making the access.
 */
uint8_t Sys_GetCoreID(void)
{
	return ((uint8_t)(IP_MSCM->CPXNUM & MSCM_CPXNUM_CPN_MASK));
}
