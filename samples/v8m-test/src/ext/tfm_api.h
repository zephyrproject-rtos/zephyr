/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_API_H__
#define __TFM_API_H__

#ifdef __cplusplus
extern "C" {
#endif

/* FixMe: sort out DEBUG compile option and limit return value options
 * on external interfaces */
/* Note:
 * TFM will only return values recognized and parsed by TFM core.
 * Service return codes are not automatically passed on to REE.
 * Any non-zero return value is interpreted as an error that may trigger
 * TEE error handling flow.
 */
enum tfm_status_e
{
    TFM_SUCCESS = 0,
    TFM_SERVICE_PENDED,
    TFM_SERVICE_BUSY,
    TFM_ERROR_SERVICE_ALREADY_PENDED,
    TFM_ERROR_SECURE_DOMAIN_LOCKED,
    TFM_ERROR_INVALID_PARAMETER,
    TFM_ERROR_SERVICE_NON_REENTRANT,
    TFM_ERROR_NS_THREAD_MODE_CALL,
    TFM_ERROR_INVALID_EXC_MODE,
    TFM_SECURE_LOCK_FAILED,
    TFM_SECURE_UNLOCK_FAILED,
    TFM_ERROR_GENERIC = 0x1F,
    TFM_SERVICE_SPECIFIC_ERROR_MIN,
};

//==================== Secure function declarations ==========================//

/* Placeholder for secure function declarations defined by TF-M in the future */

//================ End Secure function declarations ==========================//

#ifdef __cplusplus
}
#endif

#endif /* __TFM_API_H__ */
