/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_q31_to_q15.c
 * Description:  Converts the elements of the Q31 vector to Q15 vector
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
  @addtogroup q31_to_x
  @{
 */

/**
  @brief         Converts the elements of the Q31 vector to Q15 vector.
  @param[in]     pSrc       points to the Q31 input vector
  @param[out]    pDst       points to the Q15 output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Details
                   The equation used for the conversion process is:
  <pre>
      pDst[n] = (q15_t) pSrc[n] >> 16;   0 <= n < blockSize.
  </pre>
 */
#if defined(ARM_MATH_MVEI)
void arm_q31_to_q15(
  const q31_t * pSrc,
        q15_t * pDst,
        uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q31x4x2_t tmp;
    q15x8_t vecDst;
    q31_t const *pSrcVec;


    pSrcVec = (q31_t const *) pSrc;
    blkCnt = blockSize >> 3;
    while (blkCnt > 0U)
    {
        /* C = (q15_t) A >> 16 */
        /* convert from q31 to q15 and then store the results in the destination buffer */
        tmp = vld2q(pSrcVec);   
        pSrcVec += 8;
        vecDst = vshrnbq_n_s32(vecDst, tmp.val[0], 16);
        vecDst = vshrntq_n_s32(vecDst, tmp.val[1], 16);
        vst1q(pDst, vecDst);    
        pDst += 8;
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
    }

    /*
     * tail
     */
    blkCnt = blockSize & 7;
    while (blkCnt > 0U)
    {
      /* C = (q15_t) (A >> 16) */
  
      /* Convert from q31 to q15 and store result in destination buffer */
      *pDst++ = (q15_t) (*pSrcVec++ >> 16);
  
      /* Decrement loop counter */
      blkCnt--;
    }
}

#else
void arm_q31_to_q15(
  const q31_t * pSrc,
        q15_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */
  const q31_t *pIn = pSrc;                             /* Source pointer */

#if defined (ARM_MATH_LOOPUNROLL) && defined (ARM_MATH_DSP)
        q31_t in1, in2, in3, in4;
        q31_t out1, out2;
#endif

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = (q15_t) (A >> 16) */

    /* Convert from q31 to q15 and store result in destination buffer */
#if defined (ARM_MATH_DSP)

    in1 = *pIn++;
    in2 = *pIn++;
    in3 = *pIn++;
    in4 = *pIn++;

    /* pack two higher 16-bit values from two 32-bit values */
#ifndef ARM_MATH_BIG_ENDIAN
    out1 = __PKHTB(in2, in1, 16);
    out2 = __PKHTB(in4, in3, 16);
#else
    out1 = __PKHTB(in1, in2, 16);
    out2 = __PKHTB(in3, in4, 16);
#endif /* #ifdef ARM_MATH_BIG_ENDIAN */

    write_q15x2_ia (&pDst, out1);
    write_q15x2_ia (&pDst, out2);

#else

    *pDst++ = (q15_t) (*pIn++ >> 16);
    *pDst++ = (q15_t) (*pIn++ >> 16);
    *pDst++ = (q15_t) (*pIn++ >> 16);
    *pDst++ = (q15_t) (*pIn++ >> 16);

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
    /* C = (q15_t) (A >> 16) */

    /* Convert from q31 to q15 and store result in destination buffer */
    *pDst++ = (q15_t) (*pIn++ >> 16);

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of q31_to_x group
 */
