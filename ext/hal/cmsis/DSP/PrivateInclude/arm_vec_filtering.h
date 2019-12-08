/******************************************************************************
 * @file     arm_vec_filtering.h
 * @brief    Private header file for CMSIS DSP Library
 * @version  V1.7.0
 * @date     30. October 2019
 ******************************************************************************/
/*
 * Copyright (c) 2010-2019 Arm Limited or its affiliates. All rights reserved.
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

#ifndef _ARM_VEC_FILTERING_H_
#define _ARM_VEC_FILTERING_H_

#include "arm_math.h"
#include "arm_helium_utils.h"

#ifdef   __cplusplus
extern "C"
{
#endif

#if (defined(ARM_MATH_MVEF) || defined(ARM_MATH_HELIUM)) && !defined(ARM_MATH_AUTOVECTORIZE)

#define MVE_INTR_CORR_QUAD_INC_X_FIXED_SIZE_F32(acc0, acc1, acc2, acc3, pX, pY, count)\
{                                                                                     \
    float32_t const *pSrcX, *pSrcY;                                                   \
    f32x4_t   acc0Vec, acc1Vec, acc2Vec, acc3Vec, xVec, yVec;                         \
    uint32_t    k;                                                                    \
                                                                                      \
    acc0Vec = vdupq_n_f32(0.0f);                                                      \
    acc1Vec = vdupq_n_f32(0.0f);                                                      \
    acc2Vec = vdupq_n_f32(0.0f);                                                      \
    acc3Vec = vdupq_n_f32(0.0f);                                                      \
    pSrcX = (float32_t const *) pX;                                                   \
    pSrcY = (float32_t const *) pY;                                                   \
    k = count >> 2;                                                                   \
                                                                                      \
    while (k > 0U)                                                                    \
    {                                                                                 \
        yVec = vld1q(pSrcY);                                                          \
        pSrcY += 4;                                                                   \
        xVec = vldrwq_f32(&pSrcX[1]);                                                 \
        acc1Vec = vfmaq_f32(acc1Vec, xVec, yVec);                                     \
        xVec = vldrwq_f32(&pSrcX[2]);                                                 \
        acc2Vec = vfmaq_f32(acc2Vec, xVec, yVec);                                     \
        xVec = vldrwq_f32(&pSrcX[3]);                                                 \
        acc3Vec = vfmaq_f32(acc3Vec, xVec, yVec);                                     \
        xVec = vld1q(pSrcX);                                                          \
        pSrcX += 4;                                                                   \
        acc0Vec = vfmaq_f32(acc0Vec, xVec, yVec);                                     \
        /*  Decrement the loop counter   */                                           \
        k--;                                                                          \
    }                                                                                 \
    /* loop + tail predication expected here  */                                      \
    k = count % 0x4U;                                                                 \
    if (k > 0U)                                                                       \
    {                                                                                 \
        mve_pred16_t p0 = vctp32q(k);                                                 \
        yVec = vld1q(pSrcY);                                                          \
        pSrcY += 4;                                                                   \
        xVec = vldrwq_f32(&pSrcX[1]);                                                 \
        acc1Vec = vfmaq_m_f32(acc1Vec, xVec, yVec, p0);                               \
        xVec = vldrwq_f32(&pSrcX[2]);                                                 \
        acc2Vec = vfmaq_m_f32(acc2Vec, xVec, yVec, p0);                               \
        xVec = vldrwq_f32(&pSrcX[3]);                                                 \
        acc3Vec = vfmaq_m_f32(acc3Vec, xVec, yVec, p0);                               \
        xVec = vld1q(pSrcX);                                                          \
        pSrcX += 4;                                                                   \
        acc0Vec = vfmaq_m_f32(acc0Vec, xVec, yVec, p0);                               \
    }                                                                                 \
                                                                                      \
    acc0 = vecAddAcrossF32Mve(acc0Vec);                                               \
    acc1 = vecAddAcrossF32Mve(acc1Vec);                                               \
    acc2 = vecAddAcrossF32Mve(acc2Vec);                                               \
    acc3 = vecAddAcrossF32Mve(acc3Vec);                                               \
}

#define MVE_INTR_CORR_SINGLE_F32(acc, pX, pY, count) \
{                                                    \
    float32_t const *pSrcX, *pSrcY;                  \
    f32x4_t   accVec, xVec, yVec;                    \
    uint32_t    k;                                   \
                                                     \
    accVec = vdupq_n_f32(0.0f);                      \
    pSrcX = (float32_t const *) pX;                  \
    pSrcY = (float32_t const *) pY;                  \
    k = count >> 2;                                  \
                                                     \
    while (k > 0U)                                   \
    {                                                \
        yVec = vld1q(pSrcY);                         \
        pSrcY += 4;                                  \
        xVec = vld1q(pSrcX);                         \
        pSrcX += 4;                                  \
        accVec = vfmaq_f32(accVec, xVec, yVec);      \
        /*  Decrement the loop counter   */          \
        k--;                                         \
    }                                                \
    /* Loop with tail predication expected here  */  \
    k = count % 0x4U;                                \
    if (k > 0U)                                      \
    {                                                \
        mve_pred16_t p0 = vctp32q(k);                \
        yVec = vld1q(pSrcY);                         \
        pSrcY += 4;                                  \
        xVec = vld1q(pSrcX);                         \
        pSrcX += 4;                                  \
        accVec = vfmaq_m_f32(accVec, xVec, yVec, p0);\
    }                                                \
    acc = vecAddAcrossF32Mve(accVec);                \
}

#define MVE_INTR_CORR_DUAL_INC_X_DEC_SIZE_F32(acc0, acc1, pX, pY, count)\
{                                                                       \
    float32_t const *pSrcX, *pSrcY;                                     \
    f32x4_t   acc0Vec, acc1Vec, xVec, yVec;                             \
    uint32_t    k;                                                      \
                                                                        \
    acc0Vec = vdupq_n_f32(0.0f);                                        \
    acc1Vec = vdupq_n_f32(0.0f);                                        \
    pSrcX = (float32_t const *) pX;                                     \
    pSrcY = (float32_t const *) pY;                                     \
    k = (count-1) >> 2;                                                 \
                                                                        \
    while (k > 0U)                                                      \
    {                                                                   \
        yVec = vld1q(pSrcY);                                            \
        pSrcY += 4;                                                     \
        xVec = vldrwq_f32(&pSrcX[1]);                                   \
        acc1Vec = vfmaq_f32(acc1Vec, xVec, yVec);                       \
        xVec = vld1q(pSrcX);                                            \
        pSrcX += 4;                                                     \
        acc0Vec = vfmaq_f32(acc0Vec, xVec, yVec);                       \
        /*  Decrement the loop counter   */                             \
        k--;                                                            \
    }                                                                   \
    /* use predication to finalize MAC sum */                           \
    /* acc1 requires exact number of sample (count-1)  */               \
    /* disable extra lanes in final MAC computation  */                 \
    k = (count-1) % 0x4U;                                               \
    mve_pred16_t p0 = vctp32q(k);                                       \
    yVec = vld1q(pSrcY);                                                \
    pSrcY += 4;                                                         \
    xVec = vldrwq_f32(&pSrcX[1]);                                       \
    acc1Vec = vfmaq_m_f32(acc1Vec, xVec, yVec, p0);                     \
    /* acc0 requires 1 additional sample  (count) */                    \
    /* so add 1 to unmask an extra lane  in final MAC computation  */   \
    p0 = vctp32q(k+1);                                                  \
    xVec = vld1q(pSrcX);                                                \
    pSrcX += 4;                                                         \
    acc0Vec = vfmaq_m_f32(acc0Vec, xVec, yVec, p0);                     \
                                                                        \
    acc0 = vecAddAcrossF32Mve(acc0Vec);                                 \
    acc1 = vecAddAcrossF32Mve(acc1Vec);                                 \
}

#define MVE_INTR_CORR_DUAL_INC_X_FIXED_SIZE_F32(acc0, acc1, pX, pY, count)\
{                                                                         \
    float32_t const *pSrcX, *pSrcY;                                       \
    f32x4_t   acc0Vec, acc1Vec, xVec, yVec;                               \
    uint32_t    k;                                                        \
                                                                          \
    acc0Vec = vdupq_n_f32(0.0f);                                          \
    acc1Vec = vdupq_n_f32(0.0f);                                          \
    pSrcX = (float32_t const *) pX;                                       \
    pSrcY = (float32_t const *) pY;                                       \
    k = count >> 2;                                                       \
                                                                          \
    while (k > 0U)                                                        \
    {                                                                     \
        yVec = vld1q(pSrcY);                                              \
        pSrcY += 4;                                                       \
        xVec = vldrwq_f32(&pSrcX[1]);                                     \
        acc1Vec = vfmaq_f32(acc1Vec, xVec, yVec);                         \
        xVec = vld1q(pSrcX);                                              \
        pSrcX += 4;                                                       \
        acc0Vec = vfmaq_f32(acc0Vec, xVec, yVec);                         \
        /*  Decrement the loop counter   */                               \
        k--;                                                              \
    }                                                                     \
    /* loop + tail predication expected here  */                          \
    k = count % 0x4U;                                                     \
    if (k > 0U)                                                           \
    {                                                                     \
        mve_pred16_t p0 = vctp32q(k);                                     \
        yVec = vld1q(pSrcY);                                              \
        pSrcY += 4;                                                       \
        xVec = vldrwq_f32(&pSrcX[1]);                                     \
        acc1Vec = vfmaq_m_f32(acc1Vec, xVec, yVec, p0);                   \
        xVec = vld1q(pSrcX);                                              \
        pSrcX += 4;                                                       \
        acc0Vec = vfmaq_m_f32(acc0Vec, xVec, yVec, p0);                   \
    }                                                                     \
                                                                          \
    acc0 = vecAddAcrossF32Mve(acc0Vec);                                   \
    acc1 = vecAddAcrossF32Mve(acc1Vec);                                   \
}

