/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_shift_q15.c
 * Description:  Shifts the elements of a Q15 vector by a specified number of bits
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
  @addtogroup BasicShift
  @{
 */

/**
  @brief         Shifts the elements of a Q15 vector a specified number of bits
  @param[in]     pSrc       points to the input vector
  @param[in]     shiftBits  number of bits to shift.  A positive value shifts left; a negative value shifts right.
  @param[out]    pDst       points to the output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   Results outside of the allowable Q15 range [0x8000 0x7FFF] are saturated.
 */

#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

void arm_shift_q15(
    const q15_t * pSrc,
    int8_t shiftBits,
    q15_t * pDst,
    uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q15x8_t vecSrc;
    q15x8_t vecDst;

    /* Compute 8 outputs at a time */
    blkCnt = blockSize >> 3;
    while (blkCnt > 0U)
    {
        /*
         * C = A (>> or <<) shiftBits
         * Shift the input and then store the result in the destination buffer.
         */
        vecSrc = vld1q(pSrc);
        vecDst = vqshlq_r(vecSrc, shiftBits);
        vst1q(pDst, vecDst);
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
        /*
         * advance vector source and destination pointers
         */
        pSrc += 8;
        pDst += 8;
    }
    /*
     * tail
     */
    blkCnt = blockSize & 7;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp16q(blkCnt);
        vecSrc = vld1q(pSrc);
        vecDst = vqshlq_r(vecSrc, shiftBits);
        vstrhq_p(pDst, vecDst, p0);
    }
}

#else
void arm_shift_q15(
  const q15_t * pSrc,
        int8_t shiftBits,
        q15_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */
        uint8_t sign = (shiftBits & 0x80);             /* Sign of shiftBits */

#if defined (ARM_MATH_LOOPUNROLL)

#if defined (ARM_MATH_DSP)
  q15_t in1, in2;                                /* Temporary input variables */
#endif

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  /* If the shift value is positive then do right shift else left shift */
  if (sign == 0U)
  {
    while (blkCnt > 0U)
    {
      /* C = A << shiftBits */

#if defined (ARM_MATH_DSP)
      /* read 2 samples from source */
      in1 = *pSrc++;
      in2 = *pSrc++;

      /* Shift the inputs and then store the results in the destination buffer. */
#ifndef ARM_MATH_BIG_ENDIAN
      write_q15x2_ia (&pDst, __PKHBT(__SSAT((in1 << shiftBits), 16),
                                     __SSAT((in2 << shiftBits), 16), 16));
#else
      write_q15x2_ia (&pDst, __PKHBT(__SSAT((in2 << shiftBits), 16),
                                      __SSAT((in1 << shiftBits), 16), 16));
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

      /* read 2 samples from source */
      in1 = *pSrc++;
      in2 = *pSrc++;

#ifndef ARM_MATH_BIG_ENDIAN
      write_q15x2_ia (&pDst, __PKHBT(__SSAT((in1 << shiftBits), 16),
                                     __SSAT((in2 << shiftBits), 16), 16));
#else
      write_q15x2_ia (&pDst, __PKHBT(__SSAT((in2 << shiftBits), 16),
                                     __SSAT((in1 << shiftBits), 16), 16));
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

#else
      *pDst++ = __SSAT(((q31_t) *pSrc++ << shiftBits), 16);
      *pDst++ = __SSAT(((q31_t) *pSrc++ << shiftBits), 16);
      *pDst++ = __SSAT(((q31_t) *pSrc++ << shiftBits), 16);
      *pDst++ = __SSAT(((q31_t) *pSrc++ << shiftBits), 16);
#endif

      /* Decrement loop counter */
      blkCnt--;
    }
  }
  else
  {
    while (blkCnt > 0U)
    {
      /* C = A >> shiftBits */

#if defined (ARM_MATH_DSP)
      /* read 2 samples from source */
      in1 = *pSrc++;
      in2 = *pSrc++;

      /* Shift the inputs and then store the results in the destination buffer. */
#ifndef ARM_MATH_BIG_ENDIAN
      write_q15x2_ia (&pDst, __PKHBT((in1 >> -shiftBits),
                                     (in2 >> -shiftBits), 16));
#else
      write_q15x2_ia (&pDst, __PKHBT((in2 >> -shiftBits),
                                     (in1 >> -shiftBits), 16));
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

      /* read 2 samples from source */
      in1 = *pSrc++;
      in2 = *pSrc++;

#ifndef ARM_MATH_BIG_ENDIAN
      write_q15x2_ia (&pDst, __PKHBT((in1 >> -shiftBits),
                                     (in2 >> -shiftBits), 16));
#else
      write_q15x2_ia (&pDst, __PKHBT((in2 >> -shiftBits),
                                     (in1 >> -shiftBits), 16));
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

#else
      *pDst++ = (*pSrc++ >> -shiftBits);
      *pDst++ = (*pSrc++ >> -shiftBits);
      *pDst++ = (*pSrc++ >> -shiftBits);
      *pDst++ = (*pSrc++ >> -shiftBits);
#endif

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
      *pDst++ = __SSAT(((q31_t) *pSrc++ << shiftBits), 16);

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
