/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_float_to_q7.c
 * Description:  Converts the elements of the floating-point vector to Q7 vector
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
 * @brief Converts the elements of the floating-point vector to Q7 vector.
 * @param[in]       *pSrc points to the floating-point input vector
 * @param[out]      *pDst points to the Q7 output vector
 * @param[in]       blockSize length of the input vector
 * @return none.
 *
 *\par Description:
 * \par
 * The equation used for the conversion process is:
 * <pre>
 * 	pDst[n] = (q7_t)(pSrc[n] * 128);   0 <= n < blockSize.
 * </pre>
 * \par Scaling and Overflow Behavior:
 * \par
 * The function uses saturating arithmetic.
 * Results outside of the allowable Q7 range [0x80 0x7F] will be saturated.
 * \note
 * In order to apply rounding, the library should be rebuilt with the ROUNDING macro
 * defined in the preprocessor section of project options.
 */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
void arm_float_to_q7(
  const float32_t * pSrc,
  q7_t * pDst,
  uint32_t blockSize)
{
    uint32_t         blkCnt;     /* loop counters */
    float32_t       maxQ = powf(2.0, 7);
    f32x4x4_t       tmp;
    q15x8_t         evVec, oddVec;
    q7x16_t         vecDst;
    float32_t const *pSrcVec;

    pSrcVec = (float32_t const *) pSrc;
    blkCnt = blockSize >> 4;
    while (blkCnt > 0U) {
        tmp = vld4q(pSrcVec);
        pSrcVec += 16;
        /*
         * C = A * 128.0
         * convert from float to q7 and then store the results in the destination buffer
         */
        tmp.val[0] = vmulq(tmp.val[0], maxQ);
        tmp.val[1] = vmulq(tmp.val[1], maxQ);
        tmp.val[2] = vmulq(tmp.val[2], maxQ);
        tmp.val[3] = vmulq(tmp.val[3], maxQ);

        /*
         * convert and pack evens
         */
        evVec = vqmovnbq(evVec, vcvtaq_s32_f32(tmp.val[0]));
        evVec = vqmovntq(evVec, vcvtaq_s32_f32(tmp.val[2]));
        /*
         * convert and pack odds
         */
        oddVec = vqmovnbq(oddVec, vcvtaq_s32_f32(tmp.val[1]));
        oddVec = vqmovntq(oddVec, vcvtaq_s32_f32(tmp.val[3]));
        /*
         * merge
         */
        vecDst = vqmovnbq(vecDst, evVec);
        vecDst = vqmovntq(vecDst, oddVec);

        vst1q(pDst, vecDst);
        pDst += 16;
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
    }

  blkCnt = blockSize & 0xF;
  while (blkCnt > 0U)
  {
    /* C = A * 128 */

    /* Convert from float to q7 and store result in destination buffer */
#ifdef ARM_MATH_ROUNDING

    in = (*pSrcVec++ * 128);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = (q7_t) (__SSAT((q15_t) (in), 8));

#else

    *pDst++ = (q7_t) __SSAT((q31_t) (*pSrcVec++ * 128.0f), 8);

#endif /* #ifdef ARM_MATH_ROUNDING */

    /* Decrement loop counter */
    blkCnt--;
  }

}
#else
#if defined(ARM_MATH_NEON)
void arm_float_to_q7(
  const float32_t * pSrc,
  q7_t * pDst,
  uint32_t blockSize)
{
  const float32_t *pIn = pSrc;                         /* Src pointer */
  uint32_t blkCnt;                               /* loop counter */

  float32x4_t inV;
  #ifdef ARM_MATH_ROUNDING
  float32_t in;
  float32x4_t zeroV = vdupq_n_f32(0.0f);
  float32x4_t pHalf = vdupq_n_f32(0.5f / 128.0f);
  float32x4_t mHalf = vdupq_n_f32(-0.5f / 128.0f);
  float32x4_t r;
  uint32x4_t cmp;
  #endif

  int16x4_t cvt1,cvt2;
  int8x8_t outV;

  blkCnt = blockSize >> 3U;

  /* Compute 8 outputs at a time.
   ** a second loop below computes the remaining 1 to 7 samples. */
  while (blkCnt > 0U)
  {

#ifdef ARM_MATH_ROUNDING
    /* C = A * 128 */
    /* Convert from float to q7 and then store the results in the destination buffer */
    inV = vld1q_f32(pIn);
    cmp = vcgtq_f32(inV,zeroV);
    r = vbslq_f32(cmp,pHalf,mHalf);
    inV = vaddq_f32(inV, r);
    cvt1 = vqmovn_s32(vcvtq_n_s32_f32(inV,7));
    pIn += 4;

    inV = vld1q_f32(pIn);
    cmp = vcgtq_f32(inV,zeroV);
    r = vbslq_f32(cmp,pHalf,mHalf);
    inV = vaddq_f32(inV, r);
    cvt2 = vqmovn_s32(vcvtq_n_s32_f32(inV,7));
    pIn += 4;
    
    outV = vqmovn_s16(vcombine_s16(cvt1,cvt2));
    vst1_s8(pDst, outV);
    pDst += 8;

#else

    /* C = A * 128 */
    /* Convert from float to q7 and then store the results in the destination buffer */
    inV = vld1q_f32(pIn);
    cvt1 = vqmovn_s32(vcvtq_n_s32_f32(inV,7));
    pIn += 4;

    inV = vld1q_f32(pIn);
    cvt2 = vqmovn_s32(vcvtq_n_s32_f32(inV,7));
    pIn += 4;

    outV = vqmovn_s16(vcombine_s16(cvt1,cvt2));

    vst1_s8(pDst, outV);
    pDst += 8;
#endif /*      #ifdef ARM_MATH_ROUNDING        */

    /* Decrement the loop counter */
    blkCnt--;
  }

  /* If the blockSize is not a multiple of 4, compute any remaining output samples here.
   ** No loop unrolling is used. */
  blkCnt = blockSize & 7;

  while (blkCnt > 0U)
  {

#ifdef ARM_MATH_ROUNDING
    /* C = A * 128 */
    /* Convert from float to q7 and then store the results in the destination buffer */
    in = *pIn++;
    in = (in * 128);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = (q7_t) (__SSAT((q15_t) (in), 8));

#else

    /* C = A * 128 */
    /* Convert from float to q7 and then store the results in the destination buffer */
    *pDst++ = __SSAT((q31_t) (*pIn++ * 128.0f), 8);

#endif /*      #ifdef ARM_MATH_ROUNDING        */

    /* Decrement the loop counter */
    blkCnt--;
  }

}
#else
void arm_float_to_q7(
  const float32_t * pSrc,
        q7_t * pDst,
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
    /* C = A * 128 */

    /* Convert from float to q7 and store result in destination buffer */
#ifdef ARM_MATH_ROUNDING

    in = (*pIn++ * 128);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = (q7_t) (__SSAT((q15_t) (in), 8));

    in = (*pIn++ * 128);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = (q7_t) (__SSAT((q15_t) (in), 8));

    in = (*pIn++ * 128);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = (q7_t) (__SSAT((q15_t) (in), 8));

    in = (*pIn++ * 128);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = (q7_t) (__SSAT((q15_t) (in), 8));

#else

    *pDst++ = __SSAT((q31_t) (*pIn++ * 128.0f), 8);
    *pDst++ = __SSAT((q31_t) (*pIn++ * 128.0f), 8);
    *pDst++ = __SSAT((q31_t) (*pIn++ * 128.0f), 8);
    *pDst++ = __SSAT((q31_t) (*pIn++ * 128.0f), 8);

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
    /* C = A * 128 */

    /* Convert from float to q7 and store result in destination buffer */
#ifdef ARM_MATH_ROUNDING

    in = (*pIn++ * 128);
    in += in > 0.0f ? 0.5f : -0.5f;
    *pDst++ = (q7_t) (__SSAT((q15_t) (in), 8));

#else

    *pDst++ = (q7_t) __SSAT((q31_t) (*pIn++ * 128.0f), 8);

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
