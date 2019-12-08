/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_offset_q7.c
 * Description:  Q7 vector offset
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
  @brief         Adds a constant offset to a Q7 vector.
  @param[in]     pSrc       points to the input vector
  @param[in]     offset     is the offset to be added
  @param[out]    pDst       points to the output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   Results outside of the allowable Q7 range [0x80 0x7F] are saturated.
 */
#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

void arm_offset_q7(
    const q7_t * pSrc,
    q7_t   offset,
    q7_t * pDst,
    uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q7x16_t vecSrc;

    /* Compute 16 outputs at a time */
    blkCnt = blockSize >> 4;
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
        pSrc += 16;
        pDst += 16;
    }
    /*
     * tail
     */
    blkCnt = blockSize & 0xF;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp8q(blkCnt);
        vecSrc = vld1q(pSrc);
        vstrbq_p(pDst, vqaddq(vecSrc, offset), p0);
    }
}

#else
void arm_offset_q7(
  const q7_t * pSrc,
        q7_t offset,
        q7_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */

#if defined (ARM_MATH_LOOPUNROLL)

#if defined (ARM_MATH_DSP)
  q31_t offset_packed;                           /* Offset packed to 32 bit */

  /* Offset is packed to 32 bit in order to use SIMD32 for addition */
  offset_packed = __PACKq7(offset, offset, offset, offset);
#endif

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A + offset */

#if defined (ARM_MATH_DSP)
    /* Add offset and store result in destination buffer (4 samples at a time). */
    write_q7x4_ia (&pDst, __QADD8(read_q7x4_ia ((q7_t **) &pSrc), offset_packed));
#else
    *pDst++ = (q7_t) __SSAT(*pSrc++ + offset, 8);
    *pDst++ = (q7_t) __SSAT(*pSrc++ + offset, 8);
    *pDst++ = (q7_t) __SSAT(*pSrc++ + offset, 8);
    *pDst++ = (q7_t) __SSAT(*pSrc++ + offset, 8);
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
    *pDst++ = (q7_t) __SSAT((q15_t) *pSrc++ + offset, 8);

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of BasicOffset group
 */
