/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_and_q7.c
 * Description:  Q7 bitwise AND
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
  @addtogroup And
  @{
 */

/**
  @brief         Compute the logical bitwise AND of two fixed-point vectors.
  @param[in]     pSrcA      points to input vector A
  @param[in]     pSrcB      points to input vector B
  @param[out]    pDst       points to output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none
 */

void arm_and_q7(
    const q7_t * pSrcA,
    const q7_t * pSrcB,
    q7_t * pDst,
    uint32_t blockSize)
{
    uint32_t blkCnt;      /* Loop counter */

#if defined(ARM_MATH_NEON)
    int8x16_t vecA, vecB;

    /* Compute 16 outputs at a time */
    blkCnt = blockSize >> 4U;

    while (blkCnt > 0U)
    {
        vecA = vld1q_s8(pSrcA);
        vecB = vld1q_s8(pSrcB);

        vst1q_s8(pDst, vandq_s8(vecA, vecB) );

        pSrcA += 16;
        pSrcB += 16;
        pDst  += 16;

        /* Decrement the loop counter */
        blkCnt--;
    }

    /* Tail */
    blkCnt = blockSize & 0xF;
#else
    /* Initialize blkCnt with number of samples */
    blkCnt = blockSize;
#endif

    while (blkCnt > 0U)
    {
        *pDst++ = (*pSrcA++)&(*pSrcB++);

        /* Decrement the loop counter */
        blkCnt--;
    }
}

/**
  @} end of And group
 */
