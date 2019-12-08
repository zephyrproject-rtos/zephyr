/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_weighted_sum_f32.c
 * Description:  Weighted Sum
 *
 *
 * Target Processor: Cortex-M and Cortex-A cores
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
#include <limits.h>
#include <math.h>


/**
 * @addtogroup groupSupport
 * @{
 */


/**
 * @brief Weighted sum
 *
 *
 * @param[in]    *in           Array of input values.
 * @param[in]    *weigths      Weights
 * @param[in]    blockSize     Number of samples in the input array.
 * @return       Weighted sum
 *
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"

float32_t arm_weighted_sum_f32(const float32_t *in,const float32_t *weigths, uint32_t blockSize)
{
    float32_t       accum1, accum2;
    f32x4_t         accum1V, accum2V;
    f32x4_t         inV, wV;
    const float32_t *pIn, *pW;
    uint32_t        blkCnt;


    pIn = in;
    pW = weigths;


    accum1V = vdupq_n_f32(0.0);
    accum2V = vdupq_n_f32(0.0);

    blkCnt = blockSize >> 2;
    while (blkCnt > 0) 
    {
        inV = vld1q(pIn);
        wV = vld1q(pW);

        pIn += 4;
        pW += 4;

        accum1V = vfmaq(accum1V, inV, wV);
        accum2V = vaddq(accum2V, wV);
        blkCnt--;
    }

    accum1 = vecAddAcrossF32Mve(accum1V);
    accum2 = vecAddAcrossF32Mve(accum2V);

    blkCnt = blockSize & 3;
    while(blkCnt > 0)
    {
        accum1 += *pIn++ * *pW;
        accum2 += *pW++;
        blkCnt--;
    }


    return (accum1 / accum2);
}

#else
#if defined(ARM_MATH_NEON)

#include "NEMath.h"
float32_t arm_weighted_sum_f32(const float32_t *in,const float32_t *weigths, uint32_t blockSize)
{

    float32_t accum1, accum2;
    float32x4_t accum1V, accum2V;
    float32x2_t tempV;

    float32x4_t inV,wV;

    const float32_t *pIn, *pW;
    uint32_t blkCnt;


    pIn = in;
    pW = weigths;

    accum1=0.0f;
    accum2=0.0f;

    accum1V = vdupq_n_f32(0.0f);
    accum2V = vdupq_n_f32(0.0f);

    blkCnt = blockSize >> 2;
    while(blkCnt > 0)
    {
        inV = vld1q_f32(pIn);
        wV = vld1q_f32(pW);

        pIn += 4; 
        pW += 4;

        accum1V = vmlaq_f32(accum1V,inV,wV);
        accum2V = vaddq_f32(accum2V,wV);
        blkCnt--;
    }

    tempV = vpadd_f32(vget_low_f32(accum1V),vget_high_f32(accum1V));
    accum1 = vget_lane_f32(tempV, 0) + vget_lane_f32(tempV, 1);

    tempV = vpadd_f32(vget_low_f32(accum2V),vget_high_f32(accum2V));
    accum2 = vget_lane_f32(tempV, 0) + vget_lane_f32(tempV, 1);

    blkCnt = blockSize & 3;
    while(blkCnt > 0)
    {
        accum1 += *pIn++ * *pW;
        accum2 += *pW++;
        blkCnt--;
    }


    return(accum1 / accum2);
}
#else
float32_t arm_weighted_sum_f32(const float32_t *in, const float32_t *weigths, uint32_t blockSize)
{

    float32_t accum1, accum2;
    const float32_t *pIn, *pW;
    uint32_t blkCnt;


    pIn = in;
    pW = weigths;

    accum1=0.0f;
    accum2=0.0f;

    blkCnt = blockSize;
    while(blkCnt > 0)
    {
        accum1 += *pIn++ * *pW;
        accum2 += *pW++;
        blkCnt--;
    }

    return(accum1 / accum2);
}
#endif
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
 * @} end of groupSupport group
 */
