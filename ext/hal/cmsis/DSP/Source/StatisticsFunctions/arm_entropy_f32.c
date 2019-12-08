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
 * @brief Entropy
 *
 * @param[in]  pSrcA        Array of input values.
 * @param[in]  blockSize    Number of samples in the input array.
 * @return     Entropy      -Sum(p ln p)
 *
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"
#include "arm_vec_math.h"

float32_t arm_entropy_f32(const float32_t * pSrcA,uint32_t blockSize)
{
    uint32_t        blkCnt;
    float32_t       accum=0.0f,p;


    blkCnt = blockSize;

    f32x4_t         vSum = vdupq_n_f32(0.0f);
    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2U;

    while (blkCnt > 0U)
    {
        f32x4_t         vecIn = vld1q(pSrcA);

        vSum = vaddq_f32(vSum, vmulq(vecIn, vlogq_f32(vecIn)));

        /*
         * Decrement the blockSize loop counter
         * Advance vector source and destination pointers
         */
        pSrcA += 4;
        blkCnt --;
    }

    accum = vecAddAcrossF32Mve(vSum);

    /* Tail */
    blkCnt = blockSize & 0x3;
    while(blkCnt > 0)
    {
       p = *pSrcA++;
       accum += p * logf(p);
       
       blkCnt--;
    
    }

    return (-accum);
}

#else
#if defined(ARM_MATH_NEON) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "NEMath.h"

float32_t arm_entropy_f32(const float32_t * pSrcA,uint32_t blockSize)
{
    const float32_t *pIn;
    uint32_t blkCnt;
    float32_t accum, p;

    float32x4_t accumV;
    float32x2_t accumV2;
    float32x4_t tmpV, tmpV2;
 
    pIn = pSrcA;

    accum = 0.0f;
    accumV = vdupq_n_f32(0.0f);

    blkCnt = blockSize >> 2;
    while(blkCnt > 0)
    {
      tmpV = vld1q_f32(pIn);
      pIn += 4;

      tmpV2 = vlogq_f32(tmpV);
      accumV = vmlaq_f32(accumV, tmpV, tmpV2);
       
      blkCnt--;
    
    }

    accumV2 = vpadd_f32(vget_low_f32(accumV),vget_high_f32(accumV));
    accum = vget_lane_f32(accumV2, 0) + vget_lane_f32(accumV2, 1);
    

    blkCnt = blockSize & 3;
    while(blkCnt > 0)
    {
       p = *pIn++;
       accum += p * logf(p);
       
       blkCnt--;
    
    }

    return(-accum);
}

#else
float32_t arm_entropy_f32(const float32_t * pSrcA,uint32_t blockSize)
{
    const float32_t *pIn;
    uint32_t blkCnt;
    float32_t accum, p;
 
    pIn = pSrcA;
    blkCnt = blockSize;

    accum = 0.0f;

    while(blkCnt > 0)
    {
       p = *pIn++;
       accum += p * logf(p);
       
       blkCnt--;
    
    }

    return(-accum);
}
#endif
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
 * @} end of groupStats group
 */
