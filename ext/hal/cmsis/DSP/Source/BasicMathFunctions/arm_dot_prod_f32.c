/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_dot_prod_f32.c
 * Description:  Floating-point dot product
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
  @defgroup BasicDotProd Vector Dot Product

  Computes the dot product of two vectors.
  The vectors are multiplied element-by-element and then summed.

  <pre>
      sum = pSrcA[0]*pSrcB[0] + pSrcA[1]*pSrcB[1] + ... + pSrcA[blockSize-1]*pSrcB[blockSize-1]
  </pre>

  There are separate functions for floating-point, Q7, Q15, and Q31 data types.
 */

/**
  @addtogroup BasicDotProd
  @{
 */

/**
  @brief         Dot product of floating-point vectors.
  @param[in]     pSrcA      points to the first input vector.
  @param[in]     pSrcB      points to the second input vector.
  @param[in]     blockSize  number of samples in each vector.
  @param[out]    result     output result returned here.
  @return        none
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"


void arm_dot_prod_f32(
    const float32_t * pSrcA,
    const float32_t * pSrcB,
    uint32_t    blockSize,
    float32_t * result)
{
    f32x4_t vecA, vecB;
    f32x4_t vecSum;
    uint32_t blkCnt; 
    float32_t sum = 0.0f;  
    vecSum = vdupq_n_f32(0.0f);

    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2U;
    while (blkCnt > 0U)
    {
        /*
         * C = A[0]* B[0] + A[1]* B[1] + A[2]* B[2] + .....+ A[blockSize-1]* B[blockSize-1]
         * Calculate dot product and then store the result in a temporary buffer.
         * and advance vector source and destination pointers
         */
        vecA = vld1q(pSrcA);
        pSrcA += 4;
        
        vecB = vld1q(pSrcB);
        pSrcB += 4;

        vecSum = vfmaq(vecSum, vecA, vecB);
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt --;
    }


    blkCnt = blockSize & 3;
    if (blkCnt > 0U)
    {
        /* C = A[0]* B[0] + A[1]* B[1] + A[2]* B[2] + .....+ A[blockSize-1]* B[blockSize-1] */

        mve_pred16_t p0 = vctp32q(blkCnt);
        vecA = vld1q(pSrcA);
        vecB = vld1q(pSrcB);
        vecSum = vfmaq_m(vecSum, vecA, vecB, p0);
    }

    sum = vecAddAcrossF32Mve(vecSum);

    /* Store result in destination buffer */
    *result = sum;

}

#else

void arm_dot_prod_f32(
  const float32_t * pSrcA,
  const float32_t * pSrcB,
        uint32_t blockSize,
        float32_t * result)
{
        uint32_t blkCnt;                               /* Loop counter */
        float32_t sum = 0.0f;                          /* Temporary return variable */

#if defined(ARM_MATH_NEON) && !defined(ARM_MATH_AUTOVECTORIZE)
    f32x4_t vec1;
    f32x4_t vec2;
    f32x4_t accum = vdupq_n_f32(0);   
    f32x2_t tmp = vdup_n_f32(0);    

    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2U;

    vec1 = vld1q_f32(pSrcA);
    vec2 = vld1q_f32(pSrcB);

    while (blkCnt > 0U)
    {
        /* C = A[0]*B[0] + A[1]*B[1] + A[2]*B[2] + ... + A[blockSize-1]*B[blockSize-1] */
        /* Calculate dot product and then store the result in a temporary buffer. */
        
	      accum = vmlaq_f32(accum, vec1, vec2);
	
        /* Increment pointers */
        pSrcA += 4;
        pSrcB += 4; 

        vec1 = vld1q_f32(pSrcA);
        vec2 = vld1q_f32(pSrcB);
        
        /* Decrement the loop counter */
        blkCnt--;
    }
    
#if __aarch64__
    sum = vpadds_f32(vpadd_f32(vget_low_f32(accum), vget_high_f32(accum)));
#else
    tmp = vpadd_f32(vget_low_f32(accum), vget_high_f32(accum));
    sum = vget_lane_f32(tmp, 0) + vget_lane_f32(tmp, 1);

#endif    

    /* Tail */
    blkCnt = blockSize & 0x3;

#else
#if defined (ARM_MATH_LOOPUNROLL) && !defined(ARM_MATH_AUTOVECTORIZE)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  /* First part of the processing with loop unrolling. Compute 4 outputs at a time.
   ** a second loop below computes the remaining 1 to 3 samples. */
  while (blkCnt > 0U)
  {
    /* C = A[0]* B[0] + A[1]* B[1] + A[2]* B[2] + .....+ A[blockSize-1]* B[blockSize-1] */

    /* Calculate dot product and store result in a temporary buffer. */
    sum += (*pSrcA++) * (*pSrcB++);

    sum += (*pSrcA++) * (*pSrcB++);

    sum += (*pSrcA++) * (*pSrcB++);

    sum += (*pSrcA++) * (*pSrcB++);

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = blockSize % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */
#endif /* #if defined(ARM_MATH_NEON) */

  while (blkCnt > 0U)
  {
    /* C = A[0]* B[0] + A[1]* B[1] + A[2]* B[2] + .....+ A[blockSize-1]* B[blockSize-1] */

    /* Calculate dot product and store result in a temporary buffer. */
    sum += (*pSrcA++) * (*pSrcB++);

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Store result in destination buffer */
  *result = sum;
}

#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */
/**
  @} end of BasicDotProd group
 */
