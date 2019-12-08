
/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_braycurtis_distance_f32.c
 * Description:  Bray-Curtis distance between two vectors
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
 * @ingroup groupDistance
 * @{
 */

/**
 * @defgroup FloatDist Float Distances
 *
 * Distances between two vectors of float values.
 */

/**
  @addtogroup FloatDist
  @{
 */


/**
 * @brief        Bray-Curtis distance between two vectors
 * @param[in]    pA         First vector
 * @param[in]    pB         Second vector
 * @param[in]    blockSize  vector length
 * @return distance
 *
 */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"

float32_t arm_braycurtis_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
    float32_t       accumDiff = 0.0f, accumSum = 0.0f;
    uint32_t        blkCnt;
    f32x4_t         a, b, c, accumDiffV, accumSumV;


    accumDiffV = vdupq_n_f32(0.0f);
    accumSumV = vdupq_n_f32(0.0f);

    blkCnt = blockSize >> 2;
    while (blkCnt > 0) {
        a = vld1q(pA);
        b = vld1q(pB);

        c = vabdq(a, b);
        accumDiffV = vaddq(accumDiffV, c);

        c = vaddq_f32(a, b);
        c = vabsq_f32(c);
        accumSumV = vaddq(accumSumV, c);

        pA += 4;
        pB += 4;
        blkCnt--;
    }

    blkCnt = blockSize & 3;
    if (blkCnt > 0U) {
        mve_pred16_t    p0 = vctp32q(blkCnt);

        a = vldrwq_z_f32(pA, p0);
        b = vldrwq_z_f32(pB, p0);

        c = vabdq(a, b);
        accumDiffV = vaddq_m(accumDiffV, accumDiffV, c, p0);

        c = vaddq_f32(a, b);
        c = vabsq_f32(c);
        accumSumV = vaddq_m(accumSumV, accumSumV, c, p0);
    }

    accumDiff = vecAddAcrossF32Mve(accumDiffV);
    accumSum = vecAddAcrossF32Mve(accumSumV);

    /*
       It is assumed that accumSum is not zero. Since it is the sum of several absolute
       values it would imply that all of them are zero. It is very unlikely for long vectors.
     */
    return (accumDiff / accumSum);
}
#else
#if defined(ARM_MATH_NEON)

#include "NEMath.h"

float32_t arm_braycurtis_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
   float32_t accumDiff=0.0f, accumSum=0.0f;
   uint32_t blkCnt;
   float32x4_t a,b,c,accumDiffV, accumSumV;
   float32x2_t accumV2;

   accumDiffV = vdupq_n_f32(0.0f);
   accumSumV = vdupq_n_f32(0.0f);

   blkCnt = blockSize >> 2;
   while(blkCnt > 0)
   {
        a = vld1q_f32(pA);
        b = vld1q_f32(pB);

        c = vabdq_f32(a,b);
        accumDiffV = vaddq_f32(accumDiffV,c);

        c = vaddq_f32(a,b);
        c = vabsq_f32(c);
        accumSumV = vaddq_f32(accumSumV,c);

        pA += 4;
        pB += 4;
        blkCnt --;
   }
   accumV2 = vpadd_f32(vget_low_f32(accumDiffV),vget_high_f32(accumDiffV));
   accumDiff = vget_lane_f32(accumV2, 0) + vget_lane_f32(accumV2, 1);

   accumV2 = vpadd_f32(vget_low_f32(accumSumV),vget_high_f32(accumSumV));
   accumSum = vget_lane_f32(accumV2, 0) + vget_lane_f32(accumV2, 1);

   blkCnt = blockSize & 3;
   while(blkCnt > 0)
   {
      accumDiff += fabsf(*pA - *pB);
      accumSum += fabsf(*pA++ + *pB++);
      blkCnt --;
   }
   /*

   It is assumed that accumSum is not zero. Since it is the sum of several absolute
   values it would imply that all of them are zero. It is very unlikely for long vectors.

   */
   return(accumDiff / accumSum);
}

#else
float32_t arm_braycurtis_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
   float32_t accumDiff=0.0f, accumSum=0.0f, tmpA, tmpB;

   while(blockSize > 0)
   {
      tmpA = *pA++;
      tmpB = *pB++;
      accumDiff += fabsf(tmpA - tmpB);
      accumSum += fabsf(tmpA + tmpB);
      blockSize --;
   }
   /*

   It is assumed that accumSum is not zero. Since it is the sum of several absolute
   values it would imply that all of them are zero. It is very unlikely for long vectors.

   */
   return(accumDiff / accumSum);
}
#endif
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */


/**
 * @} end of FloatDist group
 */

/**
 * @} end of groupDistance group
 */
