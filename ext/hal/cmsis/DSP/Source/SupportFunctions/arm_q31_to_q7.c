/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_q31_to_q7.c
 * Description:  Converts the elements of the Q31 vector to Q7 vector
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
  @brief         Converts the elements of the Q31 vector to Q7 vector.
  @param[in]     pSrc       points to the Q31 input vector
  @param[out]    pDst       points to the Q7 output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Details
                   The equation used for the conversion process is:
  <pre>
      pDst[n] = (q7_t) pSrc[n] >> 24;   0 <= n < blockSize.
  </pre>
 */
#if defined(ARM_MATH_MVEI)
void arm_q31_to_q7(
  const q31_t * pSrc,
        q7_t * pDst,
        uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q31x4x4_t tmp;
    q15x8_t evVec, oddVec;
    q7x16_t  vecDst;
    q31_t const *pSrcVec;

    pSrcVec = (q31_t const *) pSrc;
    blkCnt = blockSize >> 4;
    while (blkCnt > 0U)
    {
        tmp = vld4q(pSrcVec);  
        pSrcVec += 16;
        /* C = (q7_t) A >> 24 */
        /* convert from q31 to q7 and then store the results in the destination buffer */
        /*
         * narrow and pack evens
         */
        evVec = vshrnbq_n_s32(evVec, tmp.val[0], 16);
        evVec = vshrntq_n_s32(evVec, tmp.val[2], 16);
        /*
         * narrow and pack odds
         */
        oddVec = vshrnbq_n_s32(oddVec, tmp.val[1], 16);
        oddVec = vshrntq_n_s32(oddVec, tmp.val[3], 16);
        /*
         * narrow & merge
         */
        vecDst = vshrnbq_n_s16(vecDst, evVec, 8);
        vecDst = vshrntq_n_s16(vecDst, oddVec, 8);

        vst1q(pDst, vecDst);    
        pDst += 16;
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
    }
    /*
     * tail
     */
    blkCnt = blockSize & 0xF;
    while (blkCnt > 0U)
    {
      /* C = (q7_t) (A >> 24) */
  
      /* Convert from q31 to q7 and store result in destination buffer */
      *pDst++ = (q7_t) (*pSrcVec++ >> 24);
  
      /* Decrement loop counter */
      blkCnt--;
    }
}
#else
void arm_q31_to_q7(
  const q31_t * pSrc,
        q7_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */
  const q31_t *pIn = pSrc;                             /* Source pointer */

#if defined (ARM_MATH_LOOPUNROLL)

  q7_t out1, out2, out3, out4;

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = (q7_t) (A >> 24) */

    /* Convert from q31 to q7 and store result in destination buffer */

    out1 = (q7_t) (*pIn++ >> 24);
    out2 = (q7_t) (*pIn++ >> 24);
    out3 = (q7_t) (*pIn++ >> 24);
    out4 = (q7_t) (*pIn++ >> 24);
    write_q7x4_ia (&pDst, __PACKq7(out1, out2, out3, out4));

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
    /* C = (q7_t) (A >> 24) */

    /* Convert from q31 to q7 and store result in destination buffer */
    *pDst++ = (q7_t) (*pIn++ >> 24);

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of q31_to_x group
 */
