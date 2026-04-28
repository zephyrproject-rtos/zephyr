/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Infineon PSOC EDGE84 soc.
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>


#define PSE84_CPU_FREQ_HZ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

#if defined(CONFIG_BUILD_WITH_TFM)
#include "mtb_srf_pool_init.h"
#include "cy_syslib.h"

mtb_srf_pool_t cy_pdl_srf_default_pool;
CY_SECTION_SHAREDMEM _MTB_SRF_DATA_ALIGN uint32_t cy_pdl_srf_default_pool_memory[
								(MTB_SRF_POOL_ENTRY_SIZE(
								MTB_SRF_MAX_ARG_IN_SIZE,
								MTB_SRF_MAX_ARG_OUT_SIZE)
								* MTB_SRF_POOL_SIZE) /
								sizeof(uint32_t)];
#endif /* defined(CONFIG_BUILD_WITH_TFM) */


void soc_early_init_hook(void)
{
	/* Initialize SystemCoreClock variable. */
	SystemCoreClockSetup(PSE84_CPU_FREQ_HZ, PSE84_CPU_FREQ_HZ);

#ifdef CONFIG_BUILD_WITH_TFM
	cy_rslt_t srf_init = mtb_srf_pool_init(&cy_pdl_srf_default_pool,
							&cy_pdl_srf_default_pool_memory[0],
							MTB_SRF_POOL_SIZE,
							MTB_SRF_MAX_ARG_IN_SIZE,
							MTB_SRF_MAX_ARG_OUT_SIZE);

	if (srf_init != CY_RSLT_SUCCESS) {
		/*If SRF pool initialization fails then panic as TF-M relies
		 * on this pool for handling secure requests
		 */
		k_panic();
	}
#endif /* CONFIG_BUILD_WITH_TFM */

}

void soc_late_init_hook(void)
{
#if defined(CONFIG_SOC_PSE84_M55_ENABLE)
	ifx_pse84_cm55_startup();
#endif
}
