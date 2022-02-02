/*******************************************************************************
 * File Name: cycfg_clocks.h
 *
 * Description:
 * Clock configuration
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

#if !defined(CYCFG_CLOCKS_H)
#define CYCFG_CLOCKS_H

#include "cycfg_notices.h"
#include "cy_sysclk.h"
#if defined (CY_USING_HAL)
    #include "cyhal_hwmgr.h"
#endif // defined (CY_USING_HAL)

#if defined(__cplusplus)
extern "C" {
#endif

#define CYBSP_CSD_CLK_DIV_ENABLED 1U
#define CYBSP_CS_CLK_DIV_ENABLED CYBSP_CSD_CLK_DIV_ENABLED
#define CYBSP_CSD_CLK_DIV_HW CY_SYSCLK_DIV_8_BIT
#define CYBSP_CS_CLK_DIV_HW CYBSP_CSD_CLK_DIV_HW
#define CYBSP_CSD_CLK_DIV_NUM 0U
#define CYBSP_CS_CLK_DIV_NUM CYBSP_CSD_CLK_DIV_NUM

#if defined (CY_USING_HAL)
extern const cyhal_resource_inst_t CYBSP_CSD_CLK_DIV_obj;
    #define CYBSP_CS_CLK_DIV_obj CYBSP_CSD_CLK_DIV_obj
#endif // defined (CY_USING_HAL)

void init_cycfg_clocks(void);

#if defined(__cplusplus)
}
#endif


#endif /* CYCFG_CLOCKS_H */