#define MVE_INTR_CORR_DUAL_DEC_Y_INC_SIZE_F32(acc0, acc1, pX, pY, count)\
{                                                                       \
    float32_t const *pSrcX, *pSrcY;                                     \
    f32x4_t   acc0Vec, acc1Vec, xVec, yVec;                             \
    uint32_t    k;                                                      \
                                                                        \
    acc0Vec = vdupq_n_f32(0.0f);                                        \
    acc1Vec = vdupq_n_f32(0.0f);                                        \
    pSrcX = (float32_t const *) pX;                                     \
    pSrcY = (float32_t const *) pY;                                     \
    k = count >> 2;                                                     \
    while (k > 0U)                                                      \
    {                                                                   \
        xVec = vld1q(pSrcX);                                            \
        pSrcX += 4;                                                     \
        yVec = vldrwq_f32(&pSrcY[-1]);                                  \
        acc1Vec = vfmaq_f32(acc1Vec, xVec, yVec);                       \
        yVec = vld1q(pSrcY);                                            \
        pSrcY += 4;                                                     \
        acc0Vec = vfmaq_f32(acc0Vec, xVec, yVec);                       \
        /*  Decrement the loop counter   */                             \
        k--;                                                            \
    }                                                                   \
    k = count % 0x4U;                                                   \
    /* use predication to finalize MAC sum */                           \
    /* acc1 requires 1 additional sample  */                            \
    /* so add 1 to unmask an extra lane  in final MAC computation  */   \
    mve_pred16_t p0 = vctp32q(k+1);                                     \
    xVec = vld1q(pSrcX);                                                \
    pSrcX += 4;                                                         \
    yVec = vldrwq_f32(&pSrcY[-1]);                                      \
    acc1Vec = vfmaq_m_f32(acc1Vec, xVec, yVec,p0);                      \
    /* acc0 requires exact number of sample  */                         \
    /* disable extra lanes in final MAC computation  */                 \
    p0 = vctp32q(k);                                                    \
    yVec = vld1q(pSrcY);                                                \
    pSrcY += 4;                                                         \
    acc0Vec = vfmaq_m_f32(acc0Vec, xVec, yVec,p0);                      \
                                                                        \
    acc0 = vecAddAcrossF32Mve(acc0Vec);                                 \
    acc1 = vecAddAcrossF32Mve(acc1Vec);                                 \
}

#define MVE_INTR_CONV_DUAL_INC_X_DEC_SIZE_F32(acc0, acc1, pX, pY, count)                            \
{                                                                                                   \
    float32_t const *pSrcX;                                                                         \
    f32x4_t   acc0Vec, acc1Vec, xVec, yVec;                                                       \
    uint32_t    k;                                                                                  \
                                                                                                    \
    acc0Vec = vdupq_n_f32(0.0f);                                                                    \
    acc1Vec = vdupq_n_f32(0.0f);                                                                    \
    pSrcX = (float32_t const *) pX;                                                                 \
    k = (count - 1) >> 2;                                                                           \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        /* note */                                                                                  \
        /* could can be more efficient using Vector Scatter Store: */                               \
        /* + pre-increment + WB */                                                                  \
        /* To be revisited when intrinsic available */                                              \
        /* SDCOMP-52618 */                                                                          \
        yVec = vldrwq_gather_shifted_offset_f32(pY, decrIdxVec);                                    \
        pY-=4;                                                                                      \
        xVec = vldrwq_f32(&pSrcX[1]);                                                               \
        acc1Vec = vfmaq_f32(acc1Vec, xVec, yVec);                                                   \
        xVec = vld1q(pSrcX);  pSrcX += 4;                                               \
        acc0Vec = vfmaq_f32(acc0Vec, xVec, yVec);                                                   \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* Loop with tail predication expected here  */                                                 \
    k = (count - 1) % 0x4U;                                                                         \
    mve_pred16_t p0 = vctp32q(k);                                                        \
    yVec = vldrwq_gather_shifted_offset_f32(pY, decrIdxVec);                                        \
    xVec = vldrwq_f32(&pSrcX[1]);                                                                   \
    acc1Vec = vfmaq_m_f32(acc1Vec, xVec, yVec, p0);                                                 \
    xVec = vld1q(pSrcX);  pSrcX += 4;                                                   \
    p0 = vctp32q(k+1);                                                                   \
    acc0Vec = vfmaq_m_f32(acc0Vec, xVec, yVec, p0);                                                 \
                                                                                                    \
    acc0 = vecAddAcrossF32Mve(acc0Vec);                                                             \
    acc1 = vecAddAcrossF32Mve(acc1Vec);                                                             \
}

#define MVE_INTR_CONV_DUAL_INC_X_FIXED_SIZE_F32(acc0, acc1, pX, pY, count)                          \
{                                                                                                   \
    float32_t const *pSrcX;                                                                         \
    f32x4_t   acc0Vec, acc1Vec, xVec, yVec;                                                       \
    uint32_t    k;                                                                                  \
                                                                                                    \
    acc0Vec = vdupq_n_f32(0.0f);                                                                    \
    acc1Vec = vdupq_n_f32(0.0f);                                                                    \
    pSrcX = (float32_t const *) pX;                                                                 \
    k = count >> 2;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        /* note */                                                                                  \
        /* could can be more efficient using Vector Scatter Store: */                               \
        /* + pre-increment + WB */                                                                  \
        /* To be revisited when intrinsic available */                                              \
        /* SDCOMP-52618 */                                                                          \
        yVec = vldrwq_gather_shifted_offset_f32(pY, decrIdxVec);                                    \
        pY-=4;                                                                                      \
        xVec = vldrwq_f32(&pSrcX[1]);                                                               \
        acc1Vec = vfmaq_f32(acc1Vec, xVec, yVec);                                                   \
        xVec = vld1q(pSrcX);  pSrcX += 4;                                               \
        acc0Vec = vfmaq_f32(acc0Vec, xVec, yVec);                                                   \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* Loop with tail predication expected here  */                                                 \
    k = count % 0x4U;                                                                               \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp32q(k);                                                    \
        yVec = vldrwq_gather_shifted_offset_f32(pY, decrIdxVec);                                    \
        xVec = vldrwq_f32(&pSrcX[1]);                                                               \
        acc1Vec = vfmaq_m_f32(acc1Vec, xVec, yVec, p0);                                             \
        xVec = vld1q(pSrcX);  pSrcX += 4;                                               \
        acc0Vec = vfmaq_m_f32(acc0Vec, xVec, yVec, p0);                                             \
    }                                                                                               \
    acc0 = vecAddAcrossF32Mve(acc0Vec);                                                             \
    acc1 = vecAddAcrossF32Mve(acc1Vec);                                                             \
}

#define MVE_INTR_CONV_DUAL_INC_Y_INC_SIZE_F32(acc0, acc1, pX, pY, count)                            \
{                                                                                                   \
    float32_t   const *pSrcX;                                                                       \
    const float32_t  *pY1 = pY + 1;                                                                       \
    f32x4_t   acc0Vec, acc1Vec, xVec, yVec;                                                       \
    uint32_t    k;                                                                                  \
                                                                                                    \
    acc0Vec = vdupq_n_f32(0.0f);                                                                    \
    acc1Vec = vdupq_n_f32(0.0f);                                                                    \
    pSrcX = (float32_t const *) pX;                                                                 \
    k = count >> 2;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        xVec = vld1q(pSrcX);  pSrcX += 4;                                               \
        yVec = vldrwq_gather_shifted_offset_f32(pY, decrIdxVec);                                    \
        pY-=4;                                                                                      \
        acc0Vec = vfmaq_f32(acc0Vec, xVec, yVec);                                                   \
        yVec = vldrwq_gather_shifted_offset_f32(pY1, decrIdxVec);                                   \
        pY1-=4;                                                                                     \
        acc1Vec = vfmaq_f32(acc1Vec, xVec, yVec);                                                   \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    k = count % 0x4U;                                                                               \
    /* use predication to finalize MAC sum */                                                       \
    /* acc0 requires exact number of sample  */                                                     \
    /* disable extra lanes in final MAC computation  */                                             \
    mve_pred16_t p0 = vctp32q(k);                                                        \
    xVec = vld1q(pSrcX);  pSrcX += 4;                                                   \
    yVec = vldrwq_gather_shifted_offset_f32(pY, decrIdxVec);                                        \
    acc0Vec = vfmaq_m_f32(acc0Vec, xVec, yVec, p0);                                                 \
    yVec = vldrwq_gather_shifted_offset_f32(pY1, decrIdxVec);                                       \
    /* acc1 requires 1 additional sample  */                                                        \
    /* so add 1 to unmask an extra lane  in final MAC computation  */                               \
    p0 = vctp32q(k+1);                                                                   \
    acc1Vec = vfmaq_m_f32(acc1Vec, xVec, yVec, p0);                                                 \
                                                                                                    \
    acc0 = vecAddAcrossF32Mve(acc0Vec);                                                             \
    acc1 = vecAddAcrossF32Mve(acc1Vec);                                                             \
}

