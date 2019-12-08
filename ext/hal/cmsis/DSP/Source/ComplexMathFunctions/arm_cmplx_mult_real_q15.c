/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cmplx_mult_real_q15.c
 * Description:  Q15 complex by real multiplication
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
  @brief         Q15 complex-by-real multiplication.
  @param[in]     pSrcCmplx   points to complex input vector
  @param[in]     pSrcReal    points to real input vector
  @param[out]    pCmplxDst   points to complex output vector
  @param[in]     numSamples  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   Results outside of the allowable Q15 range [0x8000 0x7FFF] are saturated.
 */
#if defined(ARM_MATH_MVEI)

void arm_cmplx_mult_real_q15(
  const q15_t * pSrcCmplx,
  const q15_t * pSrcReal,
        q15_t * pCmplxDst,
        uint32_t numSamples)
{
  const static uint16_t stride_cmplx_x_real_16[8] = {
      0, 0, 1, 1, 2, 2, 3, 3
      };
  q15x8_t rVec;
  q15x8_t cmplxVec;
  q15x8_t dstVec;
  uint16x8_t strideVec;
  uint32_t blockSizeC = numSamples * CMPLX_DIM;   /* loop counters */
  uint32_t blkCnt;
  q15_t in;  

  /*
  * stride vector for pairs of real generation
  */
  strideVec = vld1q(stride_cmplx_x_real_16);

  blkCnt = blockSizeC >> 3;

  while (blkCnt > 0U) 
  {
    cmplxVec = vld1q(pSrcCmplx);
    rVec = vldrhq_gather_shifted_offset_s16(pSrcReal, strideVec);
    dstVec = vqdmulhq(cmplxVec, rVec);
    vst1q(pCmplxDst, dstVec);

    pSrcReal += 4;
    pSrcCmplx += 8;
    pCmplxDst += 8;
    blkCnt --;
  }

  /* Tail */
  blkCnt = (blockSizeC & 7) >> 1;
  while (blkCnt > 0U)
  {
    /* C[2 * i    ] = A[2 * i    ] * B[i]. */
    /* C[2 * i + 1] = A[2 * i + 1] * B[i]. */

    in = *pSrcReal++;
    /* store the result in the destination buffer. */
    *pCmplxDst++ = (q15_t) __SSAT((((q31_t) *pSrcCmplx++ * in) >> 15), 16);
    *pCmplxDst++ = (q15_t) __SSAT((((q31_t) *pSrcCmplx++ * in) >> 15), 16);

    /* Decrement loop counter */
    blkCnt--;
  }
}
#else
void arm_cmplx_mult_real_q15(
  const q15_t * pSrcCmplx,
  const q15_t * pSrcReal,
        q15_t * pCmplxDst,
        uint32_t numSamples)
{
        uint32_t blkCnt;                               /* Loop counter */
        q15_t in;                                      /* Temporary variable */

#if defined (ARM_MATH_LOOPUNROLL)

#if defined (ARM_MATH_DSP)
        q31_t inA1, inA2;                              /* Temporary variables to hold input data */
        q31_t inB1;                                    /* Temporary variables to hold input data */
        q15_t out1, out2, out3, out4;                  /* Temporary variables to hold output data */
        q31_t mul1, mul2, mul3, mul4;                  /* Temporary variables to hold intermediate data */
#endif

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = numSamples >> 2U;

  while (blkCnt > 0U)
  {
    /* C[2 * i    ] = A[2 * i    ] * B[i]. */
    /* C[2 * i + 1] = A[2 * i + 1] * B[i]. */

#if defined (ARM_MATH_DSP)
    /* read 2 complex numbers both real and imaginary from complex input buffer */
    inA1 = read_q15x2_ia ((q15_t **) &pSrcCmplx);
    inA2 = read_q15x2_ia ((q15_t **) &pSrcCmplx);
    /* read 2 real values at a time from real input buffer */
    inB1 = read_q15x2_ia ((q15_t **) &pSrcReal);

    /* multiply complex number with real numbers */
#ifndef ARM_MATH_BIG_ENDIAN
    mul1 = (q31_t) ((q15_t) (inA1)       * (q15_t) (inB1));
    mul2 = (q31_t) ((q15_t) (inA1 >> 16) * (q15_t) (inB1));
    mul3 = (q31_t) ((q15_t) (inA2)       * (q15_t) (inB1 >> 16));
    mul4 = (q31_t) ((q15_t) (inA2 >> 16) * (q15_t) (inB1 >> 16));
#else
    mul2 = (q31_t) ((q15_t) (inA1 >> 16) * (q15_t) (inB1 >> 16));
    mul1 = (q31_t) ((q15_t) inA1         * (q15_t) (inB1 >> 16));
    mul4 = (q31_t) ((q15_t) (inA2 >> 16) * (q15_t) inB1);
    mul3 = (q31_t) ((q15_t) inA2         * (q15_t) inB1);
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

    /* saturate the result */
    out1 = (q15_t) __SSAT(mul1 >> 15U, 16);
    out2 = (q15_t) __SSAT(mul2 >> 15U, 16);
    out3 = (q15_t) __SSAT(mul3 >> 15U, 16);
    out4 = (q15_t) __SSAT(mul4 >> 15U, 16);

    /* pack real and imaginary outputs and store them to destination */
    write_q15x2_ia (&pCmplxDst, __PKHBT(out1, out2, 16));
    write_q15x2_ia (&pCmplxDst, __PKHBT(out3, out4, 16));

    inA1 = read_q15x2_ia ((q15_t **) &pSrcCmplx);
    inA2 = read_q15x2_ia ((q15_t **) &pSrcCmplx);
    inB1 = read_q15x2_ia ((q15_t **) &pSrcReal);

#ifndef ARM_MATH_BIG_ENDIAN
    mul1 = (q31_t) ((q15_t) (inA1)       * (q15_t) (inB1));
    mul2 = (q31_t) ((q15_t) (inA1 >> 16) * (q15_t) (inB1));
    mul3 = (q31_t) ((q15_t) (inA2)       * (q15_t) (inB1 >> 16));
    mul4 = (q31_t) ((q15_t) (inA2 >> 16) * (q15_t) (inB1 >> 16));
#else
    mul2 = (q31_t) ((q15_t) (inA1 >> 16) * (q15_t) (inB1 >> 16));
    mul1 = (q31_t) ((q15_t) inA1         * (q15_t) (inB1 >> 16));
    mul4 = (q31_t) ((q15_t) (inA2 >> 16) * (q15_t) inB1);
    mul3 = (q31_t) ((q15_t) inA2 * (q15_t) inB1);
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

    out1 = (q15_t) __SSAT(mul1 >> 15U, 16);
    out2 = (q15_t) __SSAT(mul2 >> 15U, 16);
    out3 = (q15_t) __SSAT(mul3 >> 15U, 16);
    out4 = (q15_t) __SSAT(mul4 >> 15U, 16);

    write_q15x2_ia (&pCmplxDst, __PKHBT(out1, out2, 16));
    write_q15x2_ia (&pCmplxDst, __PKHBT(out3, out4, 16));
#else
    in = *pSrcReal++;
    *pCmplxDst++ = (q15_t) __SSAT((((q31_t) *pSrcCmplx++ * in) >> 15), 16);
    *pCmplxDst++ = (q15_t) __SSAT((((q31_t) *pSrcCmplx++ * in) >> 15), 16);

    in = *pSrcReal++;
    *pCmplxDst++ = (q15_t) __SSAT((((q31_t) *pSrcCmplx++ * in) >> 15), 16);
    *pCmplxDst++ = (q15_t) __SSAT((((q31_t) *pSrcCmplx++ * in) >> 15), 16);

    in = *pSrcReal++;
    *pCmplxDst++ = (q15_t) __SSAT((((q31_t) *pSrcCmplx++ * in) >> 15), 16);
    *pCmplxDst++ = (q15_t) __SSAT((((q31_t) *pSrcCmplx++ * in) >> 15), 16);

    in = *pSrcReal++;
    *pCmplxDst++ = (q15_t) __SSAT((((q31_t) *pSrcCmplx++ * in) >> 15), 16);
    *pCmplxDst++ = (q15_t) __SSAT((((q31_t) *pSrcCmplx++ * in) >> 15), 16);
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
    /* store the result in the destination buffer. */
    *pCmplxDst++ = (q15_t) __SSAT((((q31_t) *pSrcCmplx++ * in) >> 15), 16);
    *pCmplxDst++ = (q15_t) __SSAT((((q31_t) *pSrcCmplx++ * in) >> 15), 16);

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of CmplxByRealMult group
 */
