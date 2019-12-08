/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mat_add_f32.c
 * Description:  Floating-point matrix addition
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
  @ingroup groupMatrix
 */

/**
  @defgroup MatrixAdd Matrix Addition

  Adds two matrices.
  \image html MatrixAddition.gif "Addition of two 3 x 3 matrices"

  The functions check to make sure that
  <code>pSrcA</code>, <code>pSrcB</code>, and <code>pDst</code> have the same
  number of rows and columns.
 */

/**
  @addtogroup MatrixAdd
  @{
 */


/**
  @brief         Floating-point matrix addition.
  @param[in]     pSrcA      points to first input matrix structure
  @param[in]     pSrcB      points to second input matrix structure
  @param[out]    pDst       points to output matrix structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS       : Operation successful
                   - \ref ARM_MATH_SIZE_MISMATCH : Matrix size check failed
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
arm_status arm_mat_add_f32(
  const arm_matrix_instance_f32 * pSrcA,
  const arm_matrix_instance_f32 * pSrcB,
  arm_matrix_instance_f32 * pDst)
{
    arm_status status;  
    uint32_t  numSamples;       /* total number of elements in the matrix  */
    float32_t *pDataA, *pDataB, *pDataDst;
    f32x4_t vecA, vecB, vecDst;
    float32_t const *pSrcAVec;
    float32_t const *pSrcBVec;
    uint32_t  blkCnt;           /* loop counters */

    pDataA = pSrcA->pData;
    pDataB = pSrcB->pData;
    pDataDst = pDst->pData;
    pSrcAVec = (float32_t const *) pDataA;
    pSrcBVec = (float32_t const *) pDataB;

