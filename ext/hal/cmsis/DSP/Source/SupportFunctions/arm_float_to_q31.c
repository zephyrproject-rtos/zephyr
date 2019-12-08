/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_float_to_q31.c
 * Description:  Converts the elements of the floating-point vector to Q31 vector
 *
 * $Date:        18. March 2019
 * $Revision:    V1.6.0
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

/**
  @ingroup groupSupport
 */

/**
 * @defgroup float_to_x  Convert 32-bit floating point value
 */

/**
  @addtogroup float_to_x
  @{
 */

/**
  @brief         Converts the elements of the floating-point vector to Q31 vector.
  @param[in]     pSrc       points to the floating-point input vector
  @param[out]    pDst       points to the Q31 output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Details
                   The equation used for the conversion process is:
  <pre>
      pDst[n] = (q31_t)(pSrc[n] * 2147483648);   0 <= n < blockSize.
  </pre>

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   Results outside of the allowable Q31 range[0x80000000 0x7FFFFFFF] are saturated.

  @note
                   In order to apply rounding, the library should be rebuilt with the ROUNDING macro
                   defined in the preprocessor section of project options.
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
void arm_float_to_q31(
  const float32_t * pSrc,
  q31_t * pDst,
  uint32_t blockSize)
{
    uint32_t         blkCnt;
    float32_t       maxQ = (float32_t) Q31_MAX;
    f32x4_t         vecDst;


    blkCnt = blockSize >> 2U;

    /* Compute 4 outputs at a time. */
    while (blkCnt > 0U)
    {

        vecDst = vldrwq_f32(pSrc);
        /* C = A * 2147483648 */
        /* convert from float to Q31 and then store the results in the destination buffer */
        vecDst = vmulq(vecDst, maxQ);

        vstrwq_s32(pDst, vcvtaq_s32_f32(vecDst));
        /*
         * Decrement the blockSize loop counter
         * Advance vector source and destination pointers
         */
        pSrc += 4;
        pDst += 4;
        blkCnt --;
    }

    blkCnt = blockSize & 3;

    while (blkCnt > 0U)
    {
       /* C = A * 2147483648 */
   
       /* convert from float to Q31 and store result in destination buffer */
#ifdef ARM_MATH_ROUNDING

       in = (*pSrc++ * 2147483648.0f);
       in += in > 0.0f ? 0.5f : -0.5f;
       *pDst++ = clip_q63_to_q31((q63_t) (in));

#else

       /* C = A * 2147483648 */
       /* Convert from float to Q31 and then store the results in the destination buffer */
       *pDst++ = clip_q63_to_q31((q63_t) (*pSrc++ * 2147483648.0f));

#endif /* #ifdef ARM_MATH_ROUNDING */

       /* Decrement loop counter */
       blkCnt--;
  }
}
#else
#if defined(ARM_MATH_NEON)
void arm_float_to_q31(
  const float32_t * pSrc,
  q31_t * pDst,
  uint32_t blockSize)
{
  const float32_t *pIn = pSrc;                         /* Src pointer */
  uint32_t blkCnt;                               /* loop counter */

  float32x4_t inV;
  #ifdef ARM_MATH_ROUNDING
  float32_t in;
  float32x4_t zeroV = vdupq_n_f32(0.0f);
  float32x4_t pHalf = vdupq_n_f32(0.5f / 2147483648.0f);
  float32x4_t mHalf = vdupq_n_f32(-0.5f / 2147483648.0f);
  float32x4_t r;
  uint32x4_t cmp;
  #endif

  int32x4_t outV;

  blkCnt = blockSize >> 2U;

  /* Compute 4 outputs at a time.
   ** a second loop below computes the remaining 1 to 3 samples. */
  while (blkCnt > 0U)
  {

#ifdef ARM_MATH_ROUNDING

    /* C = A * 32768 */
    /* Convert from float to Q31 and then store the results in the destination buffer */
    inV = vld1q_f32(pIn);
    cmp = vcgtq_f32(inV,zeroV);
    r = vbslq_f32(cmp,pHalf,mHalf);
    inV = vaddq_f32(inV, r);

    pIn += 4;

    outV = vcvtq_n_s32_f32(inV,31);

    vst1q_s32(pDst, outV);
    pDst += 4;

#else

    /* C = A * 2147483648 */
    /* Convert from float to Q31 and then store the results in the destination buffer */
    inV = vld1q_f32(pIn);

    outV = vcvtq_n_s32_f32(inV,31);

    vst1q_s32(pDst, outV);
    pDst += 4;
    pIn += 4;

#endif /*      #ifdef ARM_MATH_ROUNDING        */

    /* Decrement the loop counter */
    blkCnt--;
  }

  /* If the blockSize is not a multiple of 4, compute any remaining output samples here.
   ** No loop unrolling is used. */
  blkCnt = blockSize & 3;

  while (blkCnt > 0U)
  {

#ifdef ARM_MATH_ROUNDING

    /* C = A * 2147483648 */
    /* Convert from float to Q31 and then store the results in the destination buffer */
    in = *pIn++;
    in = (in * 2147483648.0f);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = clip_q63_to_q31((q63_t) (in));

#else

    /* C = A * 2147483648 */
    /* Convert from float to Q31 and then store the results in the destination buffer */
    *pDst++ = clip_q63_to_q31((q63_t) (*pIn++ * 2147483648.0f));

#endif /*      #ifdef ARM_MATH_ROUNDING        */

    /* Decrement the loop counter */
    blkCnt--;
  }


}
#else
void arm_float_to_q31(
  const float32_t * pSrc,
        q31_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */
  const float32_t *pIn = pSrc;                         /* Source pointer */

#ifdef ARM_MATH_ROUNDING
        float32_t in;
#endif /* #ifdef ARM_MATH_ROUNDING */

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A * 2147483648 */

    /* convert from float to Q31 and store result in destination buffer */
#ifdef ARM_MATH_ROUNDING

    in = (*pIn++ * 2147483648.0f);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = clip_q63_to_q31((q63_t) (in));

    in = (*pIn++ * 2147483648.0f);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = clip_q63_to_q31((q63_t) (in));

    in = (*pIn++ * 2147483648.0f);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = clip_q63_to_q31((q63_t) (in));

    in = (*pIn++ * 2147483648.0f);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = clip_q63_to_q31((q63_t) (in));

#else

    /* C = A * 2147483648 */
    /* Convert from float to Q31 and then store the results in the destination buffer */
    *pDst++ = clip_q63_to_q31((q63_t) (*pIn++ * 2147483648.0f));
    *pDst++ = clip_q63_to_q31((q63_t) (*pIn++ * 2147483648.0f));
    *pDst++ = clip_q63_to_q31((q63_t) (*pIn++ * 2147483648.0f));
    *pDst++ = clip_q63_to_q31((q63_t) (*pIn++ * 2147483648.0f));

#endif /* #ifdef ARM_MATH_ROUNDING */

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = blockSize % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* C = A * 2147483648 */

    /* convert from float to Q31 and store result in destination buffer */
#ifdef ARM_MATH_ROUNDING

    in = (*pIn++ * 2147483648.0f);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = clip_q63_to_q31((q63_t) (in));

#else

    /* C = A * 2147483648 */
    /* Convert from float to Q31 and then store the results in the destination buffer */
    *pDst++ = clip_q63_to_q31((q63_t) (*pIn++ * 2147483648.0f));

#endif /* #ifdef ARM_MATH_ROUNDING */

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* #if defined(ARM_MATH_NEON) */
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of float_to_x group
 */
