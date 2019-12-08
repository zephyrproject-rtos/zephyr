/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mat_cmplx_mult_q31.c
 * Description:  Floating-point matrix multiplication
 *
 * $Date:        18. March 2019
 * $Revision:    V1.6.0
 *
 * Target Processor: Cortex-M cores
 * -------------------------------------------------------------------- */
/*
 * Copyright (C) 2010-2019 ARM Limited or its affiliates. All rights reserved.
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

#include "arm_math.h"

/**
  @ingroup groupMatrix
 */

/**
  @addtogroup CmplxMatrixMult
  @{
 */

/**
  @brief         Q31 Complex matrix multiplication.
  @param[in]     pSrcA      points to first input complex matrix structure
  @param[in]     pSrcB      points to second input complex matrix structure
  @param[out]    pDst       points to output complex matrix structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS       : Operation successful
                   - \ref ARM_MATH_SIZE_MISMATCH : Matrix size check failed

  @par           Scaling and Overflow Behavior
                   The function is implemented using an internal 64-bit accumulator.
                   The accumulator has a 2.62 format and maintains full precision of the intermediate
                   multiplication results but provides only a single guard bit. There is no saturation
                   on intermediate additions. Thus, if the accumulator overflows it wraps around and
                   distorts the result. The input signals should be scaled down to avoid intermediate
                   overflows. The input is thus scaled down by log2(numColsA) bits
                   to avoid overflows, as a total of numColsA additions are performed internally.
                   The 2.62 accumulator is right shifted by 31 bits and saturated to 1.31 format to yield the final result.
 */
#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

#define MATRIX_DIM2 2
#define MATRIX_DIM3 3
#define MATRIX_DIM4 4

__STATIC_INLINE arm_status arm_mat_cmplx_mult_q31_2x2_mve(
    const arm_matrix_instance_q31 * pSrcA,
    const arm_matrix_instance_q31 * pSrcB,
    arm_matrix_instance_q31 * pDst)
{
    q31_t const     *pInB = pSrcB->pData;   /* input data matrix pointer B */
    q31_t const     *pInA = pSrcA->pData;   /* input data matrix pointer A */
    q31_t           *pOut = pDst->pData;   /* output data matrix pointer */
    uint32x4_t     vecColBOffs0;
    q31_t const     *pInA0 = pInA;
    q31_t const     *pInA1 = pInA0 + CMPLX_DIM * MATRIX_DIM2;
    q63_t            acc0, acc1, acc2, acc3;
    q31x4_t          vecB, vecA;

    static const uint32_t offsetB0[4] = {
        0, 1,
        MATRIX_DIM2 * CMPLX_DIM, MATRIX_DIM2 * CMPLX_DIM + 1
    };

    vecColBOffs0 = vldrwq_u32(offsetB0);

    pInB = (q31_t const *) pSrcB->pData;

    vecB = vldrwq_gather_shifted_offset(pInB, vecColBOffs0);
    vecA = vldrwq_s32(pInA0);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(pInA1);
    acc2 = vmlsldavq_s32(vecA, vecB);
    acc3 = vmlaldavxq_s32(vecA, vecB);

    pOut[0 * CMPLX_DIM * MATRIX_DIM2 + 0] = (q31_t) asrl(acc0, 31);
    pOut[0 * CMPLX_DIM * MATRIX_DIM2 + 1] = (q31_t) asrl(acc1, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM2 + 0] = (q31_t) asrl(acc2, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM2 + 1] = (q31_t) asrl(acc3, 31);
    /*
     * move to next B column
     */
    pInB = pInB + CMPLX_DIM;

    vecB = vldrwq_gather_shifted_offset(pInB, vecColBOffs0);
    vecA = vldrwq_s32(pInA0);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(pInA1);
    acc2 = vmlsldavq_s32(vecA, vecB);
    acc3 = vmlaldavxq_s32(vecA, vecB);

    pOut += CMPLX_DIM;

    pOut[0 * CMPLX_DIM * MATRIX_DIM2 + 0] = (q31_t) asrl(acc0, 31);
    pOut[0 * CMPLX_DIM * MATRIX_DIM2 + 1] = (q31_t) asrl(acc1, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM2 + 0] = (q31_t) asrl(acc2, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM2 + 1] = (q31_t) asrl(acc3, 31);
    /*
     * Return to application
     */
    return (ARM_MATH_SUCCESS);
}