#define MVE_INTR_CONV_SINGLE_F32(acc, pX, pY, count)                                                \
{                                                                                                   \
    float32_t const *pSrcX;                                                                         \
    f32x4_t   accVec, xVec, yVec;                                                                 \
    uint32_t    k;                                                                                  \
                                                                                                    \
    accVec = vdupq_n_f32(0.0f);                                                                     \
    pSrcX = (float32_t const *) pX;                                                                 \
    k = count >> 2;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        /* note */                                                                                  \
        /* could can be more efficient using Vector Scatter Store: */                               \
        /* + pre-increment + WB */                                                                  \
        /* To be revisited when intrinsic available */                                              \
        /* SDCOMP-52618 */                                                                          \
        yVec = vldrwq_gather_shifted_offset_f32(pY, decrIdxVec);                                    \
        pY-=4;                                                                                      \
        xVec = vld1q(pSrcX);  pSrcX += 4;                                               \
        accVec = vfmaq_f32(accVec, xVec, yVec);                                                     \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* Loop with tail predication expected here  */                                                 \
    k = count % 0x4U;                                                                               \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp32q(k);                                                    \
        xVec = vld1q(pSrcX);  pSrcX += 4;                                               \
        yVec = vldrwq_gather_shifted_offset_f32(pY, decrIdxVec);                                    \
        accVec = vfmaq_m_f32(accVec, xVec, yVec, p0);                                               \
    }                                                                                               \
    acc = vecAddAcrossF32Mve(accVec);                                                               \
}

#endif /* (defined(ARM_MATH_MVEF) || defined(ARM_MATH_HELIUM)) && !defined(ARM_MATH_AUTOVECTORIZE)*/

#if (defined(ARM_MATH_MVEI) || defined(ARM_MATH_HELIUM))

#define MVE_INTR_CONV_SINGLE_Q31(acc, pX, pY, count)                                                \
{                                                                                                   \
    q31_t const *pSrcX;                                                                             \
    q31x4_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q31_t const *) pX;                                                                     \
    k = count >> 2;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        /* note */                                                                                  \
        /* could can be more efficient using Vector Scatter Store: */                               \
        /* + pre-increment + WB */                                                                  \
        /* To be revisited when intrinsic available */                                              \
        /* SDCOMP-52618 */                                                                          \
        yVec = vldrwq_gather_shifted_offset_s32(pY, decrIdxVec);                                    \
        pY-=4;                                                                                      \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        acc = vmlaldavaq(acc, xVec, yVec);                                                          \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* Loop with tail predication expected here  */                                                 \
    k = count % 0x4U;                                                                               \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp32q(k);                                                    \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        yVec = vldrwq_gather_shifted_offset_s32(pY, decrIdxVec);                                    \
        acc = vmlaldavaq_p(acc, xVec, yVec, p0);                                                    \
    }                                                                                               \
    acc = asrl(acc, 31);                                                                            \
}



#define MVE_INTR_CONV_DUAL_INC_Y_INC_SIZE_Q31(acc0, acc1, pX, pY, count)                            \
{                                                                                                   \
    q31_t const *pSrcX;                                                                             \
    const q31_t       *pY1 = pY + 1;                                                                      \
    q31x4_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q31_t const *) pX;                                                                     \
    k = count >> 2;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        yVec = vldrwq_gather_shifted_offset_s32(pY, decrIdxVec);                                    \
        pY-=4;                                                                                      \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        yVec = vldrwq_gather_shifted_offset_s32(pY1, decrIdxVec);                                   \
        pY1-=4;                                                                                     \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    k = count % 0x4U;                                                                               \
    /* use predication to finalize MAC sum */                                                       \
    /* acc0 requires exact number of sample  */                                                     \
    /* disable extra lanes in final MAC computation  */                                             \
    mve_pred16_t p0 = vctp32q(k);                                                        \
    xVec = vld1q(pSrcX); pSrcX += 4;                                                    \
    yVec = vldrwq_gather_shifted_offset_s32(pY, decrIdxVec);                                        \
    acc0 = vmlaldavaq_p(acc0, xVec, yVec, p0);                                                      \
    yVec = vldrwq_gather_shifted_offset_s32(pY1, decrIdxVec);                                       \
    /* acc1 requires 1 additional sample  */                                                        \
    /* so add 1 to unmask an extra lane  in final MAC computation  */                               \
    p0 = vctp32q(k+1);                                                                   \
    acc1 = vmlaldavaq_p(acc1, xVec, yVec, p0);                                                      \
                                                                                                    \
    acc0 = asrl(acc0, 31);                                                                          \
    acc1 = asrl(acc1, 31);                                                                          \
}




#define MVE_INTR_CONV_DUAL_INC_X_DEC_SIZE_Q31(acc0, acc1, pX, pY, count)\
{                                                                       \
    q31_t const *pSrcX;                                                 \
    q31x4_t   xVec, yVec;                                               \
    uint32_t    k;                                                      \
                                                                        \
    pSrcX = (q31_t const *) pX;                                         \
    k = (count-1) >> 2;                                                 \
                                                                        \
    while (k > 0U)                                                      \
    {                                                                   \
        /* note */                                                      \
        /* could can be more efficient using Vector Scatter Store: */   \
        /* + pre-increment + WB */                                      \
        /* To be revisited when intrinsic available */                  \
        /* SDCOMP-52618 */                                              \
        yVec = vldrwq_gather_shifted_offset_s32(pY, decrIdxVec);        \
        pY-=4;                                                          \
        xVec = vldrwq_s32(&pSrcX[1]);                                   \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                            \
        xVec = vld1q(pSrcX);                                            \
        pSrcX += 4;                                                     \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                            \
        /*  Decrement the loop counter   */                             \
        k--;                                                            \
    }                                                                   \
    k = (count - 1) % 0x4U;                                             \
    /* use predication to finalize MAC sum */                           \
    /* acc1 requires exact number of sample (count-1)  */               \
    /* disable extra lanes in final MAC computation  */                 \
    mve_pred16_t p0 = vctp32q(k);                                       \
    yVec = vldrwq_gather_shifted_offset_s32(pY, decrIdxVec);            \
    xVec = vldrwq_s32(&pSrcX[1]);                                       \
    acc1 = vmlaldavaq_p(acc1, xVec, yVec, p0);                          \
    /* acc0 requires 1 additional sample  (count) */                    \
    /* so add 1 to unmask an extra lane  in final MAC computation  */   \
    p0 = vctp32q(k+1);                                                  \
    xVec = vld1q(pSrcX);                                                \
    pSrcX += 4;                                                         \
    acc0 = vmlaldavaq_p(acc0, xVec, yVec, p0);                          \
                                                                        \
    acc0 = asrl(acc0, 31);                                              \
    acc1 = asrl(acc1, 31);                                              \
}



#define MVE_INTR_CONV_DUAL_INC_X_FIXED_SIZE_Q31(acc0, acc1, pX, pY, count)                          \
{                                                                                                   \
    q31_t const *pSrcX;                                                                             \
    q31x4_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q31_t const *) pX;                                                                     \
    k = count >> 2;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        /* note */                                                                                  \
        /* could can be more efficient using Vector Scatter Store: */                               \
        /* + pre-increment + WB */                                                                  \
        /* To be revisited when intrinsic available */                                              \
        /* SDCOMP-52618 */                                                                          \
        yVec = vldrwq_gather_shifted_offset_s32(pY, decrIdxVec);                                    \
        pY-=4;                                                                                      \
        xVec = vldrwq_s32(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* Loop with tail predication expected here  */                                                 \
    k = count % 0x4U;                                                                               \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp32q(k);                                                    \
        yVec = vldrwq_gather_shifted_offset_s32(pY, decrIdxVec);                                    \
        xVec = vldrwq_s32(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq_p(acc1, xVec, yVec, p0);                                                  \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        acc0 = vmlaldavaq_p(acc0, xVec, yVec, p0);                                                  \
    }                                                                                               \
    acc0 = asrl(acc0, 31);                                                                          \
    acc1 = asrl(acc1, 31);                                                                          \
}



