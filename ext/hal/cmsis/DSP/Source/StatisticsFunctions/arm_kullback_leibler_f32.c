/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_logsumexp_f32.c
 * Description:  LogSumExp
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
 * @addtogroup groupStats
 * @{
 */


/**
 * @brief Kullback-Leibler
 *
 * Distribution A may contain 0 with Neon version.
 * Result will be right but some exception flags will be set.
 *
 * Distribution B must not contain 0 probability.
 *
 * @param[in]  *pSrcA         points to an array of input values for probaility distribution A.
 * @param[in]  *pSrcB         points to an array of input values for probaility distribution B.
 * @param[in]  blockSize      number of samples in the input array.
 * @return Kullback-Leibler divergence D(A || B)
 *
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"
#include "arm_vec_math.h"

float32_t arm_kullback_leibler_f32(const float32_t * pSrcA,const float32_t * pSrcB,uint32_t blockSize)
{
    uint32_t blkCnt;
    float32_t accum, pA,pB;
 
    
    blkCnt = blockSize;

    accum = 0.0f;

    f32x4_t         vSum = vdupq_n_f32(0.0f);
    blkCnt = blockSize >> 2;
    while(blkCnt > 0)
    {
        f32x4_t         vecA = vld1q(pSrcA);
        f32x4_t         vecB = vld1q(pSrcB);
        f32x4_t         vRatio;

        vRatio = vdiv_f32(vecB, vecA);
        vSum = vaddq_f32(vSum, vmulq(vecA, vlogq_f32(vRatio)));

        /*
         * Decrement the blockSize loop counter
         * Advance vector source and destination pointers
         */
        pSrcA += 4;
        pSrcB += 4;
        blkCnt --;
    }

    accum = vecAddAcrossF32Mve(vSum);

    blkCnt = blockSize & 3;
    while(blkCnt > 0)
    {
       pA = *pSrcA++;
       pB = *pSrcB++;
       accum += pA * logf(pB / pA);
       
       blkCnt--;
    
    }

    return(-accum);
}

#else
#if defined(ARM_MATH_NEON) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "NEMath.h"

float32_t arm_kullback_leibler_f32(const float32_t * pSrcA,const float32_t * pSrcB,uint32_t blockSize)
{
    const float32_t *pInA, *pInB;
    uint32_t blkCnt;
    float32_t accum, pA,pB;

    float32x4_t accumV;
    float32x2_t accumV2;
    float32x4_t tmpVA, tmpVB,tmpV;
 
    pInA = pSrcA;
    pInB = pSrcB;

    accum = 0.0f;
    accumV = vdupq_n_f32(0.0f);

    blkCnt = blockSize >> 2;
    while(blkCnt > 0)
    {
      tmpVA = vld1q_f32(pInA);
      pInA += 4;

      tmpVB = vld1q_f32(pInB);
      pInB += 4;

      tmpV = vinvq_f32(tmpVA);
      tmpVB = vmulq_f32(tmpVB, tmpV);
      tmpVB = vlogq_f32(tmpVB);

      accumV = vmlaq_f32(accumV, tmpVA, tmpVB);
       
      blkCnt--;
    
    }

    accumV2 = vpadd_f32(vget_low_f32(accumV),vget_high_f32(accumV));
    accum = vget_lane_f32(accumV2, 0) + vget_lane_f32(accumV2, 1);

    blkCnt = blockSize & 3;
    while(blkCnt > 0)
    {
       pA = *pInA++;
       pB = *pInB++;
       accum += pA * logf(pB/pA);
       
       blkCnt--;
    
    }

    return(-accum);
}

#else
float32_t arm_kullback_leibler_f32(const float32_t * pSrcA,const float32_t * pSrcB,uint32_t blockSize)
{
    const float32_t *pInA, *pInB;
    uint32_t blkCnt;
    float32_t accum, pA,pB;
 
    pInA = pSrcA;
    pInB = pSrcB;
    blkCnt = blockSize;

    accum = 0.0f;

    while(blkCnt > 0)
    {
       pA = *pInA++;
       pB = *pInB++;
       accum += pA * logf(pB / pA);
       
       blkCnt--;
    
    }

    return(-accum);
}
#endif
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
 * @} end of groupStats group
 */
