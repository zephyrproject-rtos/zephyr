/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_q7_to_q15.c
 * Description:  Converts the elements of the Q7 vector to Q15 vector
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
  @addtogroup q7_to_x
  @{
 */

/**
  @brief         Converts the elements of the Q7 vector to Q15 vector.
  @param[in]     pSrc       points to the Q7 input vector
  @param[out]    pDst       points to the Q15 output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Details
                   The equation used for the conversion process is:
  <pre>
      pDst[n] = (q15_t) pSrc[n] << 8;   0 <= n < blockSize.
  </pre>
 */

#if defined(ARM_MATH_MVEI)
void arm_q7_to_q15(
  const q7_t * pSrc,
        q15_t * pDst,
        uint32_t blockSize)
{

    uint32_t  blkCnt;           /* loop counters */
    q15x8_t vecDst;
    q7_t const *pSrcVec;


    pSrcVec = (q7_t const *) pSrc;
    blkCnt = blockSize >> 3;
    while (blkCnt > 0U)
    {
        /* C = (q15_t) A << 8 */
        /* convert from q7 to q15 and then store the results in the destination buffer */
        /* load q7 + 32-bit widening */
        vecDst = vldrbq_s16(pSrcVec);    
        pSrcVec += 8;
        vecDst = vecDst << 8;
        vstrhq(pDst, vecDst);   
        pDst += 8;
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
    }

  blkCnt = blockSize & 7;
  while (blkCnt > 0U)
  {
    /* C = (q15_t) A << 8 */

    /* Convert from q7 to q15 and store result in destination buffer */
    *pDst++ = (q15_t) * pSrcVec++ << 8;

    /* Decrement loop counter */
    blkCnt--;
  }

}
#else
void arm_q7_to_q15(
  const q7_t * pSrc,
        q15_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */
  const q7_t *pIn = pSrc;                              /* Source pointer */

#if defined (ARM_MATH_LOOPUNROLL) && defined (ARM_MATH_DSP)
        q31_t in;
        q31_t in1, in2;
        q31_t out1, out2;
#endif

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = (q15_t) A << 8 */

    /* Convert from q7 to q15 and store result in destination buffer */
#if defined (ARM_MATH_DSP)

    in = read_q7x4_ia ((q7_t **) &pIn);

    /* rotatate in by 8 and extend two q7_t values to q15_t values */
    in1 = __SXTB16(__ROR(in, 8));

    /* extend remainig two q7_t values to q15_t values */
    in2 = __SXTB16(in);

    in1 = in1 << 8U;
    in2 = in2 << 8U;

    in1 = in1 & 0xFF00FF00;
    in2 = in2 & 0xFF00FF00;

#ifndef ARM_MATH_BIG_ENDIAN
    out2 = __PKHTB(in1, in2, 16);
    out1 = __PKHBT(in2, in1, 16);
#else
    out1 = __PKHTB(in1, in2, 16);
    out2 = __PKHBT(in2, in1, 16);
#endif

    write_q15x2_ia (&pDst, out1);
    write_q15x2_ia (&pDst, out2);

#else

    *pDst++ = (q15_t) *pIn++ << 8;
    *pDst++ = (q15_t) *pIn++ << 8;
    *pDst++ = (q15_t) *pIn++ << 8;
    *pDst++ = (q15_t) *pIn++ << 8;

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
    /* C = (q15_t) A << 8 */

    /* Convert from q7 to q15 and store result in destination buffer */
    *pDst++ = (q15_t) * pIn++ << 8;

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of q7_to_x group
 */