#define MVE_INTR_CONV_QUAD_INC_X_FIXED_SIZE_Q31(acc0, acc1, acc2, acc3, pX, pY, count)              \
{                                                                                                   \
    q31_t const *pSrcX;                                                                             \
    q31x4_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q31_t const *) pX;                                                                     \
    k = count >> 2;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        /* note */                                                                                  \
        /* could can be more efficient using Vector Scatter Store: */                               \
        /* + pre-increment + WB */                                                                  \
        /* To be revisited when intrinsic available */                                              \
        /* SDCOMP-52618 */                                                                          \
        yVec = vldrwq_gather_shifted_offset_s32(pY, decrIdxVec);                                    \
        pY-=4;                                                                                      \
        xVec = vldrwq_s32(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        xVec = vldrwq_s32(&pSrcX[2]);                                                               \
        acc2 = vmlaldavaq(acc2, xVec, yVec);                                                        \
        xVec = vldrwq_s32(&pSrcX[3]);                                                               \
        acc3 = vmlaldavaq(acc3, xVec, yVec);                                                        \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* Loop with tail predication expected here  */                                                 \
    k = count % 0x4U;                                                                               \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp32q(k);                                                    \
        yVec = vldrwq_gather_shifted_offset_s32(pY, decrIdxVec);                                    \
        xVec = vldrwq_s32(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq_p(acc1, xVec, yVec, p0);                                                  \
        xVec = vldrwq_s32(&pSrcX[2]);                                                               \
        acc2 = vmlaldavaq_p(acc2, xVec, yVec, p0);                                                  \
        xVec = vldrwq_s32(&pSrcX[3]);                                                               \
        acc3 = vmlaldavaq_p(acc3, xVec, yVec, p0);                                                  \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        acc0 = vmlaldavaq_p(acc0, xVec, yVec, p0);                                                  \
    }                                                                                               \
    acc0 = asrl(acc0, 31);                                                                          \
    acc1 = asrl(acc1, 31);                                                                          \
    acc2 = asrl(acc2, 31);                                                                          \
    acc3 = asrl(acc3, 31);                                                                          \
}

#define MVE_INTR_CORR_DUAL_DEC_Y_INC_SIZE_Q31(acc0, acc1, pX, pY, count)                            \
{                                                                                                   \
    q31_t const *pSrcX, *pSrcY;                                                                     \
    q31x4_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q31_t const *) pX;                                                                     \
    pSrcY  = (q31_t const *) pY;                                                                    \
    k = count >> 2;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        yVec = vldrwq_s32(&pSrcY[-1]);                                                              \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        yVec = vld1q(pSrcY); pSrcY += 4;                                                \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    k = count % 0x4U;                                                                               \
    /* use predication to finalize MAC sum */                                                       \
    /* acc1 requires 1 additional sample  */                                                        \
    /* so add 1 to unmask an extra lane  in final MAC computation  */                               \
    mve_pred16_t p0 = vctp32q(k+1);                                                      \
    xVec = vld1q(pSrcX); pSrcX += 4;                                                    \
    yVec = vldrwq_s32(&pSrcY[-1]);                                                                  \
    acc1 = vmlaldavaq_p(acc1, xVec, yVec,p0);                                                       \
    /* acc0 requires exact number of sample  */                                                     \
    /* disable extra lanes in final MAC computation  */                                             \
    p0 = vctp32q(k);                                                                     \
    yVec = vld1q(pSrcY);  pSrcY += 4;                                                   \
    acc0 = vmlaldavaq_p(acc0, xVec, yVec,p0);                                                       \
                                                                                                    \
    acc0 = asrl(acc0, 31);                                                                          \
    acc1 = asrl(acc1, 31);                                                                          \
}

#define MVE_INTR_CORR_SINGLE_Q31(acc, pX, pY, count)                                                \
{                                                                                                   \
    q31_t const *pSrcX, *pSrcY;                                                                     \
    q31x4_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q31_t const *) pX;                                                                     \
    pSrcY = (q31_t const *) pY;                                                                     \
    k = count >> 2;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        yVec = vld1q(pSrcY); pSrcY += 4;                                                \
        acc = vmlaldavaq(acc, xVec, yVec);                                                          \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /*  tail predication expected here  */                                                          \
    k = count % 0x4U;                                                                               \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp32q(k);                                                    \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        yVec = vld1q(pSrcY); pSrcY += 4;                                                \
        acc = vmlaldavaq_p(acc, xVec, yVec, p0);                                                    \
    }                                                                                               \
    acc = asrl(acc, 31);                                                                            \
}

#define MVE_INTR_CORR_QUAD_INC_X_FIXED_SIZE_Q31(acc0, acc1, acc2, acc3, pX, pY, count)              \
{                                                                                                   \
    q31_t const *pSrcX, *pSrcY;                                                                     \
    q31x4_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q31_t const *) pX;                                                                     \
    pSrcY  = (q31_t const *) pY;                                                                    \
    k = count >> 2;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        yVec = vld1q(pSrcY); pSrcY += 4;                                                \
        xVec = vldrwq_s32(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        xVec = vldrwq_s32(&pSrcX[2]);                                                               \
        acc2 = vmlaldavaq(acc2, xVec, yVec);                                                        \
        xVec = vldrwq_s32(&pSrcX[3]);                                                               \
        acc3 = vmlaldavaq(acc3, xVec, yVec);                                                        \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* loop + tail predication expected here  */                                                    \
    k = count % 0x4U;                                                                               \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp32q(k);                                                    \
        yVec = vld1q(pSrcY); pSrcY += 4;                                                \
        xVec = vldrwq_s32(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq_p(acc1, xVec, yVec, p0);                                                  \
        xVec = vldrwq_s32(&pSrcX[2]);                                                               \
        acc2 = vmlaldavaq_p(acc2, xVec, yVec, p0);                                                  \
        xVec = vldrwq_s32(&pSrcX[3]);                                                               \
        acc3 = vmlaldavaq_p(acc3, xVec, yVec, p0);                                                  \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        acc0 = vmlaldavaq_p(acc0, xVec, yVec, p0);                                                  \
    }                                                                                               \
                                                                                                    \
    acc0 = asrl(acc0, 31);                                                                          \
    acc1 = asrl(acc1, 31);                                                                          \
    acc2 = asrl(acc2, 31);                                                                          \
    acc3 = asrl(acc3, 31);                                                                          \
}

#define MVE_INTR_CORR_DUAL_INC_X_FIXED_SIZE_Q31(acc0, acc1, pX, pY, count)                          \
{                                                                                                   \
    q31_t const *pSrcX, *pSrcY;                                                                     \
    q31x4_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q31_t const *) pX;                                                                     \
    pSrcY  = (q31_t const *) pY;                                                                    \
    k = count >> 2;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        yVec = vld1q(pSrcY); pSrcY += 4;                                                \
        xVec = vldrwq_s32(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* loop + tail predication expected here  */                                                    \
    k = count % 0x4U;                                                                               \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp32q(k);                                                    \
        yVec = vld1q(pSrcY); pSrcY += 4;                                                \
        xVec = vldrwq_s32(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq_p(acc1, xVec, yVec, p0);                                                  \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        acc0 = vmlaldavaq_p(acc0, xVec, yVec, p0);                                                  \
    }                                                                                               \
                                                                                                    \
    acc0 = asrl(acc0, 31);                                                                          \
    acc1 = asrl(acc1, 31);                                                                          \
}

#define MVE_INTR_CORR_DUAL_INC_X_DEC_SIZE_Q31(acc0, acc1, pX, pY, count)                            \
{                                                                                                   \
    q31_t const *pSrcX, *pSrcY;                                                                     \
    q31x4_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q31_t const *) pX;                                                                     \
    pSrcY  = (q31_t const *) pY;                                                                    \
    k = (count-1) >> 2;                                                                             \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        yVec = vld1q(pSrcY); pSrcY += 4;                                                \
        xVec = vldrwq_s32(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        xVec = vld1q(pSrcX); pSrcX += 4;                                                \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* use predication to finalize MAC sum */                                                       \
    /* acc1 requires exact number of sample (count-1)  */                                           \
    /* disable extra lanes in final MAC computation  */                                             \
    k = (count-1) % 0x4U;                                                                           \
    mve_pred16_t p0 = vctp32q(k);                                                        \
    yVec = vld1q(pSrcY);  pSrcY += 4;                                                   \
    xVec = vldrwq_s32(&pSrcX[1]);                                                                   \
    acc1 = vmlaldavaq_p(acc1, xVec, yVec, p0);                                                      \
    /* acc0 requires 1 additional sample  (count) */                                                \
    /* so add 1 to unmask an extra lane  in final MAC computation  */                               \
    p0 = vctp32q(k+1);                                                                   \
    xVec = vld1q(pSrcX); pSrcX += 4;                                                    \
    acc0 = vmlaldavaq_p(acc0, xVec, yVec, p0);                                                      \
                                                                                                    \
    acc0 = asrl(acc0, 31);                                                                          \
    acc1 = asrl(acc1, 31);                                                                          \
}

