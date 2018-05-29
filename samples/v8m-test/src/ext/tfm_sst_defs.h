/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_SST_DEFS_H__
#define __TFM_SST_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <limits.h>
#include "tfm_api.h"

/* FIXME: the secure APP ID should not be share with the non-secure code
 *        as it is revealing information about secure code implementation.
 */
#define S_APP_ID 0xF0000000

/* FIXME:
 * Very descriptive error codes can leak implementation
 * information to caller and may allow exploitation
 * of implementation weaknesses by malicious actors.
 * Potential approaches-
 * a. Just return generic error for any kind of failure
 *
 * OR
 *
 * b. Non-secure callers get the generic failure, the
 *    secure side callers get a bit more detailed error
 *    codes.
 */

/* The return value is shared with the TFM service status value. The SST return
 * codes shouldn't overlap with predefined TFM status values.
 */
#define TFM_SST_ERR_OFFSET (TFM_SERVICE_SPECIFIC_ERROR_MIN)

enum tfm_sst_err_t {
    TFM_SST_ERR_SUCCESS = 0,
    TFM_SST_ERR_ASSET_NOT_PREPARED = TFM_SST_ERR_OFFSET,
    TFM_SST_ERR_ASSET_NOT_FOUND,
    TFM_SST_ERR_PARAM_ERROR,
    TFM_SST_ERR_INVALID_HANDLE,
    TFM_SST_ERR_STORAGE_SYSTEM_FULL,
    TFM_SST_ERR_SYSTEM_ERROR,
    /* Following entry is only to ensure the error code of int size */
    TFM_SST_ERR_FORCE_INT_SIZE = INT_MAX
};

struct tfm_sst_attribs_t {
    uint32_t size_current; /*!< The current size of the asset */
    uint32_t size_max;     /*!< The maximum size of the asset in bytes */
};

/* Structure to store data information to read/write from/to asset */
struct tfm_sst_buf_t {
    uint8_t* data;   /*!< Address of input/output data */
    uint32_t size;   /*!< Size of input/output data */
    uint32_t offset; /*!< Offset within asset */
};

#ifdef __cplusplus
}
#endif

#endif /* __TFM_SST_DEFS_H__ */
