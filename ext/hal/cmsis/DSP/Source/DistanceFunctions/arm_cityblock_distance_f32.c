
/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cityblock_distance_f32.c
 * Description:  Cityblock (Manhattan) distance between two vectors
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
 * @brief        Cityblock (Manhattan) distance between two vectors
 * @param[in]    pA         First vector
 * @param[in]    pB         Second vector
 * @param[in]    blockSize  vector length
 * @return distance
 *
 */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"
#include "arm_vec_math.h"

float32_t arm_cityblock_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
    uint32_t        blkCnt;
    f32x4_t         a, b, accumV, tempV;

    accumV = vdupq_n_f32(0.0f);

    blkCnt = blockSize >> 2;
    while (blkCnt > 0U) {
        a = vld1q(pA);
        b = vld1q(pB);

        tempV = vabdq(a, b);
        accumV = vaddq(accumV, tempV);

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

        tempV = vabdq(a, b);
        accumV = vaddq_m(accumV, accumV, tempV, p0);
    }

    return vecAddAcrossF32Mve(accumV);
}

#else
#if defined(ARM_MATH_NEON)

#include "NEMath.h"

float32_t arm_cityblock_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
   float32_t accum=0.0f, tmpA, tmpB;
   uint32_t blkCnt;
   float32x4_t a,b,accumV, tempV;
   float32x2_t accumV2;

   accumV = vdupq_n_f32(0.0f);

   blkCnt = blockSize >> 2;
   while(blkCnt > 0)
   {
        a = vld1q_f32(pA);
        b = vld1q_f32(pB);
 
        tempV = vabdq_f32(a,b);
        accumV = vaddq_f32(accumV, tempV);
 
        pA += 4;
        pB += 4;
        blkCnt --;
   }
   accumV2 = vpadd_f32(vget_low_f32(accumV),vget_high_f32(accumV));
   accumV2 = vpadd_f32(accumV2,accumV2);
   accum = vget_lane_f32(accumV2,0);
   

   blkCnt = blockSize & 3;
   while(blkCnt > 0)
   {
      tmpA = *pA++;
      tmpB = *pB++;
      accum += fabsf(tmpA - tmpB);
      
      blkCnt --;
   }
   return(accum);
}

#else
float32_t arm_cityblock_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
   float32_t accum,tmpA, tmpB;

   accum = 0.0f;
   while(blockSize > 0)
   {
      tmpA = *pA++;
      tmpB = *pB++;
      accum  += fabsf(tmpA - tmpB);
      
      blockSize --;
   }
  
   return(accum);
}
#endif
#endif

/**
 * @} end of FloatDist group
 */
