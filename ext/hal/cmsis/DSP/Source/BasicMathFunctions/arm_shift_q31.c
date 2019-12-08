/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_shift_q31.c
 * Description:  Shifts the elements of a Q31 vector by a specified number of bits
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
  @ingroup groupMath
 */
/**
  @defgroup BasicShift Vector Shift

  Shifts the elements of a fixed-point vector by a specified number of bits.
  There are separate functions for Q7, Q15, and Q31 data types.
  The underlying algorithm used is:

  <pre>
      pDst[n] = pSrc[n] << shift,   0 <= n < blockSize.
  </pre>

  If <code>shift</code> is positive then the elements of the vector are shifted to the left.
  If <code>shift</code> is negative then the elements of the vector are shifted to the right.

  The functions support in-place computation allowing the source and destination
  pointers to reference the same memory buffer.
 */

/**
  @addtogroup BasicShift
  @{
 */

/**
  @brief         Shifts the elements of a Q31 vector a specified number of bits.
  @param[in]     pSrc       points to the input vector
  @param[in]     shiftBits  number of bits to shift.  A positive value shifts left; a negative value shifts right.
  @param[out]    pDst       points to the output vector
  @param[in]     blockSize  number of samples in the vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   Results outside of the allowable Q31 range [0x80000000 0x7FFFFFFF] are saturated.
 */

#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

void arm_shift_q31(
    const q31_t * pSrc,
    int8_t shiftBits,
    q31_t * pDst,
    uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q31x4_t vecSrc;
    q31x4_t vecDst;

    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2;
    while (blkCnt > 0U)
    {
        /*
         * C = A (>> or <<) shiftBits
         * Shift the input and then store the result in the destination buffer.
         */
        vecSrc = vld1q((q31_t const *) pSrc);
        vecDst = vqshlq_r(vecSrc, shiftBits);
        vst1q(pDst, vecDst);
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
        /*
         * advance vector source and destination pointers
         */
        pSrc += 4;
        pDst += 4;
    }
    /*
     * tail
     */
    blkCnt = blockSize & 3;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp32q(blkCnt);
        vecSrc = vld1q((q31_t const *) pSrc);
        vecDst = vqshlq_r(vecSrc, shiftBits);
        vstrwq_p(pDst, vecDst, p0);
    }
}


#else
void arm_shift_q31(
  const q31_t * pSrc,
        int8_t shiftBits,
        q31_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */
        uint8_t sign = (shiftBits & 0x80);             /* Sign of shiftBits */

#if defined (ARM_MATH_LOOPUNROLL)

  q31_t in, out;                                 /* Temporary variables */

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  /* If the shift value is positive then do right shift else left shift */
  if (sign == 0U)
  {
    while (blkCnt > 0U)
    {
      /* C = A << shiftBits */

      /* Shift input and store result in destination buffer. */
      in = *pSrc++;
      out = in << shiftBits;
      if (in != (out >> shiftBits))
        out = 0x7FFFFFFF ^ (in >> 31);
      *pDst++ = out;

      in = *pSrc++;
      out = in << shiftBits;
      if (in != (out >> shiftBits))
        out = 0x7FFFFFFF ^ (in >> 31);
      *pDst++ = out;

      in = *pSrc++;
      out = in << shiftBits;
      if (in != (out >> shiftBits))
        out = 0x7FFFFFFF ^ (in >> 31);
      *pDst++ = out;

      in = *pSrc++;
      out = in << shiftBits;
      if (in != (out >> shiftBits))
        out = 0x7FFFFFFF ^ (in >> 31);
      *pDst++ = out;

      /* Decrement loop counter */
      blkCnt--;
    }
  }
  else
  {
    while (blkCnt > 0U)
    {
      /* C = A >> shiftBits */

      /* Shift input and store results in destination buffer. */
      *pDst++ = (*pSrc++ >> -shiftBits);
      *pDst++ = (*pSrc++ >> -shiftBits);
      *pDst++ = (*pSrc++ >> -shiftBits);
      *pDst++ = (*pSrc++ >> -shiftBits);

      /* Decrement loop counter */
      blkCnt--;
    }
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = blockSize % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  /* If the shift value is positive then do right shift else left shift */
  if (sign == 0U)
  {
    while (blkCnt > 0U)
    {
      /* C = A << shiftBits */

      /* Shift input and store result in destination buffer. */
      *pDst++ = clip_q63_to_q31((q63_t) *pSrc++ << shiftBits);

      /* Decrement loop counter */
      blkCnt--;
    }
  }
  else
  {
    while (blkCnt > 0U)
    {
      /* C = A >> shiftBits */

      /* Shift input and store result in destination buffer. */
      *pDst++ = (*pSrc++ >> -shiftBits);

      /* Decrement loop counter */
      blkCnt--;
    }
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of BasicShift group
 */
