
/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_minkowski_distance_f32.c
 * Description:  Minkowski distance between two vectors
 *
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
#include <limits.h>
#include <math.h>


/**
  @addtogroup FloatDist
  @{
 */


/**
 * @brief        Minkowski distance between two vectors
 *
 * @param[in]    pA         First vector
 * @param[in]    pB         Second vector
 * @param[in]    order      Distance order
 * @param[in]    blockSize  Number of samples
 * @return distance
 *
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"
#include "arm_vec_math.h"

float32_t arm_minkowski_distance_f32(const float32_t *pA,const float32_t *pB, int32_t order, uint32_t blockSize)
{
    uint32_t        blkCnt;
    f32x4_t         a, b, tmpV, accumV, sumV;

    sumV = vdupq_n_f32(0.0f);
    accumV = vdupq_n_f32(0.0f);

    blkCnt = blockSize >> 2;
    while (blkCnt > 0U) {
        a = vld1q(pA);
        b = vld1q(pB);

        tmpV = vabdq(a, b);
        tmpV = vpowq_f32(tmpV, vdupq_n_f32(order));
        sumV = vaddq(sumV, tmpV);

        pA += 4;
        pB += 4;
        blkCnt--;
    }

    /*
     * tail
     * (will be merged thru tail predication)
     */
    blkCnt = blockSize & 3;
    if (blkCnt > 0U) {
        mve_pred16_t    p0 = vctp32q(blkCnt);

        a = vldrwq_z_f32(pA, p0);
        b = vldrwq_z_f32(pB, p0);

        tmpV = vabdq(a, b);
        tmpV = vpowq_f32(tmpV, vdupq_n_f32(order));
        sumV = vaddq_m(sumV, sumV, tmpV, p0);
    }

    return (powf(vecAddAcrossF32Mve(sumV), (1.0f / (float32_t) order)));
}

#else
#if defined(ARM_MATH_NEON)

#include "NEMath.h"

float32_t arm_minkowski_distance_f32(const float32_t *pA,const float32_t *pB, int32_t order, uint32_t blockSize)
{
    float32_t sum;
    uint32_t blkCnt;
    float32x4_t sumV,aV,bV, tmpV, n;
    float32x2_t sumV2;

    sum = 0.0f; 
    sumV = vdupq_n_f32(0.0f);
    n = vdupq_n_f32(order);

    blkCnt = blockSize >> 2;
    while(blkCnt > 0)
    {
       aV = vld1q_f32(pA);
       bV = vld1q_f32(pB);
       pA += 4;
       pB += 4;

       tmpV = vabdq_f32(aV,bV);
       tmpV = vpowq_f32(tmpV,n);
       sumV = vaddq_f32(sumV, tmpV);


       blkCnt --;
    }

    sumV2 = vpadd_f32(vget_low_f32(sumV),vget_high_f32(sumV));
    sum = vget_lane_f32(sumV2, 0) + vget_lane_f32(sumV2, 1);

    blkCnt = blockSize & 3;
    while(blkCnt > 0)
    {
       sum += powf(fabsf(*pA++ - *pB++),order);

       blkCnt --;
    }


    return(powf(sum,(1.0f/order)));

}

#else


float32_t arm_minkowski_distance_f32(const float32_t *pA,const float32_t *pB, int32_t order, uint32_t blockSize)
{
    float32_t sum;
    uint32_t i;

    sum = 0.0f; 
    for(i=0; i < blockSize; i++)
    {
       sum += powf(fabsf(pA[i] - pB[i]),order);
    }


    return(powf(sum,(1.0f/order)));

}
#endif
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */


/**
 * @} end of FloatDist group
 */