__STATIC_INLINE arm_status arm_mat_cmplx_mult_q31_3x3_mve(
    const arm_matrix_instance_q31 * pSrcA,
    const arm_matrix_instance_q31 * pSrcB,
    arm_matrix_instance_q31 * pDst)
{
    q31_t const     *pInB = pSrcB->pData;   /* input data matrix pointer B */
    q31_t const     *pInA = pSrcA->pData;   /* input data matrix pointer A */
    q31_t           *pOut = pDst->pData;   /* output data matrix pointer */
    uint32x4_t     vecColBOffs0, vecColBOffs1;
    q31_t const     *pInA0 = pInA;
    q31_t const     *pInA1 = pInA0 + CMPLX_DIM * MATRIX_DIM3;
    q31_t const     *pInA2 = pInA1 + CMPLX_DIM * MATRIX_DIM3;
    q63_t            acc0, acc1, acc2, acc3;
    q31x4_t          vecB, vecB1, vecA;
    /*
     * enable predication to disable upper half complex vector element
     */
    mve_pred16_t p0 = vctp32q(CMPLX_DIM);

    static const uint32_t offsetB0[4] = {
        0, 1,
        MATRIX_DIM3 * CMPLX_DIM, MATRIX_DIM3 * CMPLX_DIM + 1
    };
    static const uint32_t offsetB1[4] = {
        2 * MATRIX_DIM3 * CMPLX_DIM, 2 * MATRIX_DIM3 * CMPLX_DIM + 1,
        INACTIVELANE, INACTIVELANE
    };

    vecColBOffs0 = vldrwq_u32(offsetB0);
    vecColBOffs1 = vldrwq_u32(offsetB1);

    pInB = (q31_t const *) pSrcB->pData;

    vecB = vldrwq_gather_shifted_offset(pInB, vecColBOffs0);
    vecB1 = vldrwq_gather_shifted_offset(pInB, vecColBOffs1);

    vecA = vldrwq_s32(pInA0);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(pInA1);
    acc2 = vmlsldavq_s32(vecA, vecB);
    acc3 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_z_s32(&pInA0[4], p0);
    acc0 = vmlsldavaq_s32(acc0, vecA, vecB1);
    acc1 = vmlaldavaxq_s32(acc1, vecA, vecB1);

    vecA = vldrwq_z_s32(&pInA1[4], p0);
    acc2 = vmlsldavaq_s32(acc2, vecA, vecB1);
    acc3 = vmlaldavaxq_s32(acc3, vecA, vecB1);

    pOut[0 * CMPLX_DIM * MATRIX_DIM3 + 0] = (q31_t) asrl(acc0, 31);
    pOut[0 * CMPLX_DIM * MATRIX_DIM3 + 1] = (q31_t) asrl(acc1, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM3 + 0] = (q31_t) asrl(acc2, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM3 + 1] = (q31_t) asrl(acc3, 31);

    vecA = vldrwq_s32(pInA2);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_z_s32(&pInA2[4], p0);
    acc0 = vmlsldavaq_s32(acc0, vecA, vecB1);
    acc1 = vmlaldavaxq_s32(acc1, vecA, vecB1);

    pOut[2 * CMPLX_DIM * MATRIX_DIM3 + 0] = (q31_t) asrl(acc0, 31);
    pOut[2 * CMPLX_DIM * MATRIX_DIM3 + 1] = (q31_t) asrl(acc1, 31);
    pOut += CMPLX_DIM;

    /*
     * move to next B column
     */
    pInB = pInB + CMPLX_DIM;

    vecB = vldrwq_gather_shifted_offset(pInB, vecColBOffs0);
    vecB1 = vldrwq_gather_shifted_offset(pInB, vecColBOffs1);

    vecA = vldrwq_s32(pInA0);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(pInA1);
    acc2 = vmlsldavq_s32(vecA, vecB);
    acc3 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_z_s32(&pInA0[4], p0);
    acc0 = vmlsldavaq_s32(acc0, vecA, vecB1);
    acc1 = vmlaldavaxq_s32(acc1, vecA, vecB1);

    vecA = vldrwq_z_s32(&pInA1[4], p0);
    acc2 = vmlsldavaq_s32(acc2, vecA, vecB1);
    acc3 = vmlaldavaxq_s32(acc3, vecA, vecB1);

    pOut[0 * CMPLX_DIM * MATRIX_DIM3 + 0] = (q31_t) asrl(acc0, 31);
    pOut[0 * CMPLX_DIM * MATRIX_DIM3 + 1] = (q31_t) asrl(acc1, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM3 + 0] = (q31_t) asrl(acc2, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM3 + 1] = (q31_t) asrl(acc3, 31);

    vecA = vldrwq_s32(pInA2);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_z_s32(&pInA2[4], p0);
    acc0 = vmlsldavaq_s32(acc0, vecA, vecB1);
    acc1 = vmlaldavaxq_s32(acc1, vecA, vecB1);

    pOut[2 * CMPLX_DIM * MATRIX_DIM3 + 0] = (q31_t) asrl(acc0, 31);
    pOut[2 * CMPLX_DIM * MATRIX_DIM3 + 1] = (q31_t) asrl(acc1, 31);
    pOut += CMPLX_DIM;

    /*
     * move to next B column
     */
    pInB = pInB + CMPLX_DIM;

    vecB = vldrwq_gather_shifted_offset(pInB, vecColBOffs0);
    vecB1 = vldrwq_gather_shifted_offset(pInB, vecColBOffs1);

    vecA = vldrwq_s32(pInA0);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(pInA1);
    acc2 = vmlsldavq_s32(vecA, vecB);
    acc3 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_z_s32(&pInA0[4], p0);
    acc0 = vmlsldavaq_s32(acc0, vecA, vecB1);
    acc1 = vmlaldavaxq_s32(acc1, vecA, vecB1);

    vecA = vldrwq_z_s32(&pInA1[4], p0);
    acc2 = vmlsldavaq_s32(acc2, vecA, vecB1);
    acc3 = vmlaldavaxq_s32(acc3, vecA, vecB1);

    pOut[0 * CMPLX_DIM * MATRIX_DIM3 + 0] = (q31_t) asrl(acc0, 31);
    pOut[0 * CMPLX_DIM * MATRIX_DIM3 + 1] = (q31_t) asrl(acc1, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM3 + 0] = (q31_t) asrl(acc2, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM3 + 1] = (q31_t) asrl(acc3, 31);

    vecA = vldrwq_s32(pInA2);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_z_s32(&pInA2[4], p0);
    acc0 = vmlsldavaq_s32(acc0, vecA, vecB1);
    acc1 = vmlaldavaxq_s32(acc1, vecA, vecB1);

    pOut[2 * CMPLX_DIM * MATRIX_DIM3 + 0] = (q31_t) asrl(acc0, 31);
    pOut[2 * CMPLX_DIM * MATRIX_DIM3 + 1] = (q31_t) asrl(acc1, 31);
    /*
     * Return to application
     */
    return (ARM_MATH_SUCCESS);
}

