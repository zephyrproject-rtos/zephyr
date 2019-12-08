/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_not_q15.c
 * Description:  Q15 bitwise NOT
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
  @defgroup Not Vector bitwise NOT

  Compute the logical bitwise NOT.

  There are separate functions for Q31, Q15, and Q7 data types.
 */

/**
  @addtogroup Not
  @{
 */

/**
  @brief         Compute the logical bitwise NOT of a fixed-point vector.
  @param[in]     pSrc       points to input vector 
  @param[out]    pDst       points to output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none
 */

void arm_not_q15(
    const q15_t * pSrc,
          q15_t * pDst,
    uint32_t blockSize)
{
    uint32_t blkCnt;      /* Loop counter */

#if defined(ARM_MATH_NEON)
    int16x8_t inV;

    /* Compute 8 outputs at a time */
    blkCnt = blockSize >> 3U;

    while (blkCnt > 0U)
    {
        inV = vld1q_s16(pSrc);

        vst1q_s16(pDst, vmvnq_s16(inV) );

        pSrc += 8;
        pDst += 8;

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
        *pDst++ = ~(*pSrc++);

        /* Decrement the loop counter */
        blkCnt--;
    }
}

/**
  @} end of Not group
 */
