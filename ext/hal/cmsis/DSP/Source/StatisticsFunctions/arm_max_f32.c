/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_max_f32.c
 * Description:  Maximum value of a floating-point vector
 *
 * $Date:        18. March 2019
 * $Revision:    V1.6.0
 *
 * Target Processor: Cortex-M and Cortex-A cores
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
#if (defined(ARM_MATH_NEON) || defined(ARM_MATH_MVEF)) && !defined(ARM_MATH_AUTOVECTORIZE)
#include <limits.h>
#endif

/**
  @ingroup groupStats
 */

/**
  @defgroup Max Maximum

  Computes the maximum value of an array of data.
  The function returns both the maximum value and its position within the array.
  There are separate functions for floating-point, Q31, Q15, and Q7 data types.
 */

/**
  @addtogroup Max
  @{
 */

/**
  @brief         Maximum value of a floating-point vector.
  @param[in]     pSrc       points to the input vector
  @param[in]     blockSize  number of samples in input vector
  @param[out]    pResult    maximum value returned here
  @param[out]    pIndex     index of maximum value returned here
  @return        none
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
void arm_max_f32(
  const float32_t * pSrc,
  uint32_t blockSize,
  float32_t * pResult,
  uint32_t * pIndex)
{
    uint32_t blkCnt; 
    f32x4_t vecSrc;
    f32x4_t curExtremValVec = vdupq_n_f32(F32_MIN);
    float32_t maxValue = F32_MIN;
    uint32_t idx = blockSize;
    uint32x4_t indexVec;
    uint32x4_t curExtremIdxVec;
    uint32_t curIdx = 0;
    mve_pred16_t p0;
    float32_t tmp;


    indexVec = vidupq_wb_u32(&curIdx, 1);
    curExtremIdxVec = vdupq_n_u32(0);

    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2U;
    while (blkCnt > 0U)
    {
        vecSrc = vldrwq_f32(pSrc);
        /*
         * Get current max per lane and current index per lane
         * when a max is selected
         */
        p0 = vcmpgeq(vecSrc, curExtremValVec);
        curExtremValVec = vpselq(vecSrc, curExtremValVec, p0);
        curExtremIdxVec = vpselq(indexVec, curExtremIdxVec, p0);

        indexVec = vidupq_wb_u32(&curIdx, 1);

        pSrc += 4;
        /* Decrement the loop counter */
        blkCnt--;
    }


    /*
     * Get max value across the vector
     */
    maxValue = vmaxnmvq(maxValue, curExtremValVec);
    /*
     * set index for lower values to max possible index
     */
    p0 = vcmpgeq(curExtremValVec, maxValue);
    indexVec = vpselq(curExtremIdxVec, vdupq_n_u32(blockSize), p0);
    /*
     * Get min index which is thus for a max value
     */
    idx = vminvq(idx, indexVec);

    /* Tail */
    blkCnt = blockSize & 0x3;

    while (blkCnt > 0U)
    {
      /* Initialize tmp to the next consecutive values one by one */
      tmp = *pSrc++;

      /* compare for the maximum value */
      if (maxValue < tmp)
      {
        /* Update the maximum value and it's index */
        maxValue = tmp;
        idx = blockSize - blkCnt;
      }

      /* Decrement loop counter */
      blkCnt--;
    }

    /*
     * Save result
     */
    *pIndex = idx;
    *pResult = maxValue;
}

