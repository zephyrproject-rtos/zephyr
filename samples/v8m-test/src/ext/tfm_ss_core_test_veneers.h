/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_SS_CORE_TEST_VENEERS_H__
#define __TFM_SS_CORE_TEST_VENEERS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <cmsis_compiler.h>

/**
 * \brief Tests whether the initialisation of the service was successful.
 *
 * \return Returns \ref CORE_TEST_ERRNO_SUCCESS on success, and
 *                 \ref CORE_TEST_ERRNO_SERVICE_NOT_INITED on failure.
 */
int32_t tfm_core_test_sfn_init_success(void)
__attribute__((cmse_nonsecure_entry, noinline, section("SFN")));

/**
 * \brief Tests what happens when a service calls itself directly.
 *
 * \param[in] depth  The current depth of the call (0 when first called).
 *
 * \return Returns \ref CORE_TEST_ERRNO_SUCCESS on succes, error othervise.
 */
int32_t tfm_core_test_sfn_direct_recursion(int32_t depth)
__attribute__((cmse_nonsecure_entry, noinline, section("SFN")));

/**
 * \brief Entry point for multiple test cases to be executed on the secure side.
 *
 * \param[in] a  The id of the testcase.
 * \param[in] b  First parameter for testcase.
 * \param[in] c  Second parameter for testcase.
 * \param[in] d  Third parameter for testcase.
 *
 * \return Can return various error codes.
 */
int32_t tfm_core_test_sfn(int32_t a, int32_t b, int32_t c, int32_t d)
__attribute__((cmse_nonsecure_entry, noinline, section("SFN")));

#ifdef __cplusplus
}
#endif

#endif /* __TFM_SS_CORE_TEST_VENEERS_H__ */
