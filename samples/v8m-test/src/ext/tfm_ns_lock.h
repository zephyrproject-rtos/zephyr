/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef __TFM_NS_LOCK_H__
#define __TFM_NS_LOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "tfm_ns_svc.h"

/**
 * \brief NS world, NS lock based dispatcher
 *
 * \details To be called from the SVC wrapper API interface
 */
uint32_t tfm_ns_lock_svc_dispatch(enum tfm_svc_num svc_num,
                                  uint32_t arg0,
                                  uint32_t arg1,
                                  uint32_t arg2,
                                  uint32_t arg3);

/**
 * \brief NS world, Init NS lock
 *
 * \details Needs to be called during non-secure app init
 *          to initialize the TFM NS lock object
 */
uint32_t tfm_ns_lock_init();

#ifdef __cplusplus
}
#endif

#endif /* __TFM_NS_LOCK_H__ */
