/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_q7_to_q31.c
 * Description:  Converts the elements of the Q7 vector to Q31 vector
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
  @brief         Converts the elements of the Q7 vector to Q31 vector.
  @param[in]     pSrc       points to the Q7 input vector
  @param[out]    pDst       points to the Q31 output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Details
                   The equation used for the conversion process is:
  <pre>
      pDst[n] = (q31_t) pSrc[n] << 24;   0 <= n < blockSize.
  </pre>
 */
#if defined(ARM_MATH_MVEI)
void arm_q7_to_q31(
  const q7_t * pSrc,
        q31_t * pDst,
        uint32_t blockSize)
{
    uint32_t blkCnt;   
    q31x4_t         vecDst;

    blkCnt = blockSize >> 2;
    while (blkCnt > 0U)
    {

        /* C = (q31_t)A << 16 */
        /* convert from q15 to q31 and then store the results in the destination buffer */
        /* load q15 + 32-bit widening */
        vecDst = vldrbq_s32((q7_t const *) pSrc);
        vecDst = vshlq_n(vecDst, 24);
        vstrwq_s32(pDst, vecDst);

        /*
         * Decrement the blockSize loop counter
         * Advance vector source and destination pointers
         */
        pDst += 4;
        pSrc += 4;
        blkCnt --;
     }

  blkCnt = blockSize & 3;
  while (blkCnt > 0U)
  {
    /* C = (q31_t) A << 24 */

    /* Convert from q7 to q31 and store result in destination buffer */
    *pDst++ = (q31_t) *pSrc++ << 24;

    /* Decrement loop counter */
    blkCnt--;
  }
}

#else
void arm_q7_to_q31(
  const q7_t * pSrc,
        q31_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */
  const q7_t *pIn = pSrc;                              /* Source pointer */

#if defined (ARM_MATH_LOOPUNROLL)

        q31_t in;

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = (q31_t) A << 24 */

    /* Convert from q7 to q31 and store result in destination buffer */
    in = read_q7x4_ia ((q7_t **) &pIn);

#ifndef ARM_MATH_BIG_ENDIAN

    *pDst++ = (__ROR(in, 8)) & 0xFF000000;
    *pDst++ = (__ROR(in, 16)) & 0xFF000000;
    *pDst++ = (__ROR(in, 24)) & 0xFF000000;
    *pDst++ = (in & 0xFF000000);

#else

    *pDst++ = (in & 0xFF000000);
    *pDst++ = (__ROR(in, 24)) & 0xFF000000;
    *pDst++ = (__ROR(in, 16)) & 0xFF000000;
    *pDst++ = (__ROR(in, 8)) & 0xFF000000;

#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

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
    /* C = (q31_t) A << 24 */

    /* Convert from q7 to q31 and store result in destination buffer */
    *pDst++ = (q31_t) * pIn++ << 24;

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of q7_to_x group
 */
