/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_min_q31.c
 * Description:  Minimum value of a Q31 vector
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
  @brief         Minimum value of a Q31 vector.
  @param[in]     pSrc       points to the input vector
  @param[in]     blockSize  number of samples in input vector
  @param[out]    pResult    minimum value returned here
  @param[out]    pIndex     index of minimum value returned here
  @return        none
 */
#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

void arm_min_q31(
  const q31_t * pSrc,
        uint32_t blockSize,
        q31_t * pResult,
        uint32_t * pIndex)
{
    uint32_t  blkCnt;           /* loop counters */
    q31x4_t vecSrc;
    q31x4_t curExtremValVec = vdupq_n_s32(Q31_MAX);
    q31_t minValue = Q31_MAX, temp;
    uint32_t  idx = blockSize;
    uint32x4_t indexVec;
    uint32x4_t curExtremIdxVec;
    mve_pred16_t p0;


    indexVec = vidupq_u32(0, 1);
    curExtremIdxVec = vdupq_n_u32(0);

    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2U;
    while (blkCnt > 0U)
    {
        vecSrc = vldrwq_s32(pSrc);  
        pSrc += 4;
        /*
         * Get current min per lane and current index per lane
         * when a min is selected
         */
        p0 = vcmpleq(vecSrc, curExtremValVec);
        curExtremValVec = vpselq(vecSrc, curExtremValVec, p0);
        curExtremIdxVec = vpselq(indexVec, curExtremIdxVec, p0);

        indexVec = indexVec +  4;
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
    indexVec = vpselq(curExtremIdxVec, vdupq_n_u32(blockSize), p0);
    /*
     * Get min index which is thus for a min value
     */
    idx = vminvq(idx, indexVec);


    /* Tail */
    blkCnt = blockSize & 0x3;
    while (blkCnt > 0U)
    {
      /* Initialize temp to the next consecutive values one by one */
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
#else
void arm_min_q31(
  const q31_t * pSrc,
        uint32_t blockSize,
        q31_t * pResult,
        uint32_t * pIndex)
{
        q31_t minVal, out;                             /* Temporary variables to store the output value. */
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
