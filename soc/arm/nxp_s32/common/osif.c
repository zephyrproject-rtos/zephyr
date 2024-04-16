/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <OsIf.h>
#include <OsIf_Cfg_TypesDef.h>

#if defined(CONFIG_SOC_S32Z27_R52)
#include <S32Z2_MSCM.h>
#elif defined(CONFIG_SOC_S32K344_M7)
#include <S32K344_MSCM.h>
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
