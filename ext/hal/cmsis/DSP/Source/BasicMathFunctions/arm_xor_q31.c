/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_xor_q31.c
 * Description:  Q31 bitwise exclusive OR
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

/**
  @ingroup groupSupport
 */

/**
  @addtogroup Xor
  @{
 */

/**
  @brief         Compute the logical bitwise XOR of two fixed-point vectors.
  @param[in]     pSrcA      points to input vector A
  @param[in]     pSrcB      points to input vector B
  @param[out]    pDst       points to output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none
 */

#include "arm_math.h"

void arm_xor_q31(
    const q31_t * pSrcA,
    const q31_t * pSrcB,
    q31_t * pDst,
    uint32_t blockSize)
{
    uint32_t blkCnt;      /* Loop counter */

#if defined(ARM_MATH_NEON)
    int32x4_t vecA, vecB;

    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2U;

    while (blkCnt > 0U)
    {
        vecA = vld1q_s32(pSrcA);
        vecB = vld1q_s32(pSrcB);

        vst1q_s32(pDst, veorq_s32(vecA, vecB) );

        pSrcA += 4;
        pSrcB += 4;
        pDst  += 4;

        /* Decrement the loop counter */
        blkCnt--;
    }

    /* Tail */
    blkCnt = blockSize & 3;
#else
    /* Initialize blkCnt with number of samples */
    blkCnt = blockSize;
#endif

    while (blkCnt > 0U)
    {
        *pDst++ = (*pSrcA++)^(*pSrcB++);

        /* Decrement the loop counter */
        blkCnt--;
    }
}

/**
  @} end of Xor group
 */