__STATIC_INLINE arm_status arm_mat_cmplx_mult_q31_4x4_mve(
    const arm_matrix_instance_q31 * pSrcA,
    const arm_matrix_instance_q31 * pSrcB,
    arm_matrix_instance_q31 * pDst)
{
    q31_t const     *pInB = pSrcB->pData;   /* input data matrix pointer B */
    q31_t const     *pInA = pSrcA->pData;   /* input data matrix pointer A */
    q31_t           *pOut = pDst->pData;   /* output data matrix pointer */
    uint32x4_t    vecColBOffs0, vecColBOffs1;
    q31_t const     *pInA0 = pInA;
    q31_t const     *pInA1 = pInA0 + CMPLX_DIM * MATRIX_DIM4;
    q31_t const     *pInA2 = pInA1 + CMPLX_DIM * MATRIX_DIM4;
    q31_t const     *pInA3 = pInA2 + CMPLX_DIM * MATRIX_DIM4;
    q63_t            acc0, acc1, acc2, acc3;
    q31x4_t        vecB, vecB1, vecA;

    static const uint32_t offsetB0[4] = {
        0, 1,
        MATRIX_DIM4 * CMPLX_DIM, MATRIX_DIM4 * CMPLX_DIM + 1
    };
    static const uint32_t offsetB1[4] = {
        2 * MATRIX_DIM4 * CMPLX_DIM, 2 * MATRIX_DIM4 * CMPLX_DIM + 1,
        3 * MATRIX_DIM4 * CMPLX_DIM, 3 * MATRIX_DIM4 * CMPLX_DIM + 1
    };

    vecColBOffs0 = vldrwq_u32(offsetB0);
    vecColBOffs1 = vldrwq_u32(offsetB1);

    pInB = (q31_t const *) pSrcB->pData;

    vecB = vldrwq_gather_shifted_offset(pInB, vecColBOffs0);
    vecB1 = vldrwq_gather_shifted_offset(pInB, vecColBOffs1);

    vecA = vldrwq_s32(pInA0);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(pInA1);
    acc2 = vmlsldavq_s32(vecA, vecB);
    acc3 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(&pInA0[4]);
    acc0 = vmlsldavaq_s32(acc0, vecA, vecB1);
    acc1 = vmlaldavaxq_s32(acc1, vecA, vecB1);

    vecA = vldrwq_s32(&pInA1[4]);
    acc2 = vmlsldavaq_s32(acc2, vecA, vecB1);
    acc3 = vmlaldavaxq_s32(acc3, vecA, vecB1);

    pOut[0 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc0, 31);
    pOut[0 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc1, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc2, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc3, 31);

    vecA = vldrwq_s32(pInA2);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(pInA3);
    acc2 = vmlsldavq_s32(vecA, vecB);
    acc3 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(&pInA2[4]);
    acc0 = vmlsldavaq_s32(acc0, vecA, vecB1);
    acc1 = vmlaldavaxq_s32(acc1, vecA, vecB1);

    vecA = vldrwq_s32(&pInA3[4]);
    acc2 = vmlsldavaq_s32(acc2, vecA, vecB1);
    acc3 = vmlaldavaxq_s32(acc3, vecA, vecB1);

    pOut[2 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc0, 31);
    pOut[2 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc1, 31);
    pOut[3 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc2, 31);
    pOut[3 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc3, 31);
    pOut += CMPLX_DIM;

    /*
     * move to next B column
     */
    pInB = pInB + CMPLX_DIM;

    vecB = vldrwq_gather_shifted_offset(pInB, vecColBOffs0);
    vecB1 = vldrwq_gather_shifted_offset(pInB, vecColBOffs1);

    vecA = vldrwq_s32(pInA0);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(pInA1);
    acc2 = vmlsldavq_s32(vecA, vecB);
    acc3 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(&pInA0[4]);
    acc0 = vmlsldavaq_s32(acc0, vecA, vecB1);
    acc1 = vmlaldavaxq_s32(acc1, vecA, vecB1);

    vecA = vldrwq_s32(&pInA1[4]);
    acc2 = vmlsldavaq_s32(acc2, vecA, vecB1);
    acc3 = vmlaldavaxq_s32(acc3, vecA, vecB1);

    pOut[0 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc0, 31);
    pOut[0 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc1, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc2, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc3, 31);

    vecA = vldrwq_s32(pInA2);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(pInA3);
    acc2 = vmlsldavq_s32(vecA, vecB);
    acc3 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(&pInA2[4]);
    acc0 = vmlsldavaq_s32(acc0, vecA, vecB1);
    acc1 = vmlaldavaxq_s32(acc1, vecA, vecB1);

    vecA = vldrwq_s32(&pInA3[4]);
    acc2 = vmlsldavaq_s32(acc2, vecA, vecB1);
    acc3 = vmlaldavaxq_s32(acc3, vecA, vecB1);

    pOut[2 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc0, 31);
    pOut[2 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc1, 31);
    pOut[3 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc2, 31);
    pOut[3 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc3, 31);
    pOut += CMPLX_DIM;
    /*
     * move to next B column
     */
    pInB = pInB + CMPLX_DIM;

    vecB = vldrwq_gather_shifted_offset(pInB, vecColBOffs0);
    vecB1 = vldrwq_gather_shifted_offset(pInB, vecColBOffs1);

    vecA = vldrwq_s32(pInA0);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(pInA1);
    acc2 = vmlsldavq_s32(vecA, vecB);
    acc3 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(&pInA0[4]);
    acc0 = vmlsldavaq_s32(acc0, vecA, vecB1);
    acc1 = vmlaldavaxq_s32(acc1, vecA, vecB1);

    vecA = vldrwq_s32(&pInA1[4]);
    acc2 = vmlsldavaq_s32(acc2, vecA, vecB1);
    acc3 = vmlaldavaxq_s32(acc3, vecA, vecB1);

    pOut[0 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc0, 31);
    pOut[0 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc1, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc2, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc3, 31);

    vecA = vldrwq_s32(pInA2);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(pInA3);
    acc2 = vmlsldavq_s32(vecA, vecB);
    acc3 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(&pInA2[4]);
    acc0 = vmlsldavaq_s32(acc0, vecA, vecB1);
    acc1 = vmlaldavaxq_s32(acc1, vecA, vecB1);

    vecA = vldrwq_s32(&pInA3[4]);
    acc2 = vmlsldavaq_s32(acc2, vecA, vecB1);
    acc3 = vmlaldavaxq_s32(acc3, vecA, vecB1);

    pOut[2 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc0, 31);
    pOut[2 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc1, 31);
    pOut[3 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc2, 31);
    pOut[3 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc3, 31);
    pOut += CMPLX_DIM;

    /*
     * move to next B column
     */
    pInB = pInB + CMPLX_DIM;

    vecB = vldrwq_gather_shifted_offset(pInB, vecColBOffs0);
    vecB1 = vldrwq_gather_shifted_offset(pInB, vecColBOffs1);

    vecA = vldrwq_s32(pInA0);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(pInA1);
    acc2 = vmlsldavq_s32(vecA, vecB);
    acc3 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(&pInA0[4]);
    acc0 = vmlsldavaq_s32(acc0, vecA, vecB1);
    acc1 = vmlaldavaxq_s32(acc1, vecA, vecB1);

    vecA = vldrwq_s32(&pInA1[4]);
    acc2 = vmlsldavaq_s32(acc2, vecA, vecB1);
    acc3 = vmlaldavaxq_s32(acc3, vecA, vecB1);

    pOut[0 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc0, 31);
    pOut[0 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc1, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc2, 31);
    pOut[1 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc3, 31);

    vecA = vldrwq_s32(pInA2);
    acc0 = vmlsldavq_s32(vecA, vecB);
    acc1 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(pInA3);
    acc2 = vmlsldavq_s32(vecA, vecB);
    acc3 = vmlaldavxq_s32(vecA, vecB);

    vecA = vldrwq_s32(&pInA2[4]);
    acc0 = vmlsldavaq_s32(acc0, vecA, vecB1);
    acc1 = vmlaldavaxq_s32(acc1, vecA, vecB1);

    vecA = vldrwq_s32(&pInA3[4]);
    acc2 = vmlsldavaq_s32(acc2, vecA, vecB1);
    acc3 = vmlaldavaxq_s32(acc3, vecA, vecB1);

    pOut[2 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc0, 31);
    pOut[2 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc1, 31);
    pOut[3 * CMPLX_DIM * MATRIX_DIM4 + 0] = (q31_t) asrl(acc2, 31);
    pOut[3 * CMPLX_DIM * MATRIX_DIM4 + 1] = (q31_t) asrl(acc3, 31);
    /*
     * Return to application
     */
    return (ARM_MATH_SUCCESS);
}


