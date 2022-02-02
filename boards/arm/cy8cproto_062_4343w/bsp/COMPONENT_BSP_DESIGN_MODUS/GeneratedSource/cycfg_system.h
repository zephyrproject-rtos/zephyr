/*******************************************************************************
 * File Name: cycfg_system.h
 *
 * Description:
 * System configuration
 * This file was automatically generated and should not be modified.
 * Tools Package 2.4.0.5721
 * mtb-pdl-cat1 3.0.0.10651
 * personalities 5.0.0.0
 * udd 3.0.0.1377
 *
 ********************************************************************************
 * Copyright 2021 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ********************************************************************************/

#if !defined(CYCFG_SYSTEM_H)
#define CYCFG_SYSTEM_H

#include "cycfg_notices.h"
#include "cy_sysclk.h"
#include "cy_pra.h"
#include "cy_pra_cfg.h"
#if defined (CY_USING_HAL)
    #include "cyhal_hwmgr.h"
#endif // defined (CY_USING_HAL)
#include "cy_gpio.h"
#include "cy_syspm.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define cpuss_0_dap_0_ENABLED 1U
#define srss_0_clock_0_ENABLED 1U
#define srss_0_clock_0_bakclk_0_ENABLED 1U
#define srss_0_clock_0_fastclk_0_ENABLED 1U
#define srss_0_clock_0_fll_0_ENABLED 1U
#define srss_0_clock_0_hfclk_0_ENABLED 1U
#define CY_CFG_SYSCLK_CLKHF0 0UL
#define CY_CFG_SYSCLK_CLKHF0_CLKPATH_NUM 0UL
#define srss_0_clock_0_ilo_0_ENABLED 1U
#define srss_0_clock_0_imo_0_ENABLED 1U
#define srss_0_clock_0_lfclk_0_ENABLED 1U
#define CY_CFG_SYSCLK_CLKLF_FREQ_HZ 32768
#define CY_CFG_SYSCLK_CLKLF_SOURCE CY_SYSCLK_CLKLF_IN_WCO
#define srss_0_clock_0_pathmux_0_ENABLED 1U
#define srss_0_clock_0_pathmux_1_ENABLED 1U
#define srss_0_clock_0_pathmux_2_ENABLED 1U
#define srss_0_clock_0_pathmux_3_ENABLED 1U
#define srss_0_clock_0_pathmux_4_ENABLED 1U
#define srss_0_clock_0_pathmux_5_ENABLED 1U
#define srss_0_clock_0_periclk_0_ENABLED 1U
#define srss_0_clock_0_pll_0_ENABLED 1U
#define srss_0_clock_0_slowclk_0_ENABLED 1U
#define srss_0_clock_0_wco_0_ENABLED 1U
#define srss_0_power_0_ENABLED 1U
#define CY_CFG_PWR_MODE_LP 0x01UL
#define CY_CFG_PWR_MODE_ULP 0x02UL
#define CY_CFG_PWR_MODE_ACTIVE 0x04UL
#define CY_CFG_PWR_MODE_SLEEP 0x08UL
#define CY_CFG_PWR_MODE_DEEPSLEEP 0x10UL
#define CY_CFG_PWR_SYS_IDLE_MODE CY_CFG_PWR_MODE_DEEPSLEEP
#define CY_CFG_PWR_SYS_ACTIVE_MODE CY_CFG_PWR_MODE_LP
#define CY_CFG_PWR_DEEPSLEEP_LATENCY 0UL
#define CY_CFG_PWR_USING_LDO 1
#define CY_CFG_PWR_VDDA_MV 3300
#define CY_CFG_PWR_VDDD_MV 3300
#define CY_CFG_PWR_VBACKUP_MV 3300
#define CY_CFG_PWR_VDD_NS_MV 3300
#define CY_CFG_PWR_VDDIO0_MV 3300
#define CY_CFG_PWR_VDDIO1_MV 3300

#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t srss_0_clock_0_pathmux_0_obj;
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t srss_0_clock_0_pathmux_1_obj;
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t srss_0_clock_0_pathmux_2_obj;
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t srss_0_clock_0_pathmux_3_obj;
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t srss_0_clock_0_pathmux_4_obj;
#endif // defined (CY_USING_HAL)
#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t srss_0_clock_0_pathmux_5_obj;
#endif // defined (CY_USING_HAL)

void init_cycfg_system(void);

#if defined(__cplusplus)
}
#endif


#endif /* CYCFG_SYSTEM_H */
