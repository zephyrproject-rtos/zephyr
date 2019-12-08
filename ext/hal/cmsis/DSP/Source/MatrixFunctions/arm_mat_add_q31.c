/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mat_add_q31.c
 * Description:  Q31 matrix addition
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
  @addtogroup MatrixAdd
  @{
 */

/**
  @brief         Q31 matrix addition.
  @param[in]     pSrcA      points to first input matrix structure
  @param[in]     pSrcB      points to second input matrix structure
  @param[out]    pDst       points to output matrix structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS       : Operation successful
                   - \ref ARM_MATH_SIZE_MISMATCH : Matrix size check failed

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   Results outside of the allowable Q31 range [0x80000000 0x7FFFFFFF] are saturated.
 */
#if defined(ARM_MATH_MVEI)
arm_status arm_mat_add_q31(
  const arm_matrix_instance_q31 * pSrcA,
  const arm_matrix_instance_q31 * pSrcB,
        arm_matrix_instance_q31 * pDst)
{
    arm_status status;                             /* status of matrix addition */
    uint32_t        numSamples;       /* total number of elements in the matrix  */
    q31_t          *pDataA, *pDataB, *pDataDst;
    q31x4_t       vecA, vecB, vecDst;
    q31_t const   *pSrcAVec;
    q31_t const   *pSrcBVec;
    uint32_t        blkCnt;           /* loop counters */

    pDataA = pSrcA->pData;
    pDataB = pSrcB->pData;
    pDataDst = pDst->pData;
    pSrcAVec = (q31_t const *) pDataA;
    pSrcBVec = (q31_t const *) pDataB;

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
        vecDst = vqaddq(vecA, vecB);
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
        pSrcAVec += 4;
        vecB = vld1q(pSrcBVec); 
        pSrcBVec += 4;
        vecDst = vqaddq_m(vecDst, vecA, vecB, p0);
        vstrwq_p(pDataDst, vecDst, p0);
    }
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}

#else
arm_status arm_mat_add_q31(
  const arm_matrix_instance_q31 * pSrcA,
  const arm_matrix_instance_q31 * pSrcB,
        arm_matrix_instance_q31 * pDst)
{
  q31_t *pInA = pSrcA->pData;                    /* input data matrix pointer A */
  q31_t *pInB = pSrcB->pData;                    /* input data matrix pointer B */
  q31_t *pOut = pDst->pData;                     /* output data matrix pointer */

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

      /* Add, saturate and store result in destination buffer. */
      *pOut++ = __QADD(*pInA++, *pInB++);

      *pOut++ = __QADD(*pInA++, *pInB++);

      *pOut++ = __QADD(*pInA++, *pInB++);

      *pOut++ = __QADD(*pInA++, *pInB++);

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

      /* Add, saturate and store result in destination buffer. */
      *pOut++ = __QADD(*pInA++, *pInB++);

      /* Decrement loop counter */
      blkCnt--;
    }

    /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of MatrixAdd group
 */
