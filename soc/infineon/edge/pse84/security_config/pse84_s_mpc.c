/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pse84_s_mpc.h"
#include "pse84_s_protection.h"
#include <string.h>

#if (SMIF_0_MPC_0_REGION_COUNT > 0U) || (SMIF_1_MPC_0_REGION_COUNT > 0U)
#include "cy_smif.h"

#define SMIF_DESELECT_DELAY (7U)
#define TIMEOUT_1_MS        (1000U)
#endif /* (SMIF_0_MPC_0_REGION_COUNT > 0U)|| (SMIF_1_MPC_0_REGION_COUNT > 0U) */

/* Value generated through device-configurator settings */
#define CY_MPC_PC_LAST (8)

static cy_rslt_t mpc_init(const cy_stc_mpc_regions_t *region, const cy_stc_mpc_rot_cfg_t *config,
			  uint8_t cfg_count)
{
	/* Will return an error if one occurs at all in the setup process */
	cy_rslt_t return_result = CY_RSLT_SUCCESS;
	/* Will track the current iteration for errors */
	cy_rslt_t current_result = CY_RSLT_SUCCESS;

	for (uint32_t cfg_idx = 0; cfg_idx < cfg_count; ++cfg_idx) {
		/* Call the configuration function for each region and config */
		current_result = Cy_Mpc_ConfigRotMpcStruct(region->base, region->offset,
							   region->size, &config[cfg_idx]);
		if (CY_RSLT_SUCCESS != current_result) {
			return_result = current_result;
		}
	}

	return return_result;
}

cy_rslt_t cy_mpc_init(void)
{
	/* Will return an error if one occurs at all in the setup process */
	cy_rslt_t return_result = CY_RSLT_SUCCESS;
	/* Will track the current iteration for errors */
	cy_rslt_t current_result = CY_RSLT_SUCCESS;

	/* When executing from RRAM, the SMIF must be initialized and enabled to
	 * hold onto any MPC configurations performed on the SMIF memory regions.
	 * We can disable them after performing the MPC configurations.
	 */
#if (SMIF_0_MPC_0_REGION_COUNT > 0U)
	bool is_smif0_uninit = false;

	if (!Cy_SMIF_IsEnabled(SMIF0_CORE)) {
		is_smif0_uninit = true;

		cy_stc_smif_context_t smif_core0_context;

		static const cy_stc_smif_config_t smif_0_core0_config = {
			.mode = (uint32_t)CY_SMIF_NORMAL,
			.deselectDelay = SMIF_DESELECT_DELAY,
			.blockEvent = (uint32_t)CY_SMIF_BUS_ERROR,
			.enable_internal_dll = false};

		/* Enable IP with default configuration */
		(void)Cy_SMIF_Init(SMIF0_CORE, &smif_0_core0_config, TIMEOUT_1_MS,
				   &smif_core0_context);
		Cy_SMIF_Enable(SMIF0_CORE, &smif_core0_context);
	}
#endif /* (SMIF_0_MPC_0_REGION_COUNT > 0U) */

#if (SOCMEM_0_MPC_0_REGION_COUNT > 0U)
	/* Enable SOCMEM */
	Cy_SysEnableSOCMEM(true);
#endif /* (SOCMEM_0_MPC_0_REGION_COUNT > 0U) */

	for (size_t memory_idx = 0; memory_idx < cy_response_mpcs_count; memory_idx++) {
		Cy_Mpc_SetViolationResponse(cy_response_mpcs[memory_idx].base,
					    cy_response_mpcs[memory_idx].response);
	}

	for (size_t domain_idx = 0; domain_idx < unified_mpc_domains_count; ++domain_idx) {
		const cy_stc_mpc_unified_t *domain = &unified_mpc_domains[domain_idx];

		for (uint32_t region_idx = 0; region_idx < domain->region_count; ++region_idx) {
			const cy_stc_mpc_regions_t *region = &domain->regions[region_idx];

			/* If using RRAM separate init is required. Refer to MTB secure app. */
			if (region->base != (MPC_Type *)RRAMC0_MPC0) {
				/* Check for an empty regions struct */
				if (region->base != NULL) {
					current_result = mpc_init(region, &(domain->cfg[0]),
								  domain->cfg_count);
				}
			}

			if (CY_RSLT_SUCCESS != current_result) {
				return_result = current_result;
			}
		}
	}

#if (SMIF_0_MPC_0_REGION_COUNT > 0U)
	if (is_smif0_uninit) {
		/* Disable IP as MPC configuration is complete*/
		Cy_SMIF_Disable(SMIF0_CORE);
	}
#endif /* (SMIF_0_MPC_0_REGION_COUNT > 0U) */

	return return_result;
}
