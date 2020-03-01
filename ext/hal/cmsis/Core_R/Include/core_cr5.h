/**************************************************************************//**
 * @file     core_cr5.h
 * @brief    CMSIS Cortex-R5 Core Peripheral Access Layer Header File
 * @version  V1.0.0
 * @date     17. October 2019
 ******************************************************************************/
/*
 * Copyright (c) 2009-2018 ARM Limited. All rights reserved.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if   defined ( __ICCARM__ )
  #pragma system_include         /* treat file as system include file for MISRA check */
#elif defined (__clang__)
  #pragma clang system_header   /* treat file as system include file */
#endif

#ifndef __CORE_CR5_H_GENERIC
#define __CORE_CR5_H_GENERIC

#ifdef __cplusplus
 extern "C" {
#endif

#define __CORTEX_R              5U

/* Configuration of the Cortex-R5 Processor and Core Peripherals */
#ifndef __CR_REV
#define __CR_REV                0U
#endif

#ifndef __FPU_PRESENT
#define __FPU_PRESENT           0U
#endif

#define __GIC_PRESENT           0U
#define __TIM_PRESENT           0U
#define __MPU_PRESENT           1U

/* Include Cortex-R Common Peripheral Access Layer header */
#include "core_cr.h"

#ifdef __cplusplus
}
#endif

#endif /* __CORE_CR5_H_GENERIC */
