/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_fill_f32.c
 * Description:  Fills a constant value into a floating-point vector
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
  @defgroup Fill Vector Fill

  Fills the destination vector with a constant value.

  <pre>
      pDst[n] = value;   0 <= n < blockSize.
  </pre>

  There are separate functions for floating point, Q31, Q15, and Q7 data types.
 */

/**
  @addtogroup Fill
  @{
 */

/**
  @brief         Fills a constant value into a floating-point vector.
  @param[in]     value      input value to be filled
  @param[out]    pDst       points to output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none
 */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
void arm_fill_f32(
  float32_t value,
  float32_t * pDst,
  uint32_t blockSize)
{
  uint32_t blkCnt;
  blkCnt = blockSize >> 2U;

  /* Compute 4 outputs at a time */
  while (blkCnt > 0U)
  {

    vstrwq_f32(pDst,vdupq_n_f32(value));
    /*
     * Decrement the blockSize loop counter
     * Advance vector source and destination pointers
     */
    pDst += 4;
    blkCnt --;
  }

  blkCnt = blockSize & 3;

  while (blkCnt > 0U)
  {
    /* C = value */

    /* Fill value in destination buffer */
    *pDst++ = value;

    /* Decrement loop counter */
    blkCnt--;
  }

}
#else
#if defined(ARM_MATH_NEON_EXPERIMENTAL)
void arm_fill_f32(
  float32_t value,
  float32_t * pDst,
  uint32_t blockSize)
{
  uint32_t blkCnt;                               /* loop counter */


  float32x4_t inV = vdupq_n_f32(value);

  blkCnt = blockSize >> 2U;

  /* Compute 4 outputs at a time.
   ** a second loop below computes the remaining 1 to 3 samples. */
  while (blkCnt > 0U)
  {
    /* C = value */
    /* Fill the value in the destination buffer */
    vst1q_f32(pDst, inV);
    pDst += 4;

    /* Decrement the loop counter */
    blkCnt--;
  }

  /* If the blockSize is not a multiple of 4, compute any remaining output samples here.
   ** No loop unrolling is used. */
  blkCnt = blockSize & 3;

  while (blkCnt > 0U)
  {
    /* C = value */
    /* Fill the value in the destination buffer */
    *pDst++ = value;

    /* Decrement the loop counter */
    blkCnt--;
  }
}
#else
void arm_fill_f32(
  float32_t value,
  float32_t * pDst,
  uint32_t blockSize)
{
  uint32_t blkCnt;                               /* Loop counter */

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = value */

    /* Fill value in destination buffer */
    *pDst++ = value;
    *pDst++ = value;
    *pDst++ = value;
    *pDst++ = value;

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
    /* C = value */

    /* Fill value in destination buffer */
    *pDst++ = value;

    /* Decrement loop counter */
    blkCnt--;
  }
}
#endif /* #if defined(ARM_MATH_NEON) */
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of Fill group
 */
