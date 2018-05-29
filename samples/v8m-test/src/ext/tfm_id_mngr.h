/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_ID_MNGR_H__
#define __TFM_ID_MNGR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * \brief Gets SST current application ID
 *
 * \return Returns the SST current application ID
 */
uint32_t tfm_sst_get_cur_id(void);

#ifdef __cplusplus
}
#endif

#endif /* __TFM_ID_MNGR_H__ */
