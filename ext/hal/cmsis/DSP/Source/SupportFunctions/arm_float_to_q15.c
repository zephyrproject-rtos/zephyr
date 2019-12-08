/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_float_to_q15.c
 * Description:  Converts the elements of the floating-point vector to Q15 vector
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
  @addtogroup float_to_x
  @{
 */

/**
  @brief         Converts the elements of the floating-point vector to Q15 vector.
  @param[in]     pSrc       points to the floating-point input vector
  @param[out]    pDst       points to the Q15 output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Details
                   The equation used for the conversion process is:
  <pre>
      pDst[n] = (q15_t)(pSrc[n] * 32768);   0 <= n < blockSize.
  </pre>

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   Results outside of the allowable Q15 range [0x8000 0x7FFF] are saturated.

  @note
                   In order to apply rounding, the library should be rebuilt with the ROUNDING macro
                   defined in the preprocessor section of project options.
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
void arm_float_to_q15(
  const float32_t * pSrc,
  q15_t * pDst,
  uint32_t blockSize)
{
    uint32_t         blkCnt;
    float32_t       maxQ = (float32_t) Q15_MAX;
    f32x4x2_t       tmp;
    q15x8_t         vecDst;


    blkCnt = blockSize >> 3;
    while (blkCnt > 0U) 
    {
        /* C = A * 32768 */
        /* convert from float to q15 and then store the results in the destination buffer */
        tmp = vld2q(pSrc);

        tmp.val[0] = vmulq(tmp.val[0], maxQ);
        tmp.val[1] = vmulq(tmp.val[1], maxQ);

        vecDst = vqmovnbq(vecDst, vcvtaq_s32_f32(tmp.val[0]));
        vecDst = vqmovntq(vecDst, vcvtaq_s32_f32(tmp.val[1]));
        vst1q(pDst, vecDst);
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
        pDst += 8;
        pSrc += 8;
    }

    blkCnt = blockSize & 7;
    while (blkCnt > 0U)
    {
      /* C = A * 32768 */

      /* convert from float to Q15 and store result in destination buffer */
#ifdef ARM_MATH_ROUNDING

      in = (*pSrc++ * 32768.0f);
      in += in > 0.0f ? 0.5f : -0.5f;
      *pDst++ = (q15_t) (__SSAT((q31_t) (in), 16));

#else

      /* C = A * 32768 */
      /* Convert from float to q15 and then store the results in the destination buffer */
      *pDst++ = (q15_t) __SSAT((q31_t) (*pSrc++ * 32768.0f), 16);

#endif /* #ifdef ARM_MATH_ROUNDING */

      /* Decrement loop counter */
      blkCnt--;
    }
}

#else
#if defined(ARM_MATH_NEON_EXPERIMENTAL)
void arm_float_to_q15(
  const float32_t * pSrc,
  q15_t * pDst,
  uint32_t blockSize)
{
  const float32_t *pIn = pSrc;                         /* Src pointer */
  uint32_t blkCnt;                               /* loop counter */

  float32_t in;
  float32x4_t inV;
  #ifdef ARM_MATH_ROUNDING
  float32x4_t zeroV = vdupq_n_f32(0.0f);
  float32x4_t pHalf = vdupq_n_f32(0.5f / 32768.0f);
  float32x4_t mHalf = vdupq_n_f32(-0.5f / 32768.0f);
  float32x4_t r;
  uint32x4_t cmp;
  #endif

  int32x4_t cvt;
  int16x4_t outV;

  blkCnt = blockSize >> 2U;

  /* Compute 4 outputs at a time.
   ** a second loop below computes the remaining 1 to 3 samples. */
  while (blkCnt > 0U)
  {

#ifdef ARM_MATH_ROUNDING
    /* C = A * 32768 */
    /* Convert from float to q15 and then store the results in the destination buffer */
    inV = vld1q_f32(pIn);
    cmp = vcgtq_f32(inV,zeroV);
    r = vbslq_f32(cmp,pHalf,mHalf);
    inV = vaddq_f32(inV, r);

    pIn += 4;

    cvt = vcvtq_n_s32_f32(inV,15);
    outV = vqmovn_s32(cvt);

    vst1_s16(pDst, outV);
    pDst += 4;

#else

    /* C = A * 32768 */
    /* Convert from float to q15 and then store the results in the destination buffer */
    inV = vld1q_f32(pIn);

    cvt = vcvtq_n_s32_f32(inV,15);
    outV = vqmovn_s32(cvt);

    vst1_s16(pDst, outV);
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
    /* C = A * 32768 */
    /* Convert from float to q15 and then store the results in the destination buffer */
    in = *pIn++;
    in = (in * 32768.0f);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = (q15_t) (__SSAT((q31_t) (in), 16));

#else

    /* C = A * 32768 */
    /* Convert from float to q15 and then store the results in the destination buffer */
    *pDst++ = (q15_t) __SSAT((q31_t) (*pIn++ * 32768.0f), 16);

#endif /*      #ifdef ARM_MATH_ROUNDING        */

    /* Decrement the loop counter */
    blkCnt--;
  }
}
#else
void arm_float_to_q15(
  const float32_t * pSrc,
        q15_t * pDst,
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
    /* C = A * 32768 */

    /* convert from float to Q15 and store result in destination buffer */
#ifdef ARM_MATH_ROUNDING

    in = (*pIn++ * 32768.0f);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = (q15_t) (__SSAT((q31_t) (in), 16));

    in = (*pIn++ * 32768.0f);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = (q15_t) (__SSAT((q31_t) (in), 16));

    in = (*pIn++ * 32768.0f);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = (q15_t) (__SSAT((q31_t) (in), 16));

    in = (*pIn++ * 32768.0f);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = (q15_t) (__SSAT((q31_t) (in), 16));

#else

    *pDst++ = (q15_t) __SSAT((q31_t) (*pIn++ * 32768.0f), 16);
    *pDst++ = (q15_t) __SSAT((q31_t) (*pIn++ * 32768.0f), 16);
    *pDst++ = (q15_t) __SSAT((q31_t) (*pIn++ * 32768.0f), 16);
    *pDst++ = (q15_t) __SSAT((q31_t) (*pIn++ * 32768.0f), 16);

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
    /* C = A * 32768 */

    /* convert from float to Q15 and store result in destination buffer */
#ifdef ARM_MATH_ROUNDING

    in = (*pIn++ * 32768.0f);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = (q15_t) (__SSAT((q31_t) (in), 16));

#else

    /* C = A * 32768 */
    /* Convert from float to q15 and then store the results in the destination buffer */
    *pDst++ = (q15_t) __SSAT((q31_t) (*pIn++ * 32768.0f), 16);

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
