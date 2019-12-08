/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_max_no_idx_f32.c
 * Description:  Maximum value of a floating-point vector without returning the index
 *
 * $Date:        16. October 2019
 * $Revision:    V1.6.0
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
#if (defined(ARM_MATH_NEON) || defined(ARM_MATH_MVEF)) && !defined(ARM_MATH_AUTOVECTORIZE)
#include <limits.h>
#endif

/**
  @ingroup groupStats
 */


/**
  @addtogroup Max
  @{
 */

/**
  @brief         Maximum value of a floating-point vector.
  @param[in]     pSrc       points to the input vector
  @param[in]     blockSize  number of samples in input vector
  @param[out]    pResult    maximum value returned here
  @return        none
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

void arm_max_no_idx_f32(
    const float32_t *pSrc,
    uint32_t   blockSize,
    float32_t *pResult)
{
   f32x4_t     vecSrc;
   f32x4_t     curExtremValVec = vdupq_n_f32(F32_MIN);
   float32_t   maxValue = F32_MIN;
   float32_t   newVal;
   uint32_t    blkCnt;

   /* Loop unrolling: Compute 4 outputs at a time */
   blkCnt = blockSize >> 2U;

   while (blkCnt > 0U)
   {

        vecSrc = vldrwq_f32(pSrc);
        /*
         * update per-lane max.
         */
        curExtremValVec = vmaxnmq(vecSrc, curExtremValVec);
        /*
         * Decrement the blockSize loop counter
         * Advance vector source and destination pointers
         */
        pSrc += 4;
        blkCnt --;
    }
    /*
     * Get max value across the vector
     */
    maxValue = vmaxnmvq(maxValue, curExtremValVec);

    blkCnt = blockSize & 3;

    while (blkCnt > 0U)
    {
        newVal = *pSrc++;

        /* compare for the maximum value */
        if (maxValue < newVal)
        {
            /* Update the maximum value and it's index */
            maxValue = newVal;
        }

        blkCnt --;
    }

    *pResult = maxValue;
}

#else

void arm_max_no_idx_f32(
    const float32_t *pSrc,
    uint32_t   blockSize,
    float32_t *pResult)
{
   float32_t   maxValue = F32_MIN;
   float32_t   newVal;

   while (blockSize > 0U)
   {
       newVal = *pSrc++;
   
       /* compare for the maximum value */
       if (maxValue < newVal)
       {
           /* Update the maximum value and it's index */
           maxValue = newVal;
       }
   
       blockSize --;
   }
    
   *pResult = maxValue;
}

#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of Max group
 */
