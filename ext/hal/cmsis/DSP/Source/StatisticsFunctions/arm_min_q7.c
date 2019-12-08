/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_min_q7.c
 * Description:  Minimum value of a Q7 vector
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
  @ingroup groupStats
 */


/**
  @addtogroup Min
  @{
 */

/**
  @brief         Minimum value of a Q7 vector.
  @param[in]     pSrc       points to the input vector
  @param[in]     blockSize  number of samples in input vector
  @param[out]    pResult    minimum value returned here
  @param[out]    pIndex     index of minimum value returned here
  @return        none
 */
#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

static void arm_small_blk_min_q7(
    const q7_t * pSrc,
    uint8_t blockSize,
    q7_t * pResult,
    uint32_t * pIndex)
{
    uint32_t  blkCnt;           /* loop counters */
    q7x16_t        vecSrc;
    q7x16_t        curExtremValVec = vdupq_n_s8(Q7_MAX);
    q7_t            minValue = Q7_MAX,temp;
    uint32_t        idx = blockSize;
    uint8x16_t      indexVec;
    uint8x16_t      curExtremIdxVec;
    mve_pred16_t    p0;


    indexVec = vidupq_u8(0, 1);
    curExtremIdxVec = vdupq_n_u8(0);

    blkCnt = blockSize >> 4;
    while (blkCnt > 0U)
    {
        vecSrc = vldrbq_s8(pSrc);  
        pSrc += 16;
        /*
         * Get current min per lane and current index per lane
         * when a min is selected
         */
        p0 = vcmpleq(vecSrc, curExtremValVec);
        curExtremValVec = vpselq(vecSrc, curExtremValVec, p0);
        curExtremIdxVec = vpselq(indexVec, curExtremIdxVec, p0);

        indexVec = indexVec +  16;
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
    }
    
    /*
     * Get min value across the vector
     */
    minValue = vminvq(minValue, curExtremValVec);
    /*
     * set index for lower values to min possible index
     */
    p0 = vcmpleq(curExtremValVec, minValue);
    indexVec = vpselq(curExtremIdxVec, vdupq_n_u8(blockSize), p0);
    /*
     * Get min index which is thus for a min value
     */
    idx = vminvq(idx, indexVec);

    blkCnt = blockSize & 0xF;
    while (blkCnt > 0U)
    {
      /* Initialize minVal to the next consecutive values one by one */
      temp = *pSrc++;
  
      /* compare for the minimum value */
      if (minValue > temp)
      {
        /* Update the minimum value and it's index */
        minValue = temp;
        idx = blockSize - blkCnt;
      }
  
      /* Decrement loop counter */
      blkCnt--;
    }
    /*
     * Save result
     */
    *pIndex = idx;
    *pResult = minValue;
}

void arm_min_q7(
  const q7_t * pSrc,
        uint32_t blockSize,
        q7_t * pResult,
        uint32_t * pIndex)
{
    int32_t   totalSize = blockSize;

    if (totalSize <= UINT8_MAX)
    {
        arm_small_blk_min_q7(pSrc, blockSize, pResult, pIndex);
    }
    else
    {
        uint32_t  curIdx = 0;
        q7_t      curBlkExtr = Q7_MAX;
        uint32_t  curBlkPos = 0;
        uint32_t  curBlkIdx = 0;
        /*
         * process blocks of 255 elts
         */
        while (totalSize >= UINT8_MAX)
        {
            const q7_t     *curSrc = pSrc;

            arm_small_blk_min_q7(curSrc, UINT8_MAX, pResult, pIndex);
            if (*pResult < curBlkExtr)
            {
                /*
                 * update partial extrema
                 */
                curBlkExtr = *pResult;
                curBlkPos = *pIndex;
                curBlkIdx = curIdx;
            }
            curIdx++;
            pSrc += UINT8_MAX;
            totalSize -= UINT8_MAX;
        }
        /*
         * remainder
         */
        arm_small_blk_min_q7(pSrc, totalSize, pResult, pIndex);
        if (*pResult < curBlkExtr)
        {
            curBlkExtr = *pResult;
            curBlkPos = *pIndex;
            curBlkIdx = curIdx;
        }
        *pIndex = curBlkIdx * UINT8_MAX + curBlkPos;
        *pResult = curBlkExtr;
    }
}
#else
void arm_min_q7(
  const q7_t * pSrc,
        uint32_t blockSize,
        q7_t * pResult,
        uint32_t * pIndex)
{
        q7_t minVal, out;                              /* Temporary variables to store the output value. */
        uint32_t blkCnt, outIndex;                     /* Loop counter */

#if defined (ARM_MATH_LOOPUNROLL)
        uint32_t index;                                /* index of maximum value */
#endif

  /* Initialise index value to zero. */
  outIndex = 0U;
  /* Load first input value that act as reference value for comparision */
  out = *pSrc++;

#if defined (ARM_MATH_LOOPUNROLL)
  /* Initialise index of maximum value. */
  index = 0U;

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = (blockSize - 1U) >> 2U;

  while (blkCnt > 0U)
  {
    /* Initialize minVal to next consecutive values one by one */
    minVal = *pSrc++;

    /* compare for the minimum value */
    if (out > minVal)
    {
      /* Update the minimum value and it's index */
      out = minVal;
      outIndex = index + 1U;
    }

    minVal = *pSrc++;
    if (out > minVal)
    {
      out = minVal;
      outIndex = index + 2U;
    }

    minVal = *pSrc++;
    if (out > minVal)
    {
      out = minVal;
      outIndex = index + 3U;
    }

    minVal = *pSrc++;
    if (out > minVal)
    {
      out = minVal;
      outIndex = index + 4U;
    }

    index += 4U;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = (blockSize - 1U) % 4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = (blockSize - 1U);

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* Initialize minVal to the next consecutive values one by one */
    minVal = *pSrc++;

    /* compare for the minimum value */
    if (out > minVal)
    {
      /* Update the minimum value and it's index */
      out = minVal;
      outIndex = blockSize - blkCnt;
    }

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Store the minimum value and it's index into destination pointers */
  *pResult = out;
  *pIndex = outIndex;
}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of Min group
 */
