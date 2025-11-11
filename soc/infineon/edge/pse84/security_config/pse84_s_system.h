/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(pse84_s_system_h)
#define pse84_s_system_h

#include <infineon_kconfig.h>
#include "cy_syspm.h"
#include "system_edge.h"
#include "cy_sysclk.h"
#include "cy_syspm_pdcm.h"
#include "pse84_s_mpc.h"

#define CY_CFG_PWR_MODE_LP            0x01UL
#define CY_CFG_PWR_MODE_ULP           0x02UL
#define CY_CFG_PWR_MODE_HP            0x03UL
#define CY_CFG_PWR_MODE_ACTIVE        0x04UL
#define CY_CFG_PWR_MODE_SLEEP         0x08UL
#define CY_CFG_PWR_MODE_DEEPSLEEP     0x10UL
#define CY_CFG_PWR_MODE_DEEPSLEEP_RAM 0x11UL
#define CY_CFG_PWR_MODE_DEEPSLEEP_OFF 0x12UL
#define CY_CFG_PWR_SYS_IDLE_MODE      CY_CFG_PWR_MODE_DEEPSLEEP
#define CY_CFG_PWR_DEEPSLEEP_LATENCY  20UL
#define CY_CFG_PWR_SYS_ACTIVE_MODE    CY_CFG_PWR_MODE_HP
#define CY_CFG_PWR_SYS_ACTIVE_PROFILE CY_CFG_PWR_MODE_HP
#define CY_CFG_PWR_VDDA_MV            1800
#define CY_CFG_PWR_VDDD_MV            1800
#define CY_CFG_PWR_VDDIO0_MV          1800
#define CY_CFG_PWR_VDDIO1_MV          1800
#define CY_CFG_PWR_CBUCK_VOLT         CY_SYSPM_CORE_BUCK_VOLTAGE_0_90V
#define CY_CFG_PWR_CBUCK_MODE         CY_SYSPM_CORE_BUCK_MODE_HP
#define CY_CFG_PWR_SRAMLDO_VOLT       CY_SYSPM_SRAMLDO_VOLTAGE_0_80V
#define CY_CFG_PWR_PD1_DOMAIN         1
#define CY_CFG_PWR_PPU_MAIN           PPU_V1_MODE_FULL_RET
#define CY_CFG_PWR_PPU_PD1            PPU_V1_MODE_FULL_RET
#define CY_CFG_PWR_PPU_SRAM0          PPU_V1_MODE_MEM_RET
#define CY_CFG_PWR_PPU_SRAM1          PPU_V1_MODE_MEM_RET
#define CY_CFG_PWR_PPU_SYSCPU         PPU_V1_MODE_FULL_RET
#define CY_CFG_PWR_PPU_APPCPUSS       PPU_V1_MODE_FULL_RET
#define CY_CFG_PWR_PPU_APPCPU         PPU_V1_MODE_FULL_RET
#define CY_CFG_PWR_PPU_SOCMEM         PPU_V1_MODE_MEM_RET
#define CY_CFG_PWR_PPU_U55            PPU_V1_MODE_ON
#define MXRRAMC_0_MPC_0_REGION_COUNT  4U
#define MXSRAMC_0_MPC_0_REGION_COUNT  3U
#define MXSRAMC_1_MPC_0_REGION_COUNT  2U
#define SMIF_0_MPC_0_REGION_COUNT     6U
#define SMIF_1_MPC_0_REGION_COUNT     0U
#define SOCMEM_0_MPC_0_REGION_COUNT   2U

extern const mtb_srf_protection_range_s_t mxrramc_0_mpc_0_srf_range[MXRRAMC_0_MPC_0_REGION_COUNT];
extern const mtb_srf_protection_range_s_t mxsramc_0_mpc_0_srf_range[MXSRAMC_0_MPC_0_REGION_COUNT];
extern const mtb_srf_protection_range_s_t mxsramc_1_mpc_0_srf_range[MXSRAMC_1_MPC_0_REGION_COUNT];
extern const mtb_srf_protection_range_s_t smif_0_mpc_0_srf_range[SMIF_0_MPC_0_REGION_COUNT];
extern const mtb_srf_protection_range_s_t socmem_0_mpc_0_srf_range[SOCMEM_0_MPC_0_REGION_COUNT];

void init_cycfg_power(void);
void init_cycfg_system(void);

#endif /* pse84_s_system_h */