#define MVE_INTR_CORR_DUAL_DEC_Y_INC_SIZE_Q15(acc0, acc1, pX, pY, count)                            \
{                                                                                                   \
    q15_t const *pSrcX, *pSrcY;                                                                     \
    q15x8_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q15_t const *) pX;                                                                     \
    pSrcY  = (q15_t const *) pY;                                                                    \
    k = count >> 3;                                                                                 \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        yVec = vldrhq_s16(&pSrcY[-1]);                                                              \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        yVec = vld1q(pSrcY);  pSrcY += 8;                                               \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    k = count % 0x8U;                                                                               \
    /* use predication to finalize MAC sum */                                                       \
    /* acc1 requires 1 additional sample  */                                                        \
    /* so add 1 to unmask an extra lane  in final MAC computation  */                               \
    mve_pred16_t p0 = vctp16q(k+1);                                                      \
    xVec = vld1q(pSrcX);  pSrcX += 8;                                                   \
    yVec = vldrhq_s16(&pSrcY[-1]);                                                                  \
    acc1 = vmlaldavaq_p(acc1, xVec, yVec,p0);                                                       \
    /* acc0 requires exact number of sample  */                                                     \
    /* disable extra lanes in final MAC computation  */                                             \
    p0 = vctp16q(k);                                                                     \
    yVec = vld1q(pSrcY);  pSrcY += 8;                                                   \
    acc0 = vmlaldavaq_p(acc0, xVec, yVec,p0);                                                       \
                                                                                                    \
    acc0 = asrl(acc0, 15);                                                                          \
    acc1 = asrl(acc1, 15);                                                                          \
    acc0 = __SSAT(acc0, 16);                                                                        \
    acc1 = __SSAT(acc1, 16);                                                                        \
}

#define MVE_INTR_CORR_SINGLE_Q15(acc, pX, pY, count)                                                \
{                                                                                                   \
    q15_t const *pSrcX, *pSrcY;                                                                     \
    q15x8_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q15_t const *) pX;                                                                     \
    pSrcY = (q15_t const *) pY;                                                                     \
    k = count >> 3;                                                                                 \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        yVec = vld1q(pSrcY);  pSrcY += 8;                                               \
        acc = vmlaldavaq(acc, xVec, yVec);                                                          \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /*  tail predication expected here  */                                                          \
    k = count % 0x8U;                                                                               \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp16q(k);                                                    \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        yVec = vld1q(pSrcY);  pSrcY += 8;                                               \
        acc = vmlaldavaq_p(acc, xVec, yVec, p0);                                                    \
    }                                                                                               \
    acc = asrl(acc, 15);                                                                            \
    acc = __SSAT(acc, 16);                                                                          \
}

#define MVE_INTR_CORR_QUAD_INC_X_FIXED_SIZE_Q15(acc0, acc1, acc2, acc3, pX, pY, count)              \
{                                                                                                   \
    q15_t const *pSrcX, *pSrcY;                                                                     \
    q15x8_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q15_t const *) pX;                                                                     \
    pSrcY  = (q15_t const *) pY;                                                                    \
    k = count >> 3;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        yVec = vld1q(pSrcY);  pSrcY += 8;                                               \
        xVec = vldrhq_s16(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        xVec = vldrhq_s16(&pSrcX[2]);                                                               \
        acc2 = vmlaldavaq(acc2, xVec, yVec);                                                        \
        xVec = vldrhq_s16(&pSrcX[3]);                                                               \
        acc3 = vmlaldavaq(acc3, xVec, yVec);                                                        \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* loop + tail predication expected here  */                                                    \
    k = count % 0x8U;                                                                               \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp16q(k);                                                    \
        yVec = vld1q(pSrcY);  pSrcY += 8;                                               \
        xVec = vldrhq_s16(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq_p(acc1, xVec, yVec, p0);                                                  \
        xVec = vldrhq_s16(&pSrcX[2]);                                                               \
        acc2 = vmlaldavaq_p(acc2, xVec, yVec, p0);                                                  \
        xVec = vldrhq_s16(&pSrcX[3]);                                                               \
        acc3 = vmlaldavaq_p(acc3, xVec, yVec, p0);                                                  \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        acc0 = vmlaldavaq_p(acc0, xVec, yVec, p0);                                                  \
    }                                                                                               \
                                                                                                    \
    acc0 = asrl(acc0, 15);                                                                          \
    acc1 = asrl(acc1, 15);                                                                          \
    acc2 = asrl(acc2, 15);                                                                          \
    acc3 = asrl(acc3, 15);                                                                          \
    acc0 = __SSAT(acc0, 16);                                                                        \
    acc1 = __SSAT(acc1, 16);                                                                        \
    acc2 = __SSAT(acc2, 16);                                                                        \
    acc3 = __SSAT(acc3, 16);                                                                        \
}

#define MVE_INTR_CORR_DUAL_INC_X_FIXED_SIZE_Q15(acc0, acc1, pX, pY, count)                          \
{                                                                                                   \
    q15_t const *pSrcX, *pSrcY;                                                                     \
    q15x8_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q15_t const *) pX;                                                                     \
    pSrcY  = (q15_t const *) pY;                                                                    \
    k = count >> 3;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        yVec = vld1q(pSrcY);  pSrcY += 8;                                               \
        xVec = vldrhq_s16(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* loop + tail predication expected here  */                                                    \
    k = count % 0x8U;                                                                               \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp16q(k);                                                    \
        yVec = vld1q(pSrcY);  pSrcY += 8;                                               \
        xVec = vldrhq_s16(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq_p(acc1, xVec, yVec, p0);                                                  \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        acc0 = vmlaldavaq_p(acc0, xVec, yVec, p0);                                                  \
    }                                                                                               \
                                                                                                    \
    acc0 = asrl(acc0, 15);                                                                          \
    acc1 = asrl(acc1, 15);                                                                          \
    acc0 = __SSAT(acc0, 16);                                                                        \
    acc1 = __SSAT(acc1, 16);                                                                        \
}

#define MVE_INTR_CORR_DUAL_INC_X_DEC_SIZE_Q15(acc0, acc1, pX, pY, count)                            \
{                                                                                                   \
    q15_t const *pSrcX, *pSrcY;                                                                     \
    q15x8_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q15_t const *) pX;                                                                     \
    pSrcY  = (q15_t const *) pY;                                                                    \
    k = (count-1) >> 3;                                                                             \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        yVec = vld1q(pSrcY);  pSrcY += 8;                                               \
        xVec = vldrhq_s16(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* use predication to finalize MAC sum */                                                       \
    /* acc1 requires exact number of sample (count-1)  */                                           \
    /* disable extra lanes in final MAC computation  */                                             \
    k = (count-1) % 0x8U;                                                                           \
    mve_pred16_t p0 = vctp16q(k);                                                        \
    yVec = vld1q(pSrcY);  pSrcY += 8;                                                   \
    xVec = vldrhq_s16(&pSrcX[1]);                                                                   \
    acc1 = vmlaldavaq_p(acc1, xVec, yVec, p0);                                                      \
    /* acc0 requires 1 additional sample  (count) */                                                \
    /* so add 1 to unmask an extra lane  in final MAC computation  */                               \
    p0 = vctp16q(k+1);                                                                   \
    xVec = vld1q(pSrcX);  pSrcX += 8;                                                   \
    acc0 = vmlaldavaq_p(acc0, xVec, yVec, p0);                                                      \
                                                                                                    \
    acc0 = asrl(acc0, 15);                                                                          \
    acc1 = asrl(acc1, 15);                                                                          \
    acc0 = __SSAT(acc0, 16);                                                                        \
    acc1 = __SSAT(acc1, 16);                                                                        \
}

