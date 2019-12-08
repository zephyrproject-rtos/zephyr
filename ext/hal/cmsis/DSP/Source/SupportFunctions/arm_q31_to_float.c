/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_q31_to_float.c
 * Description:  Converts the elements of the Q31 vector to floating-point vector
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
 * @defgroup q31_to_x  Convert 32-bit Integer value
 */

/**
  @addtogroup q31_to_x
  @{
 */

/**
  @brief         Converts the elements of the Q31 vector to floating-point vector.
  @param[in]     pSrc       points to the Q31 input vector
  @param[out]    pDst       points to the floating-point output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Details
                   The equation used for the conversion process is:
  <pre>
      pDst[n] = (float32_t) pSrc[n] / 2147483648;   0 <= n < blockSize.
  </pre>
 */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
void arm_q31_to_float(
  const q31_t * pSrc,
        float32_t * pDst,
        uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q31x4_t vecDst;
    q31_t const *pSrcVec;

    pSrcVec = (q31_t const *) pSrc;
    blkCnt = blockSize >> 2;
    while (blkCnt > 0U)
    {
        /* C = (float32_t) A / 2147483648 */
        /* convert from q31 to float and then store the results in the destination buffer */
        vecDst = vld1q(pSrcVec);   
        pSrcVec += 4;
        vstrwq(pDst, vcvtq_n_f32_s32(vecDst, 31));  
        pDst += 4;
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
    while (blkCnt > 0U)
    {
      /* C = (float32_t) A / 2147483648 */
  
      /* Convert from q31 to float and store result in destination buffer */
      *pDst++ = ((float32_t) *pSrcVec++ / 2147483648.0f);
  
      /* Decrement loop counter */
      blkCnt--;
    }
}

#else
#if defined(ARM_MATH_NEON_EXPERIMENTAL)
void arm_q31_to_float(
  const q31_t * pSrc,
        float32_t * pDst,
        uint32_t blockSize)
{
  const q31_t *pIn = pSrc;                             /* Src pointer */
  uint32_t blkCnt;                               /* loop counter */

  int32x4_t inV;
  float32x4_t outV;

  blkCnt = blockSize >> 2U;

  /* Compute 4 outputs at a time.
   ** a second loop below computes the remaining 1 to 3 samples. */
  while (blkCnt > 0U)
  {
    /* C = (float32_t) A / 2147483648 */
    /* Convert from q31 to float and then store the results in the destination buffer */
    inV = vld1q_s32(pIn);
    pIn += 4;

    outV = vcvtq_n_f32_s32(inV,31);

    vst1q_f32(pDst, outV);
    pDst += 4;

    /* Decrement the loop counter */
    blkCnt--;
  }

  /* If the blockSize is not a multiple of 4, compute any remaining output samples here.
   ** No loop unrolling is used. */
  blkCnt = blockSize & 3;


  while (blkCnt > 0U)
  {
    /* C = (float32_t) A / 2147483648 */
    /* Convert from q31 to float and then store the results in the destination buffer */
    *pDst++ = ((float32_t) * pIn++ / 2147483648.0f);

    /* Decrement the loop counter */
    blkCnt--;
  }
}
#else
void arm_q31_to_float(
  const q31_t * pSrc,
  float32_t * pDst,
  uint32_t blockSize)
{
  const q31_t *pIn = pSrc;                             /* Src pointer */
  uint32_t blkCnt;                               /* loop counter */

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = (float32_t) A / 2147483648 */

    /* Convert from q31 to float and store result in destination buffer */
    *pDst++ = ((float32_t) *pIn++ / 2147483648.0f);
    *pDst++ = ((float32_t) *pIn++ / 2147483648.0f);
    *pDst++ = ((float32_t) *pIn++ / 2147483648.0f);
    *pDst++ = ((float32_t) *pIn++ / 2147483648.0f);

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
    /* C = (float32_t) A / 2147483648 */

    /* Convert from q31 to float and store result in destination buffer */
    *pDst++ = ((float32_t) *pIn++ / 2147483648.0f);

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* #if defined(ARM_MATH_NEON) */
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of q31_to_x group
 */