#else
#if defined(ARM_MATH_NEON) && !defined(ARM_MATH_AUTOVECTORIZE)
void arm_max_f32(
  const float32_t * pSrc,
  uint32_t blockSize,
  float32_t * pResult,
  uint32_t * pIndex)
{
  float32_t maxVal1, out;               /* Temporary variables to store the output value. */
  uint32_t blkCnt, outIndex;              /* loop counter */

  float32x4_t outV, srcV;
  float32x2_t outV2;

  uint32x4_t idxV;
  uint32x4_t maxIdx;
  static const uint32_t indexInit[4]={4,5,6,7};
  static const uint32_t countVInit[4]={0,1,2,3};

  uint32x4_t index;
  uint32x4_t delta;
  uint32x4_t countV;
  uint32x2_t countV2;

  maxIdx = vdupq_n_u32(ULONG_MAX);
  delta = vdupq_n_u32(4);
  index = vld1q_u32(indexInit);
  countV = vld1q_u32(countVInit);


  /* Initialise the index value to zero. */
  outIndex = 0U;

  /* Load first input value that act as reference value for comparison */
  if (blockSize <= 3)
  {
      out = *pSrc++;

      blkCnt = blockSize - 1;

      while (blkCnt > 0U)
      {
        /* Initialize maxVal to the next consecutive values one by one */
        maxVal1 = *pSrc++;
    
        /* compare for the maximum value */
        if (out < maxVal1)
        {
          /* Update the maximum value and it's index */
          out = maxVal1;
          outIndex = blockSize - blkCnt;
        }
    
        /* Decrement the loop counter */
        blkCnt--;
      }
  }
  else
  {
      outV = vld1q_f32(pSrc);
      pSrc += 4;
 
      /* Compute 4 outputs at a time */
      blkCnt = (blockSize - 4 ) >> 2U;
    
      while (blkCnt > 0U)
      {
        srcV = vld1q_f32(pSrc);
        pSrc += 4;
    
        idxV = vcgtq_f32(srcV, outV);
        outV = vbslq_f32(idxV, srcV, outV );
        countV = vbslq_u32(idxV, index,countV );
    
        index = vaddq_u32(index,delta);
    
        /* Decrement the loop counter */
        blkCnt--;
      }
    
      outV2 = vpmax_f32(vget_low_f32(outV),vget_high_f32(outV));
      outV2 = vpmax_f32(outV2,outV2);
      out = vget_lane_f32(outV2, 0);
    
      idxV = vceqq_f32(outV, vdupq_n_f32(out));
      countV = vbslq_u32(idxV, countV,maxIdx);
      
      countV2 = vpmin_u32(vget_low_u32(countV),vget_high_u32(countV));
      countV2 = vpmin_u32(countV2,countV2);
      outIndex = vget_lane_u32(countV2,0); 
    
      /* if (blockSize - 1U) is not multiple of 4 */
      blkCnt = (blockSize - 4 ) % 4U;
    
      while (blkCnt > 0U)
      {
        /* Initialize maxVal to the next consecutive values one by one */
        maxVal1 = *pSrc++;
    
        /* compare for the maximum value */
        if (out < maxVal1)
        {
          /* Update the maximum value and it's index */
          out = maxVal1;
          outIndex = blockSize - blkCnt ;
        }
    
        /* Decrement the loop counter */
        blkCnt--;
      }
    
      
  }

  /* Store the maximum value and it's index into destination pointers */
  *pResult = out;
  *pIndex = outIndex;
}
#else
void arm_max_f32(
  const float32_t * pSrc,
        uint32_t blockSize,
        float32_t * pResult,
        uint32_t * pIndex)
{
        float32_t maxVal, out;                         /* Temporary variables to store the output value. */
        uint32_t blkCnt, outIndex;                     /* Loop counter */

#if defined (ARM_MATH_LOOPUNROLL) && !defined(ARM_MATH_AUTOVECTORIZE)
        uint32_t index;                                /* index of maximum value */
#endif

  /* Initialise index value to zero. */
  outIndex = 0U;

  /* Load first input value that act as reference value for comparision */
  out = *pSrc++;

#if defined (ARM_MATH_LOOPUNROLL) && !defined(ARM_MATH_AUTOVECTORIZE)
  /* Initialise index of maximum value. */
  index = 0U;

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = (blockSize - 1U) >> 2U;

  while (blkCnt > 0U)
  {
    /* Initialize maxVal to next consecutive values one by one */
    maxVal = *pSrc++;

    /* compare for the maximum value */
    if (out < maxVal)
    {
      /* Update the maximum value and it's index */
      out = maxVal;
      outIndex = index + 1U;
    }

    maxVal = *pSrc++;
    if (out < maxVal)
    {
      out = maxVal;
      outIndex = index + 2U;
    }

    maxVal = *pSrc++;
    if (out < maxVal)
    {
      out = maxVal;
      outIndex = index + 3U;
    }

    maxVal = *pSrc++;
    if (out < maxVal)
    {
      out = maxVal;
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
    /* Initialize maxVal to the next consecutive values one by one */
    maxVal = *pSrc++;

    /* compare for the maximum value */
    if (out < maxVal)
    {
      /* Update the maximum value and it's index */
      out = maxVal;
      outIndex = blockSize - blkCnt;
    }

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Store the maximum value and it's index into destination pointers */
  *pResult = out;
  *pIndex = outIndex;
}
#endif /* #if defined(ARM_MATH_NEON) */
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of Max group
 */
