/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_q15_to_q7.c
 * Description:  Converts the elements of the Q15 vector to Q7 vector
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
  @addtogroup q15_to_x
  @{
 */

/**
  @brief         Converts the elements of the Q15 vector to Q7 vector.
  @param[in]     pSrc       points to the Q15 input vector
  @param[out]    pDst       points to the Q7 output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Details
                   The equation used for the conversion process is:
  <pre>
      pDst[n] = (q7_t) pSrc[n] >> 8;   0 <= n < blockSize.
  </pre>
 */
#if defined(ARM_MATH_MVEI)
void arm_q15_to_q7(
  const q15_t * pSrc,
        q7_t * pDst,
        uint32_t blockSize)
{

    uint32_t  blkCnt;           /* loop counters */
    q15x8x2_t tmp;
    q15_t const *pSrcVec;
    q7x16_t vecDst;


    pSrcVec = (q15_t const *) pSrc;
    blkCnt = blockSize >> 4;
    while (blkCnt > 0U)
    {
        /* C = (q7_t) A >> 8 */
        /* convert from q15 to q7 and then store the results in the destination buffer */
        tmp = vld2q(pSrcVec);   
        pSrcVec += 16;
        vecDst = vqshrnbq_n_s16(vecDst, tmp.val[0], 8);
        vecDst = vqshrntq_n_s16(vecDst, tmp.val[1], 8);
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
    /* C = (q7_t) A >> 8 */

    /* Convert from q15 to q7 and store result in destination buffer */
    *pDst++ = (q7_t) (*pSrcVec++ >> 8);

    /* Decrement loop counter */
    blkCnt--;
  }
}
#else
void arm_q15_to_q7(
  const q15_t * pSrc,
        q7_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */
  const q15_t *pIn = pSrc;                             /* Source pointer */

#if defined (ARM_MATH_LOOPUNROLL) && defined (ARM_MATH_DSP)
        q31_t in1, in2;
        q31_t out1, out2;
#endif

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = (q7_t) A >> 8 */

    /* Convert from q15 to q7 and store result in destination buffer */
#if defined (ARM_MATH_DSP)

    in1 = read_q15x2_ia ((q15_t **) &pIn);
    in2 = read_q15x2_ia ((q15_t **) &pIn);

#ifndef ARM_MATH_BIG_ENDIAN

    out1 = __PKHTB(in2, in1, 16);
    out2 = __PKHBT(in2, in1, 16);

#else

    out1 = __PKHTB(in1, in2, 16);
    out2 = __PKHBT(in1, in2, 16);

#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

    /* rotate packed value by 24 */
    out2 = ((uint32_t) out2 << 8) | ((uint32_t) out2 >> 24);

    /* anding with 0xff00ff00 to get two 8 bit values */
    out1 = out1 & 0xFF00FF00;
    /* anding with 0x00ff00ff to get two 8 bit values */
    out2 = out2 & 0x00FF00FF;

    /* oring two values(contains two 8 bit values) to get four packed 8 bit values */
    out1 = out1 | out2;

    /* store 4 samples at a time to destiantion buffer */
    write_q7x4_ia (&pDst, out1);

#else

    *pDst++ = (q7_t) (*pIn++ >> 8);
    *pDst++ = (q7_t) (*pIn++ >> 8);
    *pDst++ = (q7_t) (*pIn++ >> 8);
    *pDst++ = (q7_t) (*pIn++ >> 8);

#endif /* #if defined (ARM_MATH_DSP) */

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
    /* C = (q7_t) A >> 8 */

    /* Convert from q15 to q7 and store result in destination buffer */
    *pDst++ = (q7_t) (*pIn++ >> 8);

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of q15_to_x group
 */
