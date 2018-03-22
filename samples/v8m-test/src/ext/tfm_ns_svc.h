/*
 * Copyright (c) 2017-2018, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include <cmsis_compiler.h>

#ifndef __TFM_NS_SVC_H__
#define __TFM_NS_SVC_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Macro to encode an svc instruction
 *
 */
#define SVC(code) __ASM("svc %0" : : "I" (code))

/**
 * \brief Numbers associated to each SVC available
 *
 * \details Start from 1 as 0 is reserved by RTX
 */
enum tfm_svc_num {
    SVC_INVALID = 0,

/* SVC API for SST */
    SVC_TFM_SST_GET_HANDLE,
    SVC_TFM_SST_CREATE,
    SVC_TFM_SST_GET_ATTRIBUTES,
    SVC_TFM_SST_READ,
    SVC_TFM_SST_WRITE,
    SVC_TFM_SST_DELETE,

#if defined(CORE_TEST_INTERACTIVE)
    SVC_SECURE_DECREMENT_NS_LOCK_1,
    SVC_SECURE_DECREMENT_NS_LOCK_2,
#endif /* CORE_TEST_INTERACTIVE */

#if defined(CORE_TEST_SERVICES)
    SVC_TFM_CORE_TEST,
    SVC_TFM_CORE_TEST_MULTIPLE_CALLS,
#endif /* CORE_TEST_SERVICES */

#if defined(SST_TEST_SERVICES)
    SVC_SST_TEST_SERVICE_SETUP,
    SVC_SST_TEST_SERVICE_DUMMY_ENCRYPT,
    SVC_SST_TEST_SERVICE_DUMMY_DECRYPT,
    SVC_SST_TEST_SERVICE_CLEAN,
#endif /* SST_TEST_SERVICES */

    /* add all the new entries above this line */
    SVC_TFM_MAX,
};

/* number of user SVC functions */
#define USER_SVC_COUNT (SVC_TFM_MAX - 1)

#ifdef __cplusplus
}
#endif

#endif /* __TFM_NS_SVC_H__ */