#define MVE_INTR_CONV_DUAL_INC_Y_INC_SIZE_Q15(acc0, acc1, pX, pY, count)                            \
{                                                                                                   \
    q15_t const *pSrcX;                                                                             \
    const q15_t       *pY1 = pY + 1;                                                                      \
    q15x8_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q15_t const *) pX;                                                                     \
    k = count >> 3;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        yVec = vldrhq_gather_shifted_offset_s16(pY, decrIdxVec);                                    \
        pY-=8;                                                                                      \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        yVec = vldrhq_gather_shifted_offset_s16(pY1, decrIdxVec);                                   \
        pY1-=8;                                                                                     \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    k = count % 0x8U;                                                                               \
    /* use predication to finalize MAC sum */                                                       \
    /* acc0 requires exact number of sample  */                                                     \
    /* disable extra lanes in final MAC computation  */                                             \
    mve_pred16_t p0 = vctp16q(k);                                                        \
    xVec = vld1q(pSrcX);  pSrcX += 8;                                                   \
    yVec = vldrhq_gather_shifted_offset_s16(pY, decrIdxVec);                                        \
    acc0 = vmlaldavaq_p(acc0, xVec, yVec, p0);                                                      \
    yVec = vldrhq_gather_shifted_offset_s16(pY1, decrIdxVec);                                       \
    /* acc1 requires 1 additional sample  */                                                        \
    /* so add 1 to unmask an extra lane  in final MAC computation  */                               \
    p0 = vctp16q(k+1);                                                                   \
    acc1 = vmlaldavaq_p(acc1, xVec, yVec, p0);                                                      \
                                                                                                    \
    acc0 = asrl(acc0, 15);                                                                          \
    acc1 = asrl(acc1, 15);                                                                          \
    acc0 = __SSAT(acc0, 16);                                                                        \
    acc1 = __SSAT(acc1, 16);                                                                        \
}

#define MVE_INTR_CONV_SINGLE_Q15(acc, pX, pY, count)                                                \
{                                                                                                   \
    q15_t const *pSrcX;                                                                             \
    q15x8_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q15_t const *) pX;                                                                     \
    k = count >> 3;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        /* note */                                                                                  \
        /* could can be more efficient using Vector Scatter Store: */                               \
        /* + pre-increment + WB */                                                                  \
        /* To be revisited when intrinsic available */                                              \
        /* SDCOMP-52618 */                                                                          \
        yVec = vldrhq_gather_shifted_offset_s16(pY, decrIdxVec);                                    \
        pY-=8;                                                                                      \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        acc = vmlaldavaq(acc, xVec, yVec);                                                          \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* Loop with tail predication expected here  */                                                 \
    k = count % 0x8U;                                                                               \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp16q(k);                                                    \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        yVec = vldrhq_gather_shifted_offset_s16(pY, decrIdxVec);                                    \
        acc = vmlaldavaq_p(acc, xVec, yVec, p0);                                                    \
    }                                                                                               \
    acc = asrl(acc, 15);                                                                            \
    acc = __SSAT(acc, 16);                                                                          \
}

#define MVE_INTR_CONV_QUAD_INC_X_FIXED_SIZE_Q15(acc0, acc1, acc2, acc3, pX, pY, count)              \
{                                                                                                   \
    q15_t const *pSrcX;                                                                             \
    q15x8_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q15_t const *) pX;                                                                     \
    k = count >> 3;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        /* note */                                                                                  \
        /* could can be more efficient using Vector Scatter Store: */                               \
        /* + pre-increment + WB */                                                                  \
        /* To be revisited when intrinsic available */                                              \
        /* SDCOMP-52618 */                                                                          \
        yVec = vldrhq_gather_shifted_offset_s16(pY, decrIdxVec);                                    \
        pY-=8;                                                                                      \
        xVec = vldrhq_s16(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        xVec = vldrhq_s16(&pSrcX[2]);                                                               \
        acc2 = vmlaldavaq(acc2, xVec, yVec);                                                        \
        xVec = vldrhq_s16(&pSrcX[3]);                                                               \
        acc3 = vmlaldavaq(acc3, xVec, yVec);                                                        \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* Loop with tail predication expected here  */                                                 \
    k = count % 0x8U;                                                                               \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp16q(k);                                                    \
        yVec = vldrhq_gather_shifted_offset_s16(pY, decrIdxVec);                                    \
        xVec = vldrhq_s16(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq_p(acc1, xVec, yVec, p0);                                                  \
        xVec = vldrhq_s16(&pSrcX[2]);                                                               \
        acc2 = vmlaldavaq_p(acc2, xVec, yVec, p0);                                                  \
        xVec = vldrhq_s16(&pSrcX[3]);                                                               \
        acc3 = vmlaldavaq_p(acc3, xVec, yVec, p0);                                                  \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        acc0 = vmlaldavaq_p(acc0, xVec, yVec, p0);                                                  \
    }                                                                                               \
    acc0 = asrl(acc0, 15);                                                                          \
    acc1 = asrl(acc1, 15);                                                                          \
    acc2 = asrl(acc2, 15);                                                                          \
    acc3 = asrl(acc3, 15);                                                                          \
    acc0 = __SSAT(acc0, 16);                                                                        \
    acc1 = __SSAT(acc1, 16);                                                                        \
    acc2 = __SSAT(acc2, 16);                                                                        \
    acc3 = __SSAT(acc3, 16);                                                                        \
}

#define MVE_INTR_CONV_DUAL_INC_X_FIXED_SIZE_Q15(acc0, acc1, pX, pY, count)                          \
{                                                                                                   \
    q15_t const *pSrcX;                                                                             \
    q15x8_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q15_t const *) pX;                                                                     \
    k = count >> 3;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        /* note */                                                                                  \
        /* could can be more efficient using Vector Scatter Store: */                               \
        /* + pre-increment + WB */                                                                  \
        /* To be revisited when intrinsic available */                                              \
        /* SDCOMP-52618 */                                                                          \
        yVec = vldrhq_gather_shifted_offset_s16(pY, decrIdxVec);                                    \
        pY-=8;                                                                                      \
        xVec = vldrhq_s16(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* Loop with tail predication expected here  */                                                 \
    k = count % 0x8U;                                                                               \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp16q(k);                                                    \
        yVec = vldrhq_gather_shifted_offset_s16(pY, decrIdxVec);                                    \
        xVec = vldrhq_s16(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq_p(acc1, xVec, yVec, p0);                                                  \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        acc0 = vmlaldavaq_p(acc0, xVec, yVec, p0);                                                  \
    }                                                                                               \
    acc0 = asrl(acc0, 15);                                                                          \
    acc1 = asrl(acc1, 15);                                                                          \
    acc0 = __SSAT(acc0, 16);                                                                        \
    acc1 = __SSAT(acc1, 16);                                                                        \
}

#define MVE_INTR_CONV_DUAL_INC_X_DEC_SIZE_Q15(acc0, acc1, pX, pY, count)                            \
{                                                                                                   \
    q15_t const *pSrcX;                                                                             \
    q15x8_t   xVec, yVec;                                                                         \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q15_t const *) pX;                                                                     \
    k = (count-1) >> 3;                                                                             \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        /* note */                                                                                  \
        /* could can be more efficient using Vector Scatter Store: */                               \
        /* + pre-increment + WB */                                                                  \
        /* To be revisited when intrinsic available */                                              \
        /* SDCOMP-52618 */                                                                          \
        yVec = vldrhq_gather_shifted_offset_s16(pY, decrIdxVec);                                    \
        pY-=8;                                                                                      \
        xVec = vldrhq_s16(&pSrcX[1]);                                                               \
        acc1 = vmlaldavaq(acc1, xVec, yVec);                                                        \
        xVec = vld1q(pSrcX);  pSrcX += 8;                                               \
        acc0 = vmlaldavaq(acc0, xVec, yVec);                                                        \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    k = (count - 1) % 0x8U;                                                                         \
    /* use predication to finalize MAC sum */                                                       \
    /* acc1 requires exact number of sample (count-1)  */                                           \
    /* disable extra lanes in final MAC computation  */                                             \
    mve_pred16_t p0 = vctp16q(k);                                                        \
    yVec = vldrhq_gather_shifted_offset_s16(pY, decrIdxVec);                                        \
    xVec = vldrhq_s16(&pSrcX[1]);                                                                   \
    acc1 = vmlaldavaq_p(acc1, xVec, yVec, p0);                                                      \
    /* acc0 requires 1 additional sample  (count) */                                                \
    /* so add 1 to unmask an extra lane  in final MAC computation  */                               \
    p0 = vctp16q(k+1);                                                                   \
    xVec = vld1q(pSrcX);  pSrcX += 8;                                                   \
    acc0 = vmlaldavaq_p(acc0, xVec, yVec, p0);                                                      \
                                                                                                    \
    acc0 = asrl(acc0, 15);                                                                          \
    acc1 = asrl(acc1, 15);                                                                          \
    acc0 = __SSAT(acc0, 16);                                                                        \
    acc1 = __SSAT(acc1, 16);                                                                        \
}

