/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_copy_f32.c
 * Description:  Copies the elements of a floating-point vector
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
  @defgroup copy Vector Copy

  Copies sample by sample from source vector to destination vector.

  <pre>
      pDst[n] = pSrc[n];   0 <= n < blockSize.
  </pre>

  There are separate functions for floating point, Q31, Q15, and Q7 data types.
 */

/**
  @addtogroup copy
  @{
 */

/**
  @brief         Copies the elements of a floating-point vector.
  @param[in]     pSrc       points to input vector
  @param[out]    pDst       points to output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none
 */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

void arm_copy_f32(
  const float32_t * pSrc,
  float32_t * pDst,
  uint32_t blockSize)
{
  uint32_t blkCnt;
  blkCnt = blockSize >> 2U;

  /* Compute 4 outputs at a time */
  while (blkCnt > 0U)
  {
      vstrwq_f32(pDst, vldrwq_f32(pSrc));
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
    /* C = A */

    /* Copy and store result in destination buffer */
    *pDst++ = *pSrc++;

    /* Decrement loop counter */
    blkCnt--;
  }
    
}

#else
#if defined(ARM_MATH_NEON_EXPERIMENTAL)
void arm_copy_f32(
  const float32_t * pSrc,
  float32_t * pDst,
  uint32_t blockSize)
{
  uint32_t blkCnt;                               /* loop counter */

  float32x4_t inV;

  blkCnt = blockSize >> 2U;

  /* Compute 4 outputs at a time.
   ** a second loop below computes the remaining 1 to 3 samples. */
  while (blkCnt > 0U)
  {
    /* C = A */
    /* Copy and then store the results in the destination buffer */
    inV = vld1q_f32(pSrc);
    vst1q_f32(pDst, inV);
    pSrc += 4;
    pDst += 4;

    /* Decrement the loop counter */
    blkCnt--;
  }

  /* If the blockSize is not a multiple of 4, compute any remaining output samples here.
   ** No loop unrolling is used. */
  blkCnt = blockSize & 3;

  while (blkCnt > 0U)
  {
    /* C = A */
    /* Copy and then store the results in the destination buffer */
    *pDst++ = *pSrc++;

    /* Decrement the loop counter */
    blkCnt--;
  }
}
#else
void arm_copy_f32(
  const float32_t * pSrc,
        float32_t * pDst,
        uint32_t blockSize)
{
  uint32_t blkCnt;                               /* Loop counter */

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A */

    /* Copy and store result in destination buffer */
    *pDst++ = *pSrc++;
    *pDst++ = *pSrc++;
    *pDst++ = *pSrc++;
    *pDst++ = *pSrc++;

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
    /* C = A */

    /* Copy and store result in destination buffer */
    *pDst++ = *pSrc++;

    /* Decrement loop counter */
    blkCnt--;
  }
}
#endif /* #if defined(ARM_MATH_NEON) */
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of BasicCopy group
 */
