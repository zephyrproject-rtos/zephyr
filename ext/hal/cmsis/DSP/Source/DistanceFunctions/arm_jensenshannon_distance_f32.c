
/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_jensenshannon_distance_f32.c
 * Description:  Jensen-Shannon distance between two vectors
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

#if !defined(ARM_MATH_MVEF) || defined(ARM_MATH_AUTOVECTORIZE)
__STATIC_INLINE float32_t rel_entr(float32_t x, float32_t y)
{
    return (x * logf(x / y));
}
#endif


#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"
#include "arm_vec_math.h"

float32_t arm_jensenshannon_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
    uint32_t        blkCnt;
    float32_t       tmp;
    f32x4_t         a, b, t, tmpV, accumV;

    accumV = vdupq_n_f32(0.0f);

    blkCnt = blockSize >> 2;
    while (blkCnt > 0U) {
        a = vld1q(pA);
        b = vld1q(pB);

        t = vaddq(a, b);
        t = vmulq(t, 0.5f);

        tmpV = vmulq(a, vrecip_medprec_f32(t));
        tmpV = vlogq_f32(tmpV);
        accumV = vfmaq(accumV, a, tmpV);

        tmpV = vmulq_f32(b, vrecip_medprec_f32(t));
        tmpV = vlogq_f32(tmpV);
        accumV = vfmaq(accumV, b, tmpV);

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

        t = vaddq(a, b);
        t = vmulq(t, 0.5f);

        tmpV = vmulq_f32(a, vrecip_medprec_f32(t));
        tmpV = vlogq_f32(tmpV);
        accumV = vfmaq_m_f32(accumV, a, tmpV, p0);

        tmpV = vmulq_f32(b, vrecip_medprec_f32(t));
        tmpV = vlogq_f32(tmpV);
        accumV = vfmaq_m_f32(accumV, b, tmpV, p0);

    }

    arm_sqrt_f32(vecAddAcrossF32Mve(accumV) / 2.0f, &tmp);
    return (tmp);
}

#else

#if defined(ARM_MATH_NEON)

#include "NEMath.h"


/**
 * @brief        Jensen-Shannon distance between two vectors
 *
 * This function is assuming that elements of second vector are > 0
 * and 0 only when the corresponding element of first vector is 0.
 * Otherwise the result of the computation does not make sense
 * and for speed reasons, the cases returning NaN or Infinity are not
 * managed.
 *
 * When the function is computing x log (x / y) with x == 0 and y == 0,
 * it will compute the right result (0) but a division by zero will occur
 * and should be ignored in client code.
 *
 * @param[in]    pA         First vector
 * @param[in]    pB         Second vector
 * @param[in]    blockSize  vector length
 * @return distance
 *
 */


float32_t arm_jensenshannon_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
    float32_t accum, result, tmp,a,b;
    uint32_t blkCnt;
    float32x4_t aV,bV,t, tmpV, accumV;
    float32x2_t accumV2;

    accum = 0.0f; 
    accumV = vdupq_n_f32(0.0f);

    blkCnt = blockSize >> 2;
    while(blkCnt > 0)
    {
      aV = vld1q_f32(pA);
      bV = vld1q_f32(pB);
      t = vaddq_f32(aV,bV);
      t = vmulq_n_f32(t, 0.5f);

      tmpV = vmulq_f32(aV, vinvq_f32(t));
      tmpV = vlogq_f32(tmpV);
      accumV = vmlaq_f32(accumV, aV, tmpV);


      tmpV = vmulq_f32(bV, vinvq_f32(t));
      tmpV = vlogq_f32(tmpV);
      accumV = vmlaq_f32(accumV, bV, tmpV);

      pA += 4;
      pB += 4;


      blkCnt --;
    }

    accumV2 = vpadd_f32(vget_low_f32(accumV),vget_high_f32(accumV));
    accum = vget_lane_f32(accumV2, 0) + vget_lane_f32(accumV2, 1);

    blkCnt = blockSize & 3;
    while(blkCnt > 0)
    {
      a = *pA;
      b = *pB;
      tmp = (a + b) / 2.0f;
      accum += rel_entr(a, tmp);
      accum += rel_entr(b, tmp);

      pA++;
      pB++;

      blkCnt --;
    }


    arm_sqrt_f32(accum/2.0f, &result);
    return(result);

}

#else


/**
 * @brief        Jensen-Shannon distance between two vectors
 *
 * This function is assuming that elements of second vector are > 0
 * and 0 only when the corresponding element of first vector is 0.
 * Otherwise the result of the computation does not make sense
 * and for speed reasons, the cases returning NaN or Infinity are not
 * managed.
 *
 * When the function is computing x log (x / y) with x == 0 and y == 0,
 * it will compute the right result (0) but a division by zero will occur
 * and should be ignored in client code.
 *
 * @param[in]    pA         First vector
 * @param[in]    pB         Second vector
 * @param[in]    blockSize  vector length
 * @return distance
 *
 */


float32_t arm_jensenshannon_distance_f32(const float32_t *pA,const float32_t *pB, uint32_t blockSize)
{
    float32_t left, right,sum, result, tmp;
    uint32_t i;

    left = 0.0f; 
    right = 0.0f;
    for(i=0; i < blockSize; i++)
    {
      tmp = (pA[i] + pB[i]) / 2.0f;
      left  += rel_entr(pA[i], tmp);
      right += rel_entr(pB[i], tmp);
    }


    sum = left + right;
    arm_sqrt_f32(sum/2.0f, &result);
    return(result);

}

#endif
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
 * @} end of FloatDist group
 */
