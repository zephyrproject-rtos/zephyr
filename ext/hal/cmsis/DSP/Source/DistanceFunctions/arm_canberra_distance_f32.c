
/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_canberra_distance_f32.c
 * Description:  Canberra distance between two vectors
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
 * @brief        Canberra distance between two vectors
 *
 * This function may divide by zero when samples pA[i] and pB[i] are both zero.
 * The result of the computation will be correct. So the division per zero may be
 * ignored.
 *
 * @param[in]    pA         First vector
 * @param[in]    pB         Second vector
 * @param[in]    blockSize  vector length
 * @return distance
 *
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"
#include "arm_vec_math.h"

float32_t arm_canberra_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
    float32_t       accum = 0.0f;
    uint32_t         blkCnt;
    f32x4_t         a, b, c, accumV;

    accumV = vdupq_n_f32(0.0f);

    blkCnt = blockSize >> 2;
    while (blkCnt > 0) {
        a = vld1q(pA);
        b = vld1q(pB);

        c = vabdq(a, b);

        a = vabsq(a);
        b = vabsq(b);
        a = vaddq(a, b);

        /* 
         * May divide by zero when a and b have both the same lane at zero.
         */
        a = vrecip_medprec_f32(a);

        /*
         * Force result of a division by 0 to 0. It the behavior of the
         * sklearn canberra function.
         */
        a = vdupq_m_n_f32(a, 0.0f, vcmpeqq(a, 0.0f));
        c = vmulq(c, a);
        accumV = vaddq(accumV, c);

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

        a = vabsq(a);
        b = vabsq(b);
        a = vaddq(a, b);

        /* 
         * May divide by zero when a and b have both the same lane at zero.
         */
        a = vrecip_medprec_f32(a);

        /*
         * Force result of a division by 0 to 0. It the behavior of the
         * sklearn canberra function.
         */
        a = vdupq_m_n_f32(a, 0.0f, vcmpeqq(a, 0.0f));
        c = vmulq(c, a);
        accumV = vaddq_m(accumV, accumV, c, p0);
    }

    accum = vecAddAcrossF32Mve(accumV);

    return (accum);
}

#else
#if defined(ARM_MATH_NEON)

#include "NEMath.h"

float32_t arm_canberra_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
   float32_t accum=0.0f, tmpA, tmpB,diff,sum;
   uint32_t blkCnt;
   float32x4_t a,b,c,accumV;
   float32x2_t accumV2;
   uint32x4_t   isZeroV;
   float32x4_t zeroV = vdupq_n_f32(0.0f);

   accumV = vdupq_n_f32(0.0f);

   blkCnt = blockSize >> 2;
   while(blkCnt > 0)
   {
        a = vld1q_f32(pA);
        b = vld1q_f32(pB);

        c = vabdq_f32(a,b);

        a = vabsq_f32(a);
        b = vabsq_f32(b);
        a = vaddq_f32(a,b);
        isZeroV = vceqq_f32(a,zeroV);

        /* 
         * May divide by zero when a and b have both the same lane at zero.
         */
        a = vinvq_f32(a);
        
        /*
         * Force result of a division by 0 to 0. It the behavior of the
         * sklearn canberra function.
         */
        a = vreinterpretq_f32_s32(vbicq_s32(vreinterpretq_s32_f32(a),vreinterpretq_s32_u32(isZeroV)));
        c = vmulq_f32(c,a);
        accumV = vaddq_f32(accumV,c);

        pA += 4;
        pB += 4;
        blkCnt --;
   }
   accumV2 = vpadd_f32(vget_low_f32(accumV),vget_high_f32(accumV));
   accum = vget_lane_f32(accumV2, 0) + vget_lane_f32(accumV2, 1);


   blkCnt = blockSize & 3;
   while(blkCnt > 0)
   {
      tmpA = *pA++;
      tmpB = *pB++;

      diff = fabsf(tmpA - tmpB);
      sum = fabsf(tmpA) + fabsf(tmpB);
      if ((tmpA != 0.0f) || (tmpB != 0.0f))
      {
         accum += (diff / sum);
      }
      blkCnt --;
   }
   return(accum);
}

#else
float32_t arm_canberra_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
   float32_t accum=0.0f, tmpA, tmpB,diff,sum;

   while(blockSize > 0)
   {
      tmpA = *pA++;
      tmpB = *pB++;

      diff = fabsf(tmpA - tmpB);
      sum = fabsf(tmpA) + fabsf(tmpB);
      if ((tmpA != 0.0f) || (tmpB != 0.0f))
      {
         accum += (diff / sum);
      }
      blockSize --;
   }
   return(accum);
}
#endif
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */


/**
 * @} end of FloatDist group
 */
