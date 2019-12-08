
/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_chebyshev_distance_f32.c
 * Description:  Chebyshev distance between two vectors
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
 * @brief        Chebyshev distance between two vectors
 * @param[in]    pA         First vector
 * @param[in]    pB         Second vector
 * @param[in]    blockSize  vector length
 * @return distance
 *
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"
#include "arm_vec_math.h"

float32_t arm_chebyshev_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
    uint32_t        blkCnt;     /* loop counters */
    f32x4_t         vecA, vecB;
    f32x4_t         vecDiff = vdupq_n_f32(0.0);
    float32_t       maxValue = 0.0;


    blkCnt = blockSize >> 2;
    while (blkCnt > 0U) {
        vecA = vld1q(pA);
        pA += 4;
        vecB = vld1q(pB);
        pB += 4;
        /*
         * update per-lane max.
         */
        vecDiff = vmaxnmaq(vsubq(vecA, vecB), vecDiff);
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
    }
    /*
     * tail
     * (will be merged thru tail predication)
     */
    blkCnt = blockSize & 3;
    if (blkCnt > 0U) {
        mve_pred16_t    p0 = vctp32q(blkCnt);

        vecA = vldrwq_z_f32(pA, p0);
        vecB = vldrwq_z_f32(pB, p0);

        /*
         * Get current max per lane and current index per lane
         * when a max is selected
         */
        vecDiff = vmaxnmaq_m(vecDiff, vsubq(vecA, vecB), p0);
    }
    /*
     * Get max value across the vector
     */
    return vmaxnmavq(maxValue, vecDiff);
}

#else
#if defined(ARM_MATH_NEON)

#include "NEMath.h"

float32_t arm_chebyshev_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
   float32_t diff=0.0f, maxVal=0.0f, tmpA, tmpB;
   uint32_t blkCnt;
   float32x4_t a,b,diffV, maxValV;
   float32x2_t maxValV2;

   if (blockSize <= 3)
   {
      tmpA = *pA++;
      tmpB = *pB++;
      diff = fabsf(tmpA - tmpB);
      maxVal = diff;
      blockSize--;
   
      while(blockSize > 0)
      {
         tmpA = *pA++;
         tmpB = *pB++;
         diff = fabsf(tmpA - tmpB);
         if (diff > maxVal)
         {
           maxVal = diff;
         }
         blockSize --;
      }
   }
   else
   {

      a = vld1q_f32(pA);
      b = vld1q_f32(pB);
      pA += 4;
      pB += 4;

      diffV = vabdq_f32(a,b);

      blockSize -= 4;

      maxValV = diffV;

  
      blkCnt = blockSize >> 2;
      while(blkCnt > 0)
      {
           a = vld1q_f32(pA);
           b = vld1q_f32(pB);
   
           diffV = vabdq_f32(a,b);
           maxValV = vmaxq_f32(maxValV, diffV);
   
           pA += 4;
           pB += 4;
           blkCnt --;
      }
      maxValV2 = vpmax_f32(vget_low_f32(maxValV),vget_high_f32(maxValV));
      maxValV2 = vpmax_f32(maxValV2,maxValV2);
      maxVal = vget_lane_f32(maxValV2,0);

  
      blkCnt = blockSize & 3;
      while(blkCnt > 0)
      {
         tmpA = *pA++;
         tmpB = *pB++;
         diff = fabsf(tmpA - tmpB);
         if (diff > maxVal)
         {
            maxVal = diff;
         }
         blkCnt --;
      }
   }
   return(maxVal);
}

#else
float32_t arm_chebyshev_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
   float32_t diff=0.0f,  maxVal,tmpA, tmpB;

   tmpA = *pA++;
   tmpB = *pB++;
   diff = fabsf(tmpA - tmpB);
   maxVal = diff;
   blockSize--;

   while(blockSize > 0)
   {
      tmpA = *pA++;
      tmpB = *pB++;
      diff = fabsf(tmpA - tmpB);
      if (diff > maxVal)
      {
        maxVal = diff;
      }
      blockSize --;
   }
  
   return(maxVal);
}
#endif
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */


/**
 * @} end of FloatDist group
 */
