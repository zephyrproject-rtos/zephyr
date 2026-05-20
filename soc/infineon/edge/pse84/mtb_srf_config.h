/*******************************************************************************
 * NOTE: This file is extracted from the publicly released BSP and can be found at
 * https://github.com/Infineon/TARGET_KIT_PSE84_EVAL_EPC2/blob/master/mtb_srf_config.h
 ******************************************************************************/
/*******************************************************************************
 * \file mtb_srf_config.h
 *
 * \brief
 * Configuration for the Secure Request Framework Library
 *
 *******************************************************************************
 * \copyright
 * Copyright 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.
 *
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
 *******************************************************************************/

/**
 * The Secure Request Framework (SRF) requires modules to be defined with unique values.
 * The values used do not matter, only that they are unique.  Below are two sample
 * modules - one for the Peripheral Driver Library (PDL) and one for the user.
 *
 * This file is owned by the application.  The expectation is for the user to add modules
 * as needed.  Should the user wish to use a middleware library that depends on the SRF,
 * that middleware will not add the module itself - the user must do so.
 */

#pragma once

#include "mtb_srf_iovec.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

/** Maximum input argument count in bytes, adjusting the default BSP-generated pool */
#define MTB_SRF_MAX_ARG_IN_SIZE                 (5U * sizeof(uint32_t))

/** Maximum output argument count in bytes, adjusting the default BSP-generated pool */
#define MTB_SRF_MAX_ARG_OUT_SIZE                (2U * sizeof(uint32_t))

/** Size in bytes of the secure scratch buffer into which secure request inputs are copied
 * before processing.
 *
 * The size must be a multiple of 4 bytes, and be large enough to hold:
 * * An `mtb_srf_input_ns_t` struct
 * * The maximum number of scalar input arguments (MTB_SRF_MAX_ARG_IN_SIZE)
 * * The largest input argument passed by pointer, excluding buffers whose contents
 *   are simply sent over a communications interface without requiring validation.
 * * An `mtb_srf_output_ns_t` struct
 * * The maximum number of scalar output arguments (MTB_SRF_MAX_ARG_OUT_SIZE)
 */
#ifndef MTB_SRF_SECURE_ARG_BUFFER_LEN
#define MTB_SRF_SECURE_ARG_BUFFER_LEN \
	(MTB_SRF_MAX_ARG_IN_SIZE + sizeof(mtb_srf_input_ns_t) + \
	MTB_SRF_MAX_ARG_OUT_SIZE + sizeof(mtb_srf_output_ns_t) + \
	128u)
#endif

/** Number of secure requests objects allocated, adjusting the default BSP-generated pool */
#if defined(CY_RTOS_AWARE) || defined(COMPONENT_RTOS_AWARE)
#define MTB_SRF_POOL_SIZE                       (3U)
#else
#define MTB_SRF_POOL_SIZE                       (1U)
#endif /* defined(CY_RTOS_AWARE) || defined(COMPONENT_RTOS_AWARE) */

/** IPC integration specific defines, adjusting the default BSP-generated setup */
#if defined(COMPONENT_MW_MTB_IPC)
#if defined(CY_SYSTEM_CPU_M55)
/** Define Macro to identify that this core uses IPC for SRF */
#define MTB_SRF_SUBMIT_USE_IPC
#endif
/** Default timeout for sending a request that requires IPC */
#define MTB_SRF_IPC_SERVICE_TIMEOUT_US          (MTB_SRF_NEVER_TIMEOUT)
#endif /* if defined(COMPONENT_MW_MTB_IPC) */

/** Number of modules */
#define MTB_SRF_MODULE_COUNT                    (2U)

/** Module for PDL operations */
#define MTB_SRF_MODULE_PDL                      (0U)
/** Module for User operations */
#define MTB_SRF_MODULE_USER                     (1U)

#ifdef __cplusplus
}
#endif /* defined(__cplusplus) */