#ifdef ARM_MATH_MATRIX_CHECK
  /* Check for matrix mismatch condition */
  if ((pSrcA->numRows != pSrcB->numRows) ||
     (pSrcA->numCols != pSrcB->numCols) ||
     (pSrcA->numRows != pDst->numRows) || (pSrcA->numCols != pDst->numCols))
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else
#endif
 {
    /*
     * Total number of samples in the input matrix
     */
    numSamples = (uint32_t) pSrcA->numRows * pSrcA->numCols;
    blkCnt = numSamples >> 2;
    while (blkCnt > 0U)
    {
        /* C(m,n) = A(m,n) + B(m,n) */
        /* Add and then store the results in the destination buffer. */
        vecA = vld1q(pSrcAVec); 
        pSrcAVec += 4;
        vecB = vld1q(pSrcBVec); 
        pSrcBVec += 4;
        vecDst = vaddq(vecA, vecB);
        vst1q(pDataDst, vecDst);  
        pDataDst += 4;
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
    }
    /*
     * tail
     */
    blkCnt = numSamples & 3;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp32q(blkCnt);
        vecA = vld1q(pSrcAVec); 
        vecB = vld1q(pSrcBVec); 
        vecDst = vaddq_m(vecDst, vecA, vecB, p0);
        vstrwq_p(pDataDst, vecDst, p0);
    }
    /* set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }
  return (status);
}
#else
#if defined(ARM_MATH_NEON)
/*

Neon version is assuming the matrix is small enough.
So no blocking is used for taking into account cache effects.
For big matrix, there exist better libraries for Neon.

*/
arm_status arm_mat_add_f32(
  const arm_matrix_instance_f32 * pSrcA,
  const arm_matrix_instance_f32 * pSrcB,
  arm_matrix_instance_f32 * pDst)
{
  float32_t *pIn1 = pSrcA->pData;                /* input data matrix pointer A  */
  float32_t *pIn2 = pSrcB->pData;                /* input data matrix pointer B  */
  float32_t *pOut = pDst->pData;                 /* output data matrix pointer   */


  uint32_t numSamples;                           /* total number of elements in the matrix  */
  uint32_t blkCnt;                               /* loop counters */
  arm_status status;                             /* status of matrix addition */

#ifdef ARM_MATH_MATRIX_CHECK
  /* Check for matrix mismatch condition */
  if ((pSrcA->numRows != pSrcB->numRows) ||
     (pSrcA->numCols != pSrcB->numCols) ||
     (pSrcA->numRows != pDst->numRows) || (pSrcA->numCols != pDst->numCols))
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else
#endif
  {
    float32x4_t vec1;
    float32x4_t vec2;
    float32x4_t res;

    /* Total number of samples in the input matrix */
    numSamples = (uint32_t) pSrcA->numRows * pSrcA->numCols;

    blkCnt = numSamples >> 2U;

    /* Compute 4 outputs at a time.
     ** a second loop below computes the remaining 1 to 3 samples. */
    while (blkCnt > 0U)
    {
      /* C(m,n) = A(m,n) + B(m,n) */
      /* Add and then store the results in the destination buffer. */
      vec1 = vld1q_f32(pIn1);
      vec2 = vld1q_f32(pIn2);
      res = vaddq_f32(vec1, vec2);
      vst1q_f32(pOut, res);

      /* update pointers to process next samples */
      pIn1 += 4U;
      pIn2 += 4U;
      pOut += 4U;
      /* Decrement the loop counter */
      blkCnt--;
    }

    /* If the numSamples is not a multiple of 4, compute any remaining output samples here.
     ** No loop unrolling is used. */
    blkCnt = numSamples % 0x4U;

    while (blkCnt > 0U)
    {
      /* C(m,n) = A(m,n) + B(m,n) */
      /* Add and then store the results in the destination buffer. */
      *pOut++ = (*pIn1++) + (*pIn2++);

      /* Decrement the loop counter */
      blkCnt--;
    }

    /* set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}
#else
arm_status arm_mat_add_f32(
  const arm_matrix_instance_f32 * pSrcA,
  const arm_matrix_instance_f32 * pSrcB,
        arm_matrix_instance_f32 * pDst)
{
  float32_t *pInA = pSrcA->pData;                /* input data matrix pointer A */
  float32_t *pInB = pSrcB->pData;                /* input data matrix pointer B */
  float32_t *pOut = pDst->pData;                 /* output data matrix pointer */

  uint32_t numSamples;                           /* total number of elements in the matrix */
  uint32_t blkCnt;                               /* loop counters */
  arm_status status;                             /* status of matrix addition */

#ifdef ARM_MATH_MATRIX_CHECK

  /* Check for matrix mismatch condition */
  if ((pSrcA->numRows != pSrcB->numRows) ||
      (pSrcA->numCols != pSrcB->numCols) ||
      (pSrcA->numRows != pDst->numRows)  ||
      (pSrcA->numCols != pDst->numCols)    )
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else

#endif /* #ifdef ARM_MATH_MATRIX_CHECK */

  {
    /* Total number of samples in input matrix */
    numSamples = (uint32_t) pSrcA->numRows * pSrcA->numCols;

#if defined (ARM_MATH_LOOPUNROLL)

    /* Loop unrolling: Compute 4 outputs at a time */
    blkCnt = numSamples >> 2U;

    while (blkCnt > 0U)
    {
      /* C(m,n) = A(m,n) + B(m,n) */

      /* Add and store result in destination buffer. */
      *pOut++ = *pInA++ + *pInB++;

      *pOut++ = *pInA++ + *pInB++;

      *pOut++ = *pInA++ + *pInB++;

      *pOut++ = *pInA++ + *pInB++;

      /* Decrement loop counter */
      blkCnt--;
    }

    /* Loop unrolling: Compute remaining outputs */
    blkCnt = numSamples % 0x4U;

#else

    /* Initialize blkCnt with number of samples */
    blkCnt = numSamples;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

    while (blkCnt > 0U)
    {
      /* C(m,n) = A(m,n) + B(m,n) */

      /* Add and store result in destination buffer. */
      *pOut++ = *pInA++ + *pInB++;

      /* Decrement loop counter */
      blkCnt--;
    }

    /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}
#endif /* #if defined(ARM_MATH_NEON) */
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of MatrixAdd group
 */
