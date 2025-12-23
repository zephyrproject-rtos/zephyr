/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(pse84_s_protection_h)
#define pse84_s_protection_h

#include <infineon_kconfig.h>
#include "cy_mpc.h"
#include "pse84_s_system.h"
#include "pse84_s_mpc.h"
#include "cy_ppc.h"

#include "pse84_s_sau.h"

#define vres_0_protection_0_ENABLED       1U
#define M33S_ENABLED                      1U
#define M33_ENABLED                       1U
#define M55_ENABLED                       1U
#define M33NSC_ENABLED                    1U
#define M33_M55_ENABLED                   1U
#define reserved_ENABLED                  1U
#define vres_0_protection_0_mpc_0_ENABLED 1U
#define M33S_UNIFIED_MPC_DOMAIN_IDX       0U
#define M33_UNIFIED_MPC_DOMAIN_IDX        1U
#define M55_UNIFIED_MPC_DOMAIN_IDX        2U
#define M33NSC_UNIFIED_MPC_DOMAIN_IDX     3U
#define M33_M55_UNIFIED_MPC_DOMAIN_IDX    4U
#define PPC_PC_MASK_ALL_ACCESS            0xFFU

extern const cy_stc_mpc_rot_cfg_t m33s_mpc_cfg[];
extern const cy_stc_mpc_rot_cfg_t m33_mpc_cfg[];
extern const cy_stc_mpc_rot_cfg_t m55_mpc_cfg[];
extern const cy_stc_mpc_rot_cfg_t m33nsc_mpc_cfg[];
extern const cy_stc_mpc_rot_cfg_t m33_m55_mpc_cfg[];
extern const cy_stc_mpc_regions_t m33s_mpc_regions[];
extern const cy_stc_mpc_regions_t m33_mpc_regions[];
extern const cy_stc_mpc_regions_t m55_mpc_regions[];
extern const cy_stc_mpc_regions_t m33nsc_mpc_regions[];
extern const cy_stc_mpc_regions_t m33_m55_mpc_regions[];
extern const cy_stc_mpc_resp_cfg_t cy_response_mpcs[];
extern const size_t cy_response_mpcs_count;
extern const cy_stc_mpc_unified_t unified_mpc_domains[];
extern const size_t unified_mpc_domains_count;

extern const cy_stc_ppc_attribute_t cycfg_unused_ppc_cfg;

cy_rslt_t cy_ppc0_init(void);
cy_rslt_t cy_ppc1_init(void);

#endif /* pse84_s_protection_h */