#define MVE_INTR_CORR_DUAL_DEC_Y_INC_SIZE_Q7(acc0, acc1, pX, pY, count)                             \
{                                                                                                   \
    q7_t const *pSrcX, *pSrcY;                                                                      \
    q7x16_t   xVec, yVec;                                                                          \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q7_t const *) pX;                                                                      \
    pSrcY = (q7_t const *) pY;                                                                      \
    k = count >> 4;                                                                                 \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        yVec = vldrbq_s8(&pSrcY[-1]);                                                               \
        acc1 = vmladavaq(acc1, xVec, yVec);                                                         \
        yVec = vld1q(pSrcY);  pSrcY += 16;                                                \
        acc0 = vmladavaq(acc0, xVec, yVec);                                                         \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    k = count % 0x10U;                                                                              \
    /* use predication to finalize MAC sum */                                                       \
    /* acc1 requires 1 additional sample  */                                                        \
    /* so add 1 to unmask an extra lane  in final MAC computation  */                               \
    mve_pred16_t p0 = vctp8q(k+1);                                                       \
    xVec = vld1q(pSrcX);  pSrcX += 16;                                                    \
    yVec = vldrbq_s8(&pSrcY[-1]);                                                                   \
    acc1 = vmladavaq_p(acc1, xVec, yVec,p0);                                                        \
    /* acc0 requires exact number of sample  */                                                     \
    /* disable extra lanes in final MAC computation  */                                             \
    p0 = vctp8q(k);                                                                      \
    yVec = vld1q(pSrcY);  pSrcY += 16;                                                    \
    acc0 = vmladavaq_p(acc0, xVec, yVec,p0);                                                        \
                                                                                                    \
    acc0 = (acc0 >> 7);                                                                             \
    acc1 = (acc1 >> 7);                                                                             \
    acc0 = __SSAT(acc0, 8);                                                                         \
    acc1 = __SSAT(acc1, 8);                                                                         \
}

#define MVE_INTR_CORR_SINGLE_Q7(acc, pX, pY, count)                                                 \
{                                                                                                   \
    q7_t const *pSrcX, *pSrcY;                                                                      \
    q7x16_t   xVec, yVec;                                                                          \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q7_t const *) pX;                                                                      \
    pSrcY = (q7_t const *) pY;                                                                      \
    k = count >> 4;                                                                                 \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        yVec = vld1q(pSrcY);  pSrcY += 16;                                                \
        acc = vmladavaq(acc, xVec, yVec);                                                           \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /*  tail predication expected here  */                                                          \
    k = count % 0x10U;                                                                              \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp8q(k);                                                     \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        yVec = vld1q(pSrcY);  pSrcY += 16;                                                \
        acc = vmladavaq_p(acc, xVec, yVec, p0);                                                     \
    }                                                                                               \
    acc =(acc >> 7);                                                                                \
    acc = __SSAT(acc, 8);                                                                           \
}

#define MVE_INTR_CORR_QUAD_INC_X_FIXED_SIZE_Q7(acc0, acc1, acc2, acc3, pX, pY, count)               \
{                                                                                                   \
    q7_t const *pSrcX, *pSrcY;                                                                      \
    q7x16_t   xVec, yVec;                                                                          \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q7_t const *) pX;                                                                      \
    pSrcY = (q7_t const *) pY;                                                                      \
    k = count >> 4;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        yVec = vld1q(pSrcY);  pSrcY += 16;                                                \
        xVec = vldrbq_s8(&pSrcX[1]);                                                                \
        acc1 = vmladavaq(acc1, xVec, yVec);                                                         \
        xVec = vldrbq_s8(&pSrcX[2]);                                                                \
        acc2 = vmladavaq(acc2, xVec, yVec);                                                         \
        xVec = vldrbq_s8(&pSrcX[3]);                                                                \
        acc3 = vmladavaq(acc3, xVec, yVec);                                                         \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        acc0 = vmladavaq(acc0, xVec, yVec);                                                         \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* loop + tail predication expected here  */                                                    \
    k = count % 0x10U;                                                                              \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp8q(k);                                                     \
        yVec = vld1q(pSrcY);  pSrcY += 16;                                                \
        xVec = vldrbq_s8(&pSrcX[1]);                                                                \
        acc1 = vmladavaq_p(acc1, xVec, yVec, p0);                                                   \
        xVec = vldrbq_s8(&pSrcX[2]);                                                                \
        acc2 = vmladavaq_p(acc2, xVec, yVec, p0);                                                   \
        xVec = vldrbq_s8(&pSrcX[3]);                                                                \
        acc3 = vmladavaq_p(acc3, xVec, yVec, p0);                                                   \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        acc0 = vmladavaq_p(acc0, xVec, yVec, p0);                                                   \
    }                                                                                               \
                                                                                                    \
    acc0 = (acc0 >> 7);                                                                             \
    acc1 = (acc1 >> 7);                                                                             \
    acc2 = (acc2 >> 7);                                                                             \
    acc3 = (acc3 >> 7);                                                                             \
    acc0 = __SSAT(acc0, 8);                                                                         \
    acc1 = __SSAT(acc1, 8);                                                                         \
    acc2 = __SSAT(acc2, 8);                                                                         \
    acc3 = __SSAT(acc3, 8);                                                                         \
}

#define MVE_INTR_CORR_DUAL_INC_X_FIXED_SIZE_Q7(acc0, acc1, pX, pY, count)                           \
{                                                                                                   \
    q7_t const *pSrcX, *pSrcY;                                                                      \
    q7x16_t   xVec, yVec;                                                                          \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q7_t const *) pX;                                                                      \
    pSrcY = (q7_t const *) pY;                                                                      \
    k = count >> 4;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        yVec = vld1q(pSrcY);  pSrcY += 16;                                                \
        xVec = vldrbq_s8(&pSrcX[1]);                                                                \
        acc1 = vmladavaq(acc1, xVec, yVec);                                                         \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        acc0 = vmladavaq(acc0, xVec, yVec);                                                         \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* loop + tail predication expected here  */                                                    \
    k = count % 0x10U;                                                                              \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp8q(k);                                                     \
        yVec = vld1q(pSrcY);  pSrcY += 16;                                                \
        xVec = vldrbq_s8(&pSrcX[1]);                                                                \
        acc1 = vmladavaq_p(acc1, xVec, yVec, p0);                                                   \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        acc0 = vmladavaq_p(acc0, xVec, yVec, p0);                                                   \
    }                                                                                               \
                                                                                                    \
    acc0 = (acc0 >> 7);                                                                             \
    acc1 = (acc1 >> 7);                                                                             \
    acc0 = __SSAT(acc0, 8);                                                                         \
    acc1 = __SSAT(acc1, 8);                                                                         \
}

#define MVE_INTR_CORR_DUAL_INC_X_DEC_SIZE_Q7(acc0, acc1, pX, pY, count)                             \
{                                                                                                   \
    q7_t const *pSrcX, *pSrcY;                                                                      \
    q7x16_t   xVec, yVec;                                                                          \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q7_t const *) pX;                                                                      \
    pSrcY = (q7_t const *) pY;                                                                      \
    k = (count-1) >> 4;                                                                             \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        yVec = vld1q(pSrcY);  pSrcY += 16;                                                \
        xVec = vldrbq_s8(&pSrcX[1]);                                                                \
        acc1 = vmladavaq(acc1, xVec, yVec);                                                         \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        acc0 = vmladavaq(acc0, xVec, yVec);                                                         \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* use predication to finalize MAC sum */                                                       \
    /* acc1 requires exact number of sample (count-1)  */                                           \
    /* disable extra lanes in final MAC computation  */                                             \
    k = (count-1) % 0x10U;                                                                          \
    mve_pred16_t p0 = vctp8q(k);                                                         \
    yVec = vld1q(pSrcY);  pSrcY += 16;                                                    \
    xVec = vldrbq_s8(&pSrcX[1]);                                                                    \
    acc1 = vmladavaq_p(acc1, xVec, yVec, p0);                                                       \
    /* acc0 requires 1 additional sample  (count) */                                                \
    /* so add 1 to unmask an extra lane  in final MAC computation  */                               \
    p0 = vctp8q(k+1);                                                                    \
    xVec = vld1q(pSrcX);  pSrcX += 16;                                                    \
    acc0 = vmladavaq_p(acc0, xVec, yVec, p0);                                                       \
                                                                                                    \
    acc0 = (acc0 >> 7);                                                                             \
    acc1 = (acc1 >> 7);                                                                             \
    acc0 = __SSAT(acc0, 8);                                                                         \
    acc1 = __SSAT(acc1, 8);                                                                         \
}

#define MVE_INTR_CONV_DUAL_INC_Y_INC_SIZE_Q7(acc0, acc1, pX, pY, count)                             \
{                                                                                                   \
    q7_t const *pSrcX;                                                                              \
    const q7_t       *pY1 = pY + 1;                                                                       \
    q7x16_t   xVec, yVec;                                                                          \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q7_t const *) pX;                                                                      \
    k = count >> 4;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        yVec = vldrbq_gather_offset_s8(pY, decrIdxVec);                                             \
        pY-=16;                                                                                     \
        acc0 = vmladavaq(acc0, xVec, yVec);                                                         \
        yVec = vldrbq_gather_offset_s8(pY1, decrIdxVec);                                            \
        pY1-=16;                                                                                    \
        acc1 = vmladavaq(acc1, xVec, yVec);                                                         \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    k = count % 0x10U;                                                                              \
    /* use predication to finalize MAC sum */                                                       \
    /* acc0 requires exact number of sample  */                                                     \
    /* disable extra lanes in final MAC computation  */                                             \
    mve_pred16_t p0 = vctp8q(k);                                                         \
    xVec = vld1q(pSrcX);  pSrcX += 16;                                                    \
    yVec = vldrbq_gather_offset_s8(pY, decrIdxVec);                                                 \
    acc0 = vmladavaq_p(acc0, xVec, yVec, p0);                                                       \
    yVec = vldrbq_gather_offset_s8(pY1, decrIdxVec);                                                \
    /* acc1 requires 1 additional sample  */                                                        \
    /* so add 1 to unmask an extra lane  in final MAC computation  */                               \
    p0 = vctp8q(k+1);                                                                    \
    acc1 = vmladavaq_p(acc1, xVec, yVec, p0);                                                       \
                                                                                                    \
    acc0 = (acc0 >> 7);                                                                             \
    acc1 = (acc1 >> 7);                                                                             \
    acc0 = __SSAT(acc0, 8);                                                                         \
    acc1 = __SSAT(acc1, 8);                                                                         \
}

