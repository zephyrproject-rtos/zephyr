/*
 * Copyright (c) 2017, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_SS_CORE_TEST_2_VENEERS_H__
#define __TFM_SS_CORE_TEST_2_VENEERS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <cmsis_compiler.h>

/**
 * \brief A minimal test service to be called from another service.
 *
 * \return Returns \ref TFM_SUCCESS.
 */
int32_t tfm_core_test_2_veneer_slave_service(void)
__attribute__((cmse_nonsecure_entry, noinline, section("SFN")));

/**
 * \brief Bitwise inverts the buffer received as input.
 *
 * \param[out] res_ptr  Result, 0 on success, -1 othervise.
 * \param[in] in_ptr    Buffer containing data to be inverted.
 * \param[out] out_ptr  Buffer to store the inverted data.
 * \param[in] len       Number of bytes to be inverted.
 *
 * \return Returns \ref TFM_SUCCESS on success, TFM_SERVICE_BUSY otherwise.
 */
__attribute__((cmse_nonsecure_entry, noinline, section("SFN")))
int32_t tfm_core_test_2_sfn_invert(
        int32_t *res_ptr, uint32_t *in_ptr, uint32_t *out_ptr, int32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __TFM_SS_CORE_TEST_2_VENEERS_H__ */