arm_status arm_mat_cmplx_mult_q31(
  const arm_matrix_instance_q31 * pSrcA,
  const arm_matrix_instance_q31 * pSrcB,
        arm_matrix_instance_q31 * pDst)
{
    q31_t const *pInB = (q31_t const *) pSrcB->pData;   /* input data matrix pointer B */
    q31_t const *pInA = (q31_t const *) pSrcA->pData;   /* input data matrix pointer A */
    q31_t *pOut = pDst->pData;  /* output data matrix pointer */
    q31_t *px;              /* Temporary output data matrix pointer */
    uint16_t  numRowsA = pSrcA->numRows;    /* number of rows of input matrix A    */
    uint16_t  numColsB = pSrcB->numCols;    /* number of columns of input matrix B */
    uint16_t  numColsA = pSrcA->numCols;    /* number of columns of input matrix A */
    uint16_t  col, i = 0U, row = numRowsA, colCnt;  /* loop counters */
    arm_status status;          /* status of matrix multiplication */
    uint32x4_t vecOffs, vecColBOffs;
    uint32_t  blkCnt, rowCnt;           /* loop counters */

  #ifdef ARM_MATH_MATRIX_CHECK

  /* Check for matrix mismatch condition */
  if ((pSrcA->numCols != pSrcB->numRows) ||
     (pSrcA->numRows != pDst->numRows) || (pSrcB->numCols != pDst->numCols))
  {

    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else
#endif /*      #ifdef ARM_MATH_MATRIX_CHECK    */

  {
    /*
     * small squared matrix specialized routines
     */
    if (numRowsA == numColsB && numColsB == numColsA)
    {
        if (numRowsA == 1)
        {
          q63_t sumReal = (q63_t) pInA[0] * pInB[0];
          sumReal -= (q63_t) pInA[1] * pInB[1];

          q63_t sumImag = (q63_t) pInA[0] * pInB[1];
          sumImag += (q63_t) pInA[1] * pInB[0];
       
          /* Store result in destination buffer */
          pOut[0] = (q31_t) clip_q63_to_q31(sumReal >> 31);
          pOut[1] = (q31_t) clip_q63_to_q31(sumImag >> 31);
          return (ARM_MATH_SUCCESS);
        }
        else if (numRowsA == 2)
            return arm_mat_cmplx_mult_q31_2x2_mve(pSrcA, pSrcB, pDst);
        else if (numRowsA == 3)
            return arm_mat_cmplx_mult_q31_3x3_mve(pSrcA, pSrcB, pDst);
        else if (numRowsA == 4)
            return arm_mat_cmplx_mult_q31_4x4_mve(pSrcA, pSrcB, pDst);
    }

    vecColBOffs[0] = 0;
    vecColBOffs[1] = 1;
    vecColBOffs[2] = numColsB * CMPLX_DIM;
    vecColBOffs[3] = (numColsB * CMPLX_DIM) + 1;

    /*
     * The following loop performs the dot-product of each row in pSrcA with each column in pSrcB
     */

    /*
     * row loop
     */
    rowCnt = row >> 1;
    while (rowCnt > 0u)
    {
        /*
         * Output pointer is set to starting address of the row being processed
         */
        px = pOut + i * CMPLX_DIM;
        i = i + 2 * numColsB;
        /*
         * For every row wise process, the column loop counter is to be initiated
         */
        col = numColsB;
        /*
         * For every row wise process, the pInB pointer is set
         * to the starting address of the pSrcB data
         */
        pInB = (q31_t const *) pSrcB->pData;
        /*
         * column loop
         */
        while (col > 0u)
        {
            /*
             * generate 4 columns elements
             */
            /*
             * Matrix A columns number of MAC operations are to be performed
             */
            colCnt = numColsA;

            q31_t const *pSrcA0Vec, *pSrcA1Vec;
            q31_t const *pInA0 = pInA;
            q31_t const *pInA1 = pInA0 + numColsA * CMPLX_DIM;
            q63_t   acc0, acc1, acc2, acc3;

            acc0 = 0LL;
            acc1 = 0LL;
            acc2 = 0LL;
            acc3 = 0LL;

            pSrcA0Vec = (q31_t const *) pInA0;
            pSrcA1Vec = (q31_t const *) pInA1;
            

            vecOffs = vecColBOffs;

            /*
             * process 1 x 2 block output
             */
            blkCnt = (numColsA * CMPLX_DIM) >> 2;
            while (blkCnt > 0U)
            {
                q31x4_t vecB, vecA;

                vecB = vldrwq_gather_shifted_offset(pInB, vecOffs);
                /*
                 * move Matrix B read offsets, 2 rows down
                 */
                vecOffs = vecOffs + (uint32_t) (numColsB * 2 * CMPLX_DIM);


                vecA = vld1q(pSrcA0Vec);  
                pSrcA0Vec += 4;
                acc0 =  vmlsldavaq(acc0, vecA, vecB);
                acc1 =  vmlaldavaxq(acc1, vecA, vecB);

                
                vecA = vld1q(pSrcA1Vec); 
                pSrcA1Vec += 4;
               
                acc2 =  vmlsldavaq(acc2, vecA, vecB);
                acc3 =  vmlaldavaxq(acc3, vecA, vecB);


                blkCnt--;
            }
            

            /*
             * tail
             */
            blkCnt = (numColsA * CMPLX_DIM) & 3;
            if (blkCnt > 0U)
            {
                mve_pred16_t p0 = vctp32q(blkCnt);
                q31x4_t vecB, vecA;

                vecB = vldrwq_gather_shifted_offset_z(pInB, vecOffs, p0);

                /*
                 * move Matrix B read offsets, 2 rows down
                 */
                vecOffs = vecOffs + (uint32_t) (numColsB * 2 * CMPLX_DIM);
               
                
                vecA = vld1q(pSrcA0Vec);
                acc0 =  vmlsldavaq(acc0, vecA, vecB);
                acc1 =  vmlaldavaxq(acc1, vecA, vecB);
                vecA = vld1q(pSrcA1Vec);
                acc2 =  vmlsldavaq(acc2, vecA, vecB);
                acc3 =  vmlaldavaxq(acc3, vecA, vecB);
                

            }

            px[0 * CMPLX_DIM * numColsB + 0] = (q31_t) clip_q63_to_q31(acc0 >> 31);
            px[0 * CMPLX_DIM * numColsB + 1] = (q31_t) clip_q63_to_q31(acc1 >> 31);
            px[1 * CMPLX_DIM * numColsB + 0] = (q31_t) clip_q63_to_q31(acc2 >> 31);
            px[1 * CMPLX_DIM * numColsB + 1] = (q31_t) clip_q63_to_q31(acc3 >> 31);
            px += CMPLX_DIM;
            /*
             * Decrement the column loop counter
             */
            col--;
            /*
             * Update the pointer pInB to point to the  starting address of the next column
             */
            pInB = (q31_t const *) pSrcB->pData + (numColsB - col) * CMPLX_DIM;
        }

        /*
         * Update the pointer pInA to point to the  starting address of the next row
         */
        pInA += (numColsA * 2) * CMPLX_DIM;
        /*
         * Decrement the row loop counter
         */
        rowCnt --;

    }

    rowCnt = row & 1;
    while (rowCnt > 0u)
    {
        /*
         * Output pointer is set to starting address of the row being processed
         */
        px = pOut + i * CMPLX_DIM;
        i = i + numColsB;
        /*
         * For every row wise process, the column loop counter is to be initiated
         */
        col = numColsB;
        /*
         * For every row wise process, the pInB pointer is set
         * to the starting address of the pSrcB data
         */
        pInB = (q31_t const *) pSrcB->pData;
        /*
         * column loop
         */
        while (col > 0u)
        {
            /*
             * generate 4 columns elements
             */
            /*
             * Matrix A columns number of MAC operations are to be performed
             */
            colCnt = numColsA;

            q31_t const *pSrcA0Vec;
            q31_t const *pInA0 = pInA;
            q63_t acc0,acc1;

            acc0 = 0LL;
            acc1 = 0LL;

            pSrcA0Vec = (q31_t const *) pInA0;
           
            vecOffs = vecColBOffs;

            /*
             * process 1 x 2 block output
             */
            blkCnt = (numColsA * CMPLX_DIM) >> 2;
            while (blkCnt > 0U)
            {
                q31x4_t vecB, vecA;

                vecB = vldrwq_gather_shifted_offset(pInB, vecOffs);
                /*
                 * move Matrix B read offsets, 2 rows down
                 */
                vecOffs = vecOffs + (uint32_t) (numColsB * 2 * CMPLX_DIM);
               
                vecA = vld1q(pSrcA0Vec);  
                pSrcA0Vec += 4;
                acc0 =  vmlsldavaq(acc0, vecA, vecB);
                acc1 =  vmlaldavaxq(acc1, vecA, vecB);
                

                blkCnt--;
            }


            /*
             * tail
             */
            blkCnt = (numColsA * CMPLX_DIM) & 3;
            if (blkCnt > 0U)
            {
                mve_pred16_t p0 = vctp32q(blkCnt);
                q31x4_t vecB, vecA;

                vecB = vldrwq_gather_shifted_offset_z(pInB, vecOffs, p0);

                /*
                 * move Matrix B read offsets, 2 rows down
                 */
                vecOffs = vecOffs + (uint32_t) (numColsB * 2 * CMPLX_DIM);
               
                vecA = vld1q(pSrcA0Vec);
               

                acc0 =  vmlsldavaq(acc0, vecA, vecB);
                acc1 =  vmlaldavaxq(acc1, vecA, vecB);


            }

            px[0] = (q31_t) clip_q63_to_q31(acc0 >> 31);
            px[1] = (q31_t) clip_q63_to_q31(acc1 >> 31);

           
            px += CMPLX_DIM;
            /*
             * Decrement the column loop counter
             */
            col--;
            /*
             * Update the pointer pInB to point to the  starting address of the next column
             */
            pInB = (q31_t const *) pSrcB->pData + (numColsB - col) * CMPLX_DIM;
        }

        /*
         * Update the pointer pInA to point to the  starting address of the next row
         */
        pInA += numColsA  * CMPLX_DIM;
        rowCnt--;
    }

    
      /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}

#else
arm_status arm_mat_cmplx_mult_q31(
  const arm_matrix_instance_q31 * pSrcA,
  const arm_matrix_instance_q31 * pSrcB,
        arm_matrix_instance_q31 * pDst)
{
  q31_t *pIn1 = pSrcA->pData;                    /* Input data matrix pointer A */
  q31_t *pIn2 = pSrcB->pData;                    /* Input data matrix pointer B */
  q31_t *pInA = pSrcA->pData;                    /* Input data matrix pointer A */
  q31_t *pOut = pDst->pData;                     /* Output data matrix pointer */
  q31_t *px;                                     /* Temporary output data matrix pointer */
  uint16_t numRowsA = pSrcA->numRows;            /* Number of rows of input matrix A */
  uint16_t numColsB = pSrcB->numCols;            /* Number of columns of input matrix B */
  uint16_t numColsA = pSrcA->numCols;            /* Number of columns of input matrix A */
  q63_t sumReal, sumImag;                        /* Accumulator */
  q31_t a1, b1, c1, d1;
  uint32_t col, i = 0U, j, row = numRowsA, colCnt; /* loop counters */
  arm_status status;                             /* status of matrix multiplication */

#if defined (ARM_MATH_LOOPUNROLL)
  q31_t a0, b0, c0, d0;
#endif

#ifdef ARM_MATH_MATRIX_CHECK

  /* Check for matrix mismatch condition */
  if ((pSrcA->numCols != pSrcB->numRows) ||
      (pSrcA->numRows != pDst->numRows)  ||
      (pSrcB->numCols != pDst->numCols)    )
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else

#endif /* #ifdef ARM_MATH_MATRIX_CHECK */

  {
    /* The following loop performs the dot-product of each row in pSrcA with each column in pSrcB */
    /* row loop */
    do
    {
      /* Output pointer is set to starting address of the row being processed */
      px = pOut + 2 * i;

      /* For every row wise process, the column loop counter is to be initiated */
      col = numColsB;

      /* For every row wise process, the pIn2 pointer is set
       ** to the starting address of the pSrcB data */
      pIn2 = pSrcB->pData;

      j = 0U;

      /* column loop */
      do
      {
        /* Set the variable sum, that acts as accumulator, to zero */
        sumReal = 0.0;
        sumImag = 0.0;

        /* Initiate pointer pIn1 to point to starting address of column being processed */
        pIn1 = pInA;

#if defined (ARM_MATH_LOOPUNROLL)

        /* Apply loop unrolling and compute 4 MACs simultaneously. */
        colCnt = numColsA >> 2U;

        /* matrix multiplication */
        while (colCnt > 0U)
        {

          /* Reading real part of complex matrix A */
          a0 = *pIn1;

          /* Reading real part of complex matrix B */
          c0 = *pIn2;

          /* Reading imaginary part of complex matrix A */
          b0 = *(pIn1 + 1U);

          /* Reading imaginary part of complex matrix B */
          d0 = *(pIn2 + 1U);

          /* Multiply and Accumlates */
          sumReal += (q63_t) a0 * c0;
          sumImag += (q63_t) b0 * c0;

          /* update pointers */
          pIn1 += 2U;
          pIn2 += 2 * numColsB;

          /* Multiply and Accumlates */
          sumReal -= (q63_t) b0 * d0;
          sumImag += (q63_t) a0 * d0;

          /* c(m,n) = a(1,1) * b(1,1) + a(1,2) * b(2,1) + .... + a(m,p) * b(p,n) */

          /* read real and imag values from pSrcA and pSrcB buffer */
          a1 = *(pIn1     );
          c1 = *(pIn2     );
          b1 = *(pIn1 + 1U);
          d1 = *(pIn2 + 1U);

          /* Multiply and Accumlates */
          sumReal += (q63_t) a1 * c1;
          sumImag += (q63_t) b1 * c1;

          /* update pointers */
          pIn1 += 2U;
          pIn2 += 2 * numColsB;

          /* Multiply and Accumlates */
          sumReal -= (q63_t) b1 * d1;
          sumImag += (q63_t) a1 * d1;

          a0 = *(pIn1     );
          c0 = *(pIn2     );
          b0 = *(pIn1 + 1U);
          d0 = *(pIn2 + 1U);

          /* Multiply and Accumlates */
          sumReal += (q63_t) a0 * c0;
          sumImag += (q63_t) b0 * c0;

          /* update pointers */
          pIn1 += 2U;
          pIn2 += 2 * numColsB;

          /* Multiply and Accumlates */
          sumReal -= (q63_t) b0 * d0;
          sumImag += (q63_t) a0 * d0;

          /* c(m,n) = a(1,1) * b(1,1) + a(1,2) * b(2,1) + .... + a(m,p) * b(p,n) */

          a1 = *(pIn1     );
          c1 = *(pIn2     );
          b1 = *(pIn1 + 1U);
          d1 = *(pIn2 + 1U);

          /* Multiply and Accumlates */
          sumReal += (q63_t) a1 * c1;
          sumImag += (q63_t) b1 * c1;

          /* update pointers */
          pIn1 += 2U;
          pIn2 += 2 * numColsB;

          /* Multiply and Accumlates */
          sumReal -= (q63_t) b1 * d1;
          sumImag += (q63_t) a1 * d1;

          /* Decrement loop count */
          colCnt--;
        }

        /* If the columns of pSrcA is not a multiple of 4, compute any remaining MACs here.
         ** No loop unrolling is used. */
        colCnt = numColsA % 0x4U;

#else

        /* Initialize blkCnt with number of samples */
        colCnt = numColsA;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

        while (colCnt > 0U)
        {
          /* c(m,n) = a(1,1) * b(1,1) + a(1,2) * b(2,1) + .... + a(m,p) * b(p,n) */
          a1 = *(pIn1     );
          c1 = *(pIn2     );
          b1 = *(pIn1 + 1U);
          d1 = *(pIn2 + 1U);

          /* Multiply and Accumlates */
          sumReal += (q63_t) a1 * c1;
          sumImag += (q63_t) b1 * c1;

          /* update pointers */
          pIn1 += 2U;
          pIn2 += 2 * numColsB;

          /* Multiply and Accumlates */
          sumReal -= (q63_t) b1 * d1;
          sumImag += (q63_t) a1 * d1;

          /* Decrement loop counter */
          colCnt--;
        }

        /* Store result in destination buffer */
        *px++ = (q31_t) clip_q63_to_q31(sumReal >> 31);
        *px++ = (q31_t) clip_q63_to_q31(sumImag >> 31);

        /* Update pointer pIn2 to point to starting address of next column */
        j++;
        pIn2 = pSrcB->pData + 2U * j;

        /* Decrement column loop counter */
        col--;

      } while (col > 0U);

      /* Update pointer pInA to point to starting address of next row */
      i = i + numColsB;
      pInA = pInA + 2 * numColsA;

      /* Decrement row loop counter */
      row--;

    } while (row > 0U);

    /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of MatrixMult group
 */