#define MVE_INTR_CONV_SINGLE_Q7(acc, pX, pY, count)                                                 \
{                                                                                                   \
    q7_t const *pSrcX;                                                                              \
    q7x16_t   xVec, yVec;                                                                          \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q7_t const *) pX;                                                                      \
    k = count >> 4;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        /* note */                                                                                  \
        /* could can be more efficient using Vector Scatter Store: */                               \
        /* + pre-increment + WB */                                                                  \
        /* To be revisited when intrinsic available */                                              \
        /* SDCOMP-52618 */                                                                          \
        yVec = vldrbq_gather_offset_s8(pY, decrIdxVec);                                             \
        pY-=16;                                                                                     \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        acc = vmladavaq(acc, xVec, yVec);                                                           \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* Loop with tail predication expected here  */                                                 \
    k = count % 0x10U;                                                                              \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp8q(k);                                                     \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        yVec = vldrbq_gather_offset_s8(pY, decrIdxVec);                                             \
        acc = vmladavaq_p(acc, xVec, yVec, p0);                                                     \
    }                                                                                               \
    acc = __SSAT(acc >> 7, 8);                                                                      \
}

#define MVE_INTR_CONV_QUAD_INC_X_FIXED_SIZE_Q7(acc0, acc1, acc2, acc3, pX, pY, count)               \
{                                                                                                   \
    q7_t const *pSrcX;                                                                              \
    q7x16_t   xVec, yVec;                                                                          \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q7_t const *) pX;                                                                      \
    k = count >> 4;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        /* note */                                                                                  \
        /* could can be more efficient using Vector Scatter Store: */                               \
        /* + pre-increment + WB */                                                                  \
        /* To be revisited when intrinsic available */                                              \
        /* SDCOMP-52618 */                                                                          \
        yVec = vldrbq_gather_offset_s8(pY, decrIdxVec);                                             \
        pY-=16;                                                                                     \
        xVec = vldrbq_s8(&pSrcX[1]);                                                                \
        acc1 = vmladavaq(acc1, xVec, yVec);                                                         \
        xVec = vldrbq_s8(&pSrcX[2]);                                                                \
        acc2 = vmladavaq(acc2, xVec, yVec);                                                         \
        xVec = vldrbq_s8(&pSrcX[3]);                                                                \
        acc3 = vmladavaq(acc3, xVec, yVec);                                                         \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        acc0 = vmladavaq(acc0, xVec, yVec);                                                         \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* Loop with tail predication expected here  */                                                 \
    k = count % 0x10U;                                                                              \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp8q(k);                                                     \
        yVec = vldrbq_gather_offset_s8(pY, decrIdxVec);                                             \
        xVec = vldrbq_s8(&pSrcX[1]);                                                                \
        acc1 = vmladavaq_p(acc1, xVec, yVec, p0);                                                   \
        xVec = vldrbq_s8(&pSrcX[2]);                                                                \
        acc2 = vmladavaq_p(acc2, xVec, yVec, p0);                                                   \
        xVec = vldrbq_s8(&pSrcX[3]);                                                                \
        acc3 = vmladavaq_p(acc3, xVec, yVec, p0);                                                   \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        acc0 = vmladavaq_p(acc0, xVec, yVec, p0);                                                   \
    }                                                                                               \
    acc0 = __SSAT(acc0 >> 7, 8);                                                                    \
    acc1 = __SSAT(acc1 >> 7, 8);                                                                    \
    acc2 = __SSAT(acc2 >> 7, 8);                                                                    \
    acc3 = __SSAT(acc3 >> 7, 8);                                                                    \
}

#define MVE_INTR_CONV_DUAL_INC_X_FIXED_SIZE_Q7(acc0, acc1, pX, pY, count)                           \
{                                                                                                   \
    q7_t const *pSrcX;                                                                              \
    q7x16_t   xVec, yVec;                                                                          \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q7_t const *) pX;                                                                      \
    k = count >> 4;                                                                                 \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        /* note */                                                                                  \
        /* could can be more efficient using Vector Scatter Store: */                               \
        /* + pre-increment + WB */                                                                  \
        /* To be revisited when intrinsic available */                                              \
        /* SDCOMP-52618 */                                                                          \
        yVec = vldrbq_gather_offset_s8(pY, decrIdxVec);                                             \
        pY-=16;                                                                                     \
        xVec = vldrbq_s8(&pSrcX[1]);                                                                \
        acc1 = vmladavaq(acc1, xVec, yVec);                                                         \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        acc0 = vmladavaq(acc0, xVec, yVec);                                                         \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    /* Loop with tail predication expected here  */                                                 \
    k = count % 0x10U;                                                                              \
    if (k > 0U)                                                                                     \
    {                                                                                               \
        mve_pred16_t p0 = vctp8q(k);                                                     \
        yVec = vldrbq_gather_offset_s8(pY, decrIdxVec);                                             \
        xVec = vldrbq_s8(&pSrcX[1]);                                                                \
        acc1 = vmladavaq_p(acc1, xVec, yVec, p0);                                                   \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        acc0 = vmladavaq_p(acc0, xVec, yVec, p0);                                                   \
    }                                                                                               \
    acc0 = __SSAT(acc0 >> 7, 8);                                                                    \
    acc1 = __SSAT(acc1 >> 7, 8);                                                                    \
}


#define MVE_INTR_CONV_DUAL_INC_X_DEC_SIZE_Q7(acc0, acc1, pX, pY, count)                             \
{                                                                                                   \
    q7_t const *pSrcX;                                                                              \
    q7x16_t   xVec, yVec;                                                                          \
    uint32_t    k;                                                                                  \
                                                                                                    \
    pSrcX = (q7_t const *) pX;                                                                      \
    k = (count-1) >> 4;                                                                             \
                                                                                                    \
    while (k > 0U)                                                                                  \
    {                                                                                               \
        /* note */                                                                                  \
        /* could can be more efficient using Vector Scatter Store: */                               \
        /* + pre-increment + WB */                                                                  \
        /* To be revisited when intrinsic available */                                              \
        /* SDCOMP-52618 */                                                                          \
        yVec = vldrbq_gather_offset_s8(pY, decrIdxVec);                                             \
        pY-=16;                                                                                     \
        xVec = vldrbq_s8(&pSrcX[1]);                                                                \
        acc1 = vmladavaq(acc1, xVec, yVec);                                                         \
        xVec = vld1q(pSrcX);  pSrcX += 16;                                                \
        acc0 = vmladavaq(acc0, xVec, yVec);                                                         \
        /*  Decrement the loop counter   */                                                         \
        k--;                                                                                        \
    }                                                                                               \
    k = (count - 1) % 0x10U;                                                                        \
    /* use predication to finalize MAC sum */                                                       \
    /* acc1 requires exact number of sample (count-1)  */                                           \
    /* disable extra lanes in final MAC computation  */                                             \
    mve_pred16_t p0 = vctp8q(k);                                                         \
    yVec = vldrbq_gather_offset_s8(pY, decrIdxVec);                                                 \
    xVec = vldrbq_s8(&pSrcX[1]);                                                                    \
    acc1 = vmladavaq_p(acc1, xVec, yVec, p0);                                                       \
    /* acc0 requires 1 additional sample  (count) */                                                \
    /* so add 1 to unmask an extra lane  in final MAC computation  */                               \
    p0 = vctp8q(k+1);                                                                    \
    xVec = vld1q(pSrcX);  pSrcX += 16;                                                    \
    acc0 = vmladavaq_p(acc0, xVec, yVec, p0);                                                       \
                                                                                                    \
    acc0 = (acc0 >> 7);                                                                             \
    acc1 = (acc1 >> 7);                                                                             \
    acc0 = __SSAT(acc0, 8);                                                                         \
    acc1 = __SSAT(acc1, 8);                                                                         \
}

#endif /* (defined(ARM_MATH_MVEI) || defined(ARM_MATH_HELIUM)) */

#ifdef   __cplusplus
}
#endif


#endif /* _ARM_VEC_FILTERING_H_ */
