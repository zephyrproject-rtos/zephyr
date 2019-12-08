/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_offset_q15.c
 * Description:  Q15 vector offset
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
  @addtogroup BasicOffset
  @{
 */

/**
  @brief         Adds a constant offset to a Q15 vector.
  @param[in]     pSrc       points to the input vector
  @param[in]     offset     is the offset to be added
  @param[out]    pDst       points to the output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   Results outside of the allowable Q15 range [0x8000 0x7FFF] are saturated.
 */
#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

void arm_offset_q15(
    const q15_t * pSrc,
    q15_t   offset,
    q15_t * pDst,
    uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q15x8_t vecSrc;

    /* Compute 8 outputs at a time */
    blkCnt = blockSize >> 3;
    while (blkCnt > 0U)
    {
        /*
         * C = A + offset
         * Add offset and then store the result in the destination buffer.
         */
        vecSrc = vld1q(pSrc);
        vst1q(pDst, vqaddq(vecSrc, offset));
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
        vstrhq_p(pDst, vqaddq(vecSrc, offset), p0);
    }
}


#else
void arm_offset_q15(
  const q15_t * pSrc,
        q15_t offset,
        q15_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */

#if defined (ARM_MATH_LOOPUNROLL)

#if defined (ARM_MATH_DSP)
  q31_t offset_packed;                           /* Offset packed to 32 bit */

  /* Offset is packed to 32 bit in order to use SIMD32 for addition */
  offset_packed = __PKHBT(offset, offset, 16);
#endif

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A + offset */

#if defined (ARM_MATH_DSP)
    /* Add offset and store result in destination buffer (2 samples at a time). */
    write_q15x2_ia (&pDst, __QADD16(read_q15x2_ia ((q15_t **) &pSrc), offset_packed));
    write_q15x2_ia (&pDst, __QADD16(read_q15x2_ia ((q15_t **) &pSrc), offset_packed));
#else
    *pDst++ = (q15_t) __SSAT(((q31_t) *pSrc++ + offset), 16);
    *pDst++ = (q15_t) __SSAT(((q31_t) *pSrc++ + offset), 16);
    *pDst++ = (q15_t) __SSAT(((q31_t) *pSrc++ + offset), 16);
    *pDst++ = (q15_t) __SSAT(((q31_t) *pSrc++ + offset), 16);
#endif

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
    /* C = A + offset */

    /* Add offset and store result in destination buffer. */
#if defined (ARM_MATH_DSP)
    *pDst++ = (q15_t) __QADD16(*pSrc++, offset);
#else
    *pDst++ = (q15_t) __SSAT(((q31_t) *pSrc++ + offset), 16);
#endif

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of BasicOffset group
 */
