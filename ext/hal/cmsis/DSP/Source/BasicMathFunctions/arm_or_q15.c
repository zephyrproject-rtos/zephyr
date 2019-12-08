/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_or_q15.c
 * Description:  Q15 bitwise inclusive OR
 *
 * $Date:        14 November 2019
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
  @defgroup Or Vector bitwise inclusive OR

  Compute the logical bitwise OR.

  There are separate functions for Q31, Q15, and Q7 data types.
 */

/**
  @addtogroup Or
  @{
 */

/**
  @brief         Compute the logical bitwise OR of two fixed-point vectors.
  @param[in]     pSrcA      points to input vector A
  @param[in]     pSrcB      points to input vector B
  @param[out]    pDst       points to output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none
 */

void arm_or_q15(
    const q15_t * pSrcA,
    const q15_t * pSrcB,
    q15_t * pDst,
    uint32_t blockSize)
{
    uint32_t blkCnt;      /* Loop counter */

#if defined(ARM_MATH_NEON)
    int16x8_t vecA, vecB;

    /* Compute 8 outputs at a time */
    blkCnt = blockSize >> 3U;

    while (blkCnt > 0U)
    {
        vecA = vld1q_s16(pSrcA);
        vecB = vld1q_s16(pSrcB);

        vst1q_s16(pDst, vorrq_s16(vecA, vecB) );

        pSrcA += 8;
        pSrcB += 8;
        pDst  += 8;

        /* Decrement the loop counter */
        blkCnt--;
    }

    /* Tail */
    blkCnt = blockSize & 7;
#else
    /* Initialize blkCnt with number of samples */
    blkCnt = blockSize;
#endif

    while (blkCnt > 0U)
    {
        *pDst++ = (*pSrcA++)|(*pSrcB++);

        /* Decrement the loop counter */
        blkCnt--;
    }
}

/**
  @} end of Or group
 */
