/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cmplx_mult_real_q31.c
 * Description:  Q31 complex by real multiplication
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
  @ingroup groupCmplxMath
 */

/**
  @addtogroup CmplxByRealMult
  @{
 */

/**
  @brief         Q31 complex-by-real multiplication.
  @param[in]     pSrcCmplx   points to complex input vector
  @param[in]     pSrcReal    points to real input vector
  @param[out]    pCmplxDst   points to complex output vector
  @param[in]     numSamples  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   Results outside of the allowable Q31 range[0x80000000 0x7FFFFFFF] are saturated.
 */

#if defined(ARM_MATH_MVEI)
void arm_cmplx_mult_real_q31(
  const q31_t * pSrcCmplx,
  const q31_t * pSrcReal,
        q31_t * pCmplxDst,
        uint32_t numSamples)
{

    const static uint32_t stride_cmplx_x_real_32[4] = {
        0, 0, 1, 1
    };
    q31x4_t rVec;
    q31x4_t cmplxVec;
    q31x4_t dstVec;
    uint32x4_t strideVec;
    uint32_t blockSizeC = numSamples * CMPLX_DIM;   /* loop counters */
    uint32_t blkCnt;
    q31_t in;

    /*
     * stride vector for pairs of real generation
     */
    strideVec = vld1q(stride_cmplx_x_real_32);

    /* Compute 4 complex outputs at a time */
    blkCnt = blockSizeC >> 2;
    while (blkCnt > 0U) 
    {
        cmplxVec = vld1q(pSrcCmplx);
        rVec = vldrwq_gather_shifted_offset_s32(pSrcReal, strideVec);
        dstVec = vqdmulhq(cmplxVec, rVec);
        vst1q(pCmplxDst, dstVec);

        pSrcReal += 2;
        pSrcCmplx += 4;
        pCmplxDst += 4;
        blkCnt --;
    }

    blkCnt = (blockSizeC & 3) >> 1; 
    while (blkCnt > 0U)
    {
      /* C[2 * i    ] = A[2 * i    ] * B[i]. */
      /* C[2 * i + 1] = A[2 * i + 1] * B[i]. */
  
      in = *pSrcReal++;
      /* store saturated result in 1.31 format to destination buffer */
      *pCmplxDst++ = (__SSAT((q31_t) (((q63_t) *pSrcCmplx++ * in) >> 32), 31) << 1);
      *pCmplxDst++ = (__SSAT((q31_t) (((q63_t) *pSrcCmplx++ * in) >> 32), 31) << 1);
  
      /* Decrement loop counter */
      blkCnt--;
    }
}
#else
void arm_cmplx_mult_real_q31(
  const q31_t * pSrcCmplx,
  const q31_t * pSrcReal,
        q31_t * pCmplxDst,
        uint32_t numSamples)
{
        uint32_t blkCnt;                               /* Loop counter */
        q31_t in;                                      /* Temporary variable */

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = numSamples >> 2U;

  while (blkCnt > 0U)
  {
    /* C[2 * i    ] = A[2 * i    ] * B[i]. */
    /* C[2 * i + 1] = A[2 * i + 1] * B[i]. */

    in = *pSrcReal++;
#if defined (ARM_MATH_DSP)
    /* store saturated result in 1.31 format to destination buffer */
    *pCmplxDst++ = (__SSAT((q31_t) (((q63_t) *pSrcCmplx++ * in) >> 32), 31) << 1);
    *pCmplxDst++ = (__SSAT((q31_t) (((q63_t) *pSrcCmplx++ * in) >> 32), 31) << 1);
#else
    /* store result in destination buffer. */
    *pCmplxDst++ = (q31_t) clip_q63_to_q31(((q63_t) *pSrcCmplx++ * in) >> 31);
    *pCmplxDst++ = (q31_t) clip_q63_to_q31(((q63_t) *pSrcCmplx++ * in) >> 31);
#endif

    in = *pSrcReal++;
#if defined (ARM_MATH_DSP)
    *pCmplxDst++ = (__SSAT((q31_t) (((q63_t) *pSrcCmplx++ * in) >> 32), 31) << 1);
    *pCmplxDst++ = (__SSAT((q31_t) (((q63_t) *pSrcCmplx++ * in) >> 32), 31) << 1);
#else
    *pCmplxDst++ = (q31_t) clip_q63_to_q31(((q63_t) *pSrcCmplx++ * in) >> 31);
    *pCmplxDst++ = (q31_t) clip_q63_to_q31(((q63_t) *pSrcCmplx++ * in) >> 31);
#endif

    in = *pSrcReal++;
#if defined (ARM_MATH_DSP)
    *pCmplxDst++ = (__SSAT((q31_t) (((q63_t) *pSrcCmplx++ * in) >> 32), 31) << 1);
    *pCmplxDst++ = (__SSAT((q31_t) (((q63_t) *pSrcCmplx++ * in) >> 32), 31) << 1);
#else
    *pCmplxDst++ = (q31_t) clip_q63_to_q31(((q63_t) *pSrcCmplx++ * in) >> 31);
    *pCmplxDst++ = (q31_t) clip_q63_to_q31(((q63_t) *pSrcCmplx++ * in) >> 31);
#endif

    in = *pSrcReal++;
#if defined (ARM_MATH_DSP)
    *pCmplxDst++ = (__SSAT((q31_t) (((q63_t) *pSrcCmplx++ * in) >> 32), 31) << 1);
    *pCmplxDst++ = (__SSAT((q31_t) (((q63_t) *pSrcCmplx++ * in) >> 32), 31) << 1);
#else
    *pCmplxDst++ = (q31_t) clip_q63_to_q31(((q63_t) *pSrcCmplx++ * in) >> 31);
    *pCmplxDst++ = (q31_t) clip_q63_to_q31(((q63_t) *pSrcCmplx++ * in) >> 31);
#endif

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = numSamples % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = numSamples;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* C[2 * i    ] = A[2 * i    ] * B[i]. */
    /* C[2 * i + 1] = A[2 * i + 1] * B[i]. */

    in = *pSrcReal++;
#if defined (ARM_MATH_DSP)
    /* store saturated result in 1.31 format to destination buffer */
    *pCmplxDst++ = (__SSAT((q31_t) (((q63_t) *pSrcCmplx++ * in) >> 32), 31) << 1);
    *pCmplxDst++ = (__SSAT((q31_t) (((q63_t) *pSrcCmplx++ * in) >> 32), 31) << 1);
#else
    /* store result in destination buffer. */
    *pCmplxDst++ = (q31_t) clip_q63_to_q31(((q63_t) *pSrcCmplx++ * in) >> 31);
    *pCmplxDst++ = (q31_t) clip_q63_to_q31(((q63_t) *pSrcCmplx++ * in) >> 31);
#endif

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of CmplxByRealMult group
 */
