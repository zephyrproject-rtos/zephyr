/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mat_mult_q15.c
 * Description:  Q15 matrix multiplication
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
  @addtogroup MatrixMult
  @{
 */

/**
  @brief         Q15 matrix multiplication.
  @param[in]     pSrcA      points to the first input matrix structure
  @param[in]     pSrcB      points to the second input matrix structure
  @param[out]    pDst       points to output matrix structure
  @param[in]     pState     points to the array for storing intermediate results (Unused)
  @return        execution status
                   - \ref ARM_MATH_SUCCESS       : Operation successful
                   - \ref ARM_MATH_SIZE_MISMATCH : Matrix size check failed

  @par           Scaling and Overflow Behavior
                   The function is implemented using an internal 64-bit accumulator. The inputs to the
                   multiplications are in 1.15 format and multiplications yield a 2.30 result.
                   The 2.30 intermediate results are accumulated in a 64-bit accumulator in 34.30 format.
                   This approach provides 33 guard bits and there is no risk of overflow.
                   The 34.30 result is then truncated to 34.15 format by discarding the low 15 bits
                   and then saturated to 1.15 format.
  @par
                   Refer to \ref arm_mat_mult_fast_q15() for a faster but less precise version of this function.
 */
#if defined(ARM_MATH_MVEI)

#define MVE_ASRL_SAT16(acc, shift)          ((sqrshrl_sat48(acc, -(32-shift)) >> 32) & 0xffffffff)

#define MATRIX_DIM2 2
#define MATRIX_DIM3 3
#define MATRIX_DIM4 4

__STATIC_INLINE arm_status arm_mat_mult_q15_2x2_mve(
    const arm_matrix_instance_q15 * pSrcA,
    const arm_matrix_instance_q15 * pSrcB,
    arm_matrix_instance_q15 * pDst)
{
    q15_t       *pInB = pSrcB->pData;  /* input data matrix pointer B */
    q15_t       *pInA = pSrcA->pData;  /* input data matrix pointer A */
    q15_t       *pOut = pDst->pData;   /* output data matrix pointer */
    uint16x8_t  vecColBOffs;
    q15_t       *pInA0 = pInA;
    q15_t       *pInA1 = pInA0 + MATRIX_DIM2;
    q63_t        acc0, acc1;
    q15x8_t     vecB, vecA0, vecA1;
    mve_pred16_t p0 = vctp16q(MATRIX_DIM2);

    vecColBOffs = vidupq_u16(0, 2); /* MATRIX_DIM2 */

    pInB = pSrcB->pData;

    vecB = vldrhq_gather_shifted_offset_z_s16((q15_t const *)pInB, vecColBOffs, p0);

    vecA0 = vldrhq_s16(pInA0);
    vecA1 = vldrhq_s16(pInA1);

    acc0 = vmlaldavq(vecA0, vecB);
    acc1 = vmlaldavq(vecA1, vecB);

    acc0 = asrl(acc0, 15);
    acc1 = asrl(acc1, 15);

    pOut[0 * MATRIX_DIM2] = (q15_t) __SSAT(acc0, 16);
    pOut[1 * MATRIX_DIM2] = (q15_t) __SSAT(acc1, 16);
    pOut++;

    /* move to next B column */
    pInB = pInB + 1;

    vecB = vldrhq_gather_shifted_offset_z_s16(pInB, vecColBOffs, p0);

    acc0 = vmlaldavq(vecA0, vecB);
    acc1 = vmlaldavq(vecA1, vecB);

    acc0 = asrl(acc0, 15);
    acc1 = asrl(acc1, 15);

    pOut[0 * MATRIX_DIM2] = (q15_t) __SSAT(acc0, 16);
    pOut[1 * MATRIX_DIM2] = (q15_t) __SSAT(acc1, 16);

    /*
     * Return to application
     */
    return (ARM_MATH_SUCCESS);
}



__STATIC_INLINE arm_status arm_mat_mult_q15_3x3_mve(
    const arm_matrix_instance_q15 * pSrcA,
    const arm_matrix_instance_q15 * pSrcB,
    arm_matrix_instance_q15 * pDst)
{
    q15_t       *pInB = pSrcB->pData;  /* input data matrix pointer B */
    q15_t       *pInA = pSrcA->pData;  /* input data matrix pointer A */
    q15_t       *pOut = pDst->pData;   /* output data matrix pointer */
    uint16x8_t vecColBOffs;
    q15_t       *pInA0 = pInA;
    q15_t       *pInA1 = pInA0 + MATRIX_DIM3;
    q15_t       *pInA2 = pInA1 + MATRIX_DIM3;
    q63_t        acc0, acc1, acc2;
    q15x8_t    vecB, vecA0, vecA1, vecA2;
    mve_pred16_t p0 = vctp16q(MATRIX_DIM3);

    vecColBOffs = vidupq_u16(0, 1);
    vecColBOffs = vecColBOffs * MATRIX_DIM3;

    pInB = pSrcB->pData;

    vecB = vldrhq_gather_shifted_offset_z_s16((q15_t const *)pInB, vecColBOffs, p0);

    vecA0 = vldrhq_s16(pInA0);
    vecA1 = vldrhq_s16(pInA1);
    vecA2 = vldrhq_s16(pInA2);

    acc0 = vmlaldavq(vecA0, vecB);
    acc1 = vmlaldavq(vecA1, vecB);
    acc2 = vmlaldavq(vecA2, vecB);

    acc0 = asrl(acc0, 15);
    acc1 = asrl(acc1, 15);
    acc2 = asrl(acc2, 15);

    pOut[0 * MATRIX_DIM3] = (q15_t) __SSAT(acc0, 16);
    pOut[1 * MATRIX_DIM3] = (q15_t) __SSAT(acc1, 16);
    pOut[2 * MATRIX_DIM3] = (q15_t) __SSAT(acc2, 16);
    pOut++;

    /* move to next B column */
    pInB = pInB + 1;

    vecB = vldrhq_gather_shifted_offset_z_s16(pInB, vecColBOffs, p0);

    acc0 = vmlaldavq(vecA0, vecB);
    acc1 = vmlaldavq(vecA1, vecB);
    acc2 = vmlaldavq(vecA2, vecB);

    acc0 = asrl(acc0, 15);
    acc1 = asrl(acc1, 15);
    acc2 = asrl(acc2, 15);

    pOut[0 * MATRIX_DIM3] = (q15_t) __SSAT(acc0, 16);
    pOut[1 * MATRIX_DIM3] = (q15_t) __SSAT(acc1, 16);
    pOut[2 * MATRIX_DIM3] = (q15_t) __SSAT(acc2, 16);
    pOut++;

    /* move to next B column */
    pInB = pInB + 1;

    vecB = vldrhq_gather_shifted_offset_z_s16(pInB, vecColBOffs, p0);

    acc0 = vmlaldavq(vecA0, vecB);
    acc1 = vmlaldavq(vecA1, vecB);
    acc2 = vmlaldavq(vecA2, vecB);

    acc0 = asrl(acc0, 15);
    acc1 = asrl(acc1, 15);
    acc2 = asrl(acc2, 15);

    pOut[0 * MATRIX_DIM3] = (q15_t) __SSAT(acc0, 16);
    pOut[1 * MATRIX_DIM3] = (q15_t) __SSAT(acc1, 16);
    pOut[2 * MATRIX_DIM3] = (q15_t) __SSAT(acc2, 16);
    /*
     * Return to application
     */
    return (ARM_MATH_SUCCESS);
}


__STATIC_INLINE arm_status arm_mat_mult_q15_4x4_mve(
    const arm_matrix_instance_q15 * pSrcA,
    const arm_matrix_instance_q15 * pSrcB,
    arm_matrix_instance_q15 * pDst)
{
    q15_t       *pInB = pSrcB->pData;  /* input data matrix pointer B */
    q15_t       *pInA = pSrcA->pData;  /* input data matrix pointer A */
    q15_t       *pOut = pDst->pData;   /* output data matrix pointer */
    uint16x8_t vecColBOffs;
    q15_t       *pInA0 = pInA;
    q15_t       *pInA1 = pInA0 + MATRIX_DIM4;
    q15_t       *pInA2 = pInA1 + MATRIX_DIM4;
    q15_t       *pInA3 = pInA2 + MATRIX_DIM4;
    q63_t        acc0, acc1, acc2, acc3;
    q15x8_t     vecB, vecA0, vecA1, vecA2, vecA3;
    mve_pred16_t p0 = vctp16q(MATRIX_DIM4);

    vecColBOffs = vidupq_u16(0, 4);

    pInB = pSrcB->pData;

    vecB = vldrhq_gather_shifted_offset_z_s16((q15_t const *)pInB, vecColBOffs, p0);

    vecA0 = vldrhq_s16(pInA0);
    vecA1 = vldrhq_s16(pInA1);
    vecA2 = vldrhq_s16(pInA2);
    vecA3 = vldrhq_s16(pInA3);

    acc0 = vmlaldavq(vecA0, vecB);
    acc1 = vmlaldavq(vecA1, vecB);
    acc2 = vmlaldavq(vecA2, vecB);
    acc3 = vmlaldavq(vecA3, vecB);

    acc0 = asrl(acc0, 15);
    acc1 = asrl(acc1, 15);
    acc2 = asrl(acc2, 15);
    acc3 = asrl(acc3, 15);

    pOut[0 * MATRIX_DIM4] = (q15_t) __SSAT(acc0, 16);
    pOut[1 * MATRIX_DIM4] = (q15_t) __SSAT(acc1, 16);
    pOut[2 * MATRIX_DIM4] = (q15_t) __SSAT(acc2, 16);
    pOut[3 * MATRIX_DIM4] = (q15_t) __SSAT(acc3, 16);
    pOut++;

    /* move to next B column */
    pInB = pInB + 1;

    vecB = vldrhq_gather_shifted_offset_z_s16(pInB, vecColBOffs, p0);

    acc0 = vmlaldavq(vecA0, vecB);
    acc1 = vmlaldavq(vecA1, vecB);
    acc2 = vmlaldavq(vecA2, vecB);
    acc3 = vmlaldavq(vecA3, vecB);

    acc0 = asrl(acc0, 15);
    acc1 = asrl(acc1, 15);
    acc2 = asrl(acc2, 15);
    acc3 = asrl(acc3, 15);

    pOut[0 * MATRIX_DIM4] = (q15_t) __SSAT(acc0, 16);
    pOut[1 * MATRIX_DIM4] = (q15_t) __SSAT(acc1, 16);
    pOut[2 * MATRIX_DIM4] = (q15_t) __SSAT(acc2, 16);
    pOut[3 * MATRIX_DIM4] = (q15_t) __SSAT(acc3, 16);

    pOut++;

    /* move to next B column */
    pInB = pInB + 1;

    vecB = vldrhq_gather_shifted_offset_z_s16(pInB, vecColBOffs, p0);

    acc0 = vmlaldavq(vecA0, vecB);
    acc1 = vmlaldavq(vecA1, vecB);
    acc2 = vmlaldavq(vecA2, vecB);
    acc3 = vmlaldavq(vecA3, vecB);

    acc0 = asrl(acc0, 15);
    acc1 = asrl(acc1, 15);
    acc2 = asrl(acc2, 15);
    acc3 = asrl(acc3, 15);

    pOut[0 * MATRIX_DIM4] = (q15_t) __SSAT(acc0, 16);
    pOut[1 * MATRIX_DIM4] = (q15_t) __SSAT(acc1, 16);
    pOut[2 * MATRIX_DIM4] = (q15_t) __SSAT(acc2, 16);
    pOut[3 * MATRIX_DIM4] = (q15_t) __SSAT(acc3, 16);

    pOut++;

    /* move to next B column */
    pInB = pInB + 1;

    vecB = vldrhq_gather_shifted_offset_z_s16(pInB, vecColBOffs, p0);

    acc0 = vmlaldavq(vecA0, vecB);
    acc1 = vmlaldavq(vecA1, vecB);
    acc2 = vmlaldavq(vecA2, vecB);
    acc3 = vmlaldavq(vecA3, vecB);

    acc0 = asrl(acc0, 15);
    acc1 = asrl(acc1, 15);
    acc2 = asrl(acc2, 15);
    acc3 = asrl(acc3, 15);

    pOut[0 * MATRIX_DIM4] = (q15_t) __SSAT(acc0, 16);
    pOut[1 * MATRIX_DIM4] = (q15_t) __SSAT(acc1, 16);
    pOut[2 * MATRIX_DIM4] = (q15_t) __SSAT(acc2, 16);
    pOut[3 * MATRIX_DIM4] = (q15_t) __SSAT(acc3, 16);
    /*
     * Return to application
     */
    return (ARM_MATH_SUCCESS);
}

arm_status arm_mat_mult_q15(
  const arm_matrix_instance_q15 * pSrcA,
  const arm_matrix_instance_q15 * pSrcB,
        arm_matrix_instance_q15 * pDst,
        q15_t                   * pState)
{
    q15_t    *pInB = pSrcB->pData;  /* input data matrix pointer B */
    q15_t    *pInA = pSrcA->pData;  /* input data matrix pointer A */
    q15_t    *pOut = pDst->pData;   /* output data matrix pointer */
    q15_t    *px;               /* Temporary output data matrix pointer */
    uint16_t  numRowsA = pSrcA->numRows;    /* number of rows of input matrix A    */
    uint16_t  numColsB = pSrcB->numCols;    /* number of columns of input matrix B */
    uint16_t  numColsA = pSrcA->numCols;    /* number of columns of input matrix A */
    uint16_t  col, i = 0U, row = numRowsA, colCnt;  /* loop counters */
    uint16x8_t vecOffs, vecColBOffs;
    uint32_t  blkCnt,rowCnt;           /* loop counters */
    arm_status status;                             /* Status of matrix multiplication */
    (void)pState;

#ifdef ARM_MATH_MATRIX_CHECK

  /* Check for matrix mismatch condition */
  if ((pSrcA->numCols != pSrcB->numRows) ||
      (pSrcA->numRows != pDst->numRows)  ||
      (pSrcB->numCols != pDst->numCols)    )
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else
#endif 
  {
    /* small squared matrix specialized routines */
    if(numRowsA == numColsB && numColsB == numColsA) {

        if (numRowsA == 1)
        {
           q63_t sum;
           sum = pInA[0] * pInB[0];
           pOut[0] = (q15_t) __SSAT((sum >> 15), 16);
           return (ARM_MATH_SUCCESS);
        }
        else if(numRowsA == 2)
            return arm_mat_mult_q15_2x2_mve(pSrcA, pSrcB, pDst);
        else if(numRowsA == 3)
            return arm_mat_mult_q15_3x3_mve(pSrcA, pSrcB, pDst);
        else if (numRowsA == 4)
            return arm_mat_mult_q15_4x4_mve(pSrcA, pSrcB, pDst);
    }

    vecColBOffs = vidupq_u16(0, 1);
    vecColBOffs = vecColBOffs * (uint16_t) (numColsB);

    /*
     * The following loop performs the dot-product of each row in pSrcA with each column in pSrcB
     */

    /*
     * row loop
     */
    rowCnt = row >> 2;
    while (rowCnt > 0U)
    {
        /*
         * Output pointer is set to starting address of the row being processed
         */
        px = pOut + i;
        i = i + 4 * numColsB;
        /*
         * For every row wise process, the column loop counter is to be initiated
         */
        col = numColsB;
        /*
         * For every row wise process, the pInB pointer is set
         * to the starting address of the pSrcB data
         */
        pInB = pSrcB->pData;
        /*
         * column loop
         */
        while (col > 0U)
        {
            /*
             * generate 4 columns elements
             */
            /*
             * Matrix A columns number of MAC operations are to be performed
             */
            colCnt = numColsA;

            q15_t const *pSrcA0Vec, *pSrcA1Vec, *pSrcA2Vec, *pSrcA3Vec;
            q15_t    *pInA0 = pInA;
            q15_t    *pInA1 = pInA0 + numColsA;
            q15_t    *pInA2 = pInA1 + numColsA;
            q15_t    *pInA3 = pInA2 + numColsA;
            q63_t     acc0, acc1, acc2, acc3;

            acc0 = 0LL;
            acc1 = 0LL;
            acc2 = 0LL;
            acc3 = 0LL;

            pSrcA0Vec = (q15_t const *) pInA0;
            pSrcA1Vec = (q15_t const *) pInA1;
            pSrcA2Vec = (q15_t const *) pInA2;
            pSrcA3Vec = (q15_t const *) pInA3;

            vecOffs = vecColBOffs;

            blkCnt = (numColsA) >> 3;
            while (blkCnt > 0U)
            {
                q15x8_t vecB, vecA;

                vecB = vldrhq_gather_shifted_offset((int16_t const *)pInB, vecOffs);
                vecOffs = vecOffs + (uint16_t) (numColsB * 8);

                vecA = vld1q(pSrcA0Vec);  pSrcA0Vec += 8;
                acc0 = vmlaldavaq(acc0, vecA, vecB);
                vecA = vld1q(pSrcA1Vec);  pSrcA1Vec += 8;
                acc1 = vmlaldavaq(acc1, vecA, vecB);
                vecA = vld1q(pSrcA2Vec);  pSrcA2Vec += 8;
                acc2 = vmlaldavaq(acc2, vecA, vecB);
                vecA = vld1q(pSrcA3Vec);  pSrcA3Vec += 8;
                acc3 = vmlaldavaq(acc3, vecA, vecB);
                blkCnt--;

            }
            /*
             * tail
             */
            blkCnt = numColsA & 7;
            if (blkCnt > 0U)
            {
                mve_pred16_t p0 = vctp16q(blkCnt);
                q15x8_t   vecB, vecA;

                vecB = vldrhq_gather_shifted_offset((int16_t const *)pInB, vecOffs);
                vecOffs = vecOffs + (uint16_t) (numColsB * 8);

                vecA = vld1q(pSrcA0Vec);
                acc0 = vmlaldavaq_p(acc0, vecA, vecB, p0);
                vecA = vld1q(pSrcA1Vec);
                acc1 = vmlaldavaq_p(acc1, vecA, vecB, p0);
                vecA = vld1q(pSrcA2Vec);
                acc2 = vmlaldavaq_p(acc2, vecA, vecB, p0);
                vecA = vld1q(pSrcA3Vec);
                acc3 = vmlaldavaq_p(acc3, vecA, vecB, p0);
            }

            px[0]            = (q15_t)MVE_ASRL_SAT16(acc0, 15);
            px[1 * numColsB] = (q15_t)MVE_ASRL_SAT16(acc1, 15);
            px[2 * numColsB] = (q15_t)MVE_ASRL_SAT16(acc2, 15);
            px[3 * numColsB] = (q15_t)MVE_ASRL_SAT16(acc3, 15);
            px++;
            /*
             * Decrement the column loop counter
             */
            col--;
            /*
             * Update the pointer pInB to point to the  starting address of the next column
             */
            pInB = pSrcB->pData + (numColsB - col);
        }

        /*
         * Update the pointer pInA to point to the  starting address of the next row
         */
        pInA += (numColsA * 4);
        /*
         * Decrement the row loop counter
         */
        rowCnt --;

    }

    rowCnt = row & 3;
    while (rowCnt > 0U)
    {
      /*
         * Output pointer is set to starting address of the row being processed
         */
        px = pOut + i;
        i = i + numColsB;
        /*
         * For every row wise process, the column loop counter is to be initiated
         */
        col = numColsB;
        /*
         * For every row wise process, the pInB pointer is set
         * to the starting address of the pSrcB data
         */
        pInB = pSrcB->pData;
        /*
         * column loop
         */
        while (col > 0U)
        {
            /*
             * generate 4 columns elements
             */
            /*
             * Matrix A columns number of MAC operations are to be performed
             */
            colCnt = numColsA;

            q15_t const *pSrcA0Vec;
            q15_t    *pInA0 = pInA;
            q63_t     acc0;

            acc0 = 0LL;

            pSrcA0Vec = (q15_t const *) pInA0;
           
            vecOffs = vecColBOffs;

            blkCnt = (numColsA) >> 3;
            while (blkCnt > 0U)
            {
                q15x8_t vecB, vecA;

                vecB = vldrhq_gather_shifted_offset((int16_t const *)pInB, vecOffs);
                vecOffs = vecOffs + (uint16_t) (numColsB * 8);

                vecA = vld1q(pSrcA0Vec);  
                pSrcA0Vec += 8;
                acc0 = vmlaldavaq(acc0, vecA, vecB);
                
                blkCnt--;

            }
            /*
             * tail
             */
            blkCnt = numColsA & 7;
            if (blkCnt > 0U)
            {
                mve_pred16_t p0 = vctp16q(blkCnt);
                q15x8_t   vecB, vecA;

                vecB = vldrhq_gather_shifted_offset((int16_t const *)pInB, vecOffs);
                vecOffs = vecOffs + (uint16_t) (numColsB * 8);

                vecA = vld1q(pSrcA0Vec);
                acc0 = vmlaldavaq_p(acc0, vecA, vecB, p0);
                
            }

            px[0]            = (q15_t)MVE_ASRL_SAT16(acc0, 15);
          
            px++;
            /*
             * Decrement the column loop counter
             */
            col--;
            /*
             * Update the pointer pInB to point to the  starting address of the next column
             */
            pInB = pSrcB->pData + (numColsB - col);
        }

        /*
         * Update the pointer pInA to point to the  starting address of the next row
         */
        pInA += (numColsA );
        rowCnt--;
    }
    /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);

}
#else
arm_status arm_mat_mult_q15(
  const arm_matrix_instance_q15 * pSrcA,
  const arm_matrix_instance_q15 * pSrcB,
        arm_matrix_instance_q15 * pDst,
        q15_t                   * pState)
{
        q63_t sum;                                     /* Accumulator */

#if defined (ARM_MATH_DSP)                             /* != CM0 */

        q15_t *pSrcBT = pState;                        /* Input data matrix pointer for transpose */
        q15_t *pInA = pSrcA->pData;                    /* Input data matrix pointer A of Q15 type */
        q15_t *pInB = pSrcB->pData;                    /* Input data matrix pointer B of Q15 type */
        q15_t *px;                                     /* Temporary output data matrix pointer */
        uint16_t numRowsA = pSrcA->numRows;            /* Number of rows of input matrix A */
        uint16_t numColsB = pSrcB->numCols;            /* Number of columns of input matrix B */
        uint16_t numColsA = pSrcA->numCols;            /* Number of columns of input matrix A */
        uint16_t numRowsB = pSrcB->numRows;            /* Number of rows of input matrix A */
        uint32_t col, i = 0U, row = numRowsB, colCnt;  /* Loop counters */
        arm_status status;                             /* Status of matrix multiplication */
        
        q31_t in;                                      /* Temporary variable to hold the input value */
        q31_t inA1, inB1, inA2, inB2;

#ifdef ARM_MATH_MATRIX_CHECK

  /* Check for matrix mismatch condition */
  if ((pSrcA->numCols != pSrcB->numRows) ||
      (pSrcA->numRows != pDst->numRows)  ||
      (pSrcB->numCols != pDst->numCols)    )
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else

#endif /* #ifdef ARM_MATH_MATRIX_CHECK */

  {
    /* Matrix transpose */
    do
    {
      /* The pointer px is set to starting address of column being processed */
      px = pSrcBT + i;

      /* Apply loop unrolling and exchange columns with row elements */
      col = numColsB >> 2U;

      /* First part of the processing with loop unrolling.  Compute 4 outputs at a time.
       ** a second loop below computes the remaining 1 to 3 samples. */
      while (col > 0U)
      {
        /* Read two elements from row */
        in = read_q15x2_ia ((q15_t **) &pInB);

        /* Unpack and store one element in destination */
#ifndef ARM_MATH_BIG_ENDIAN
        *px = (q15_t) in;
#else
        *px = (q15_t) ((in & (q31_t) 0xffff0000) >> 16);
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

        /* Update pointer px to point to next row of transposed matrix */
        px += numRowsB;

        /* Unpack and store second element in destination */
#ifndef ARM_MATH_BIG_ENDIAN
        *px = (q15_t) ((in & (q31_t) 0xffff0000) >> 16);
#else
        *px = (q15_t) in;
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

        /* Update pointer px to point to next row of transposed matrix */
        px += numRowsB;

        /* Read two elements from row */
        in = read_q15x2_ia ((q15_t **) &pInB);

        /* Unpack and store one element in destination */
#ifndef ARM_MATH_BIG_ENDIAN
        *px = (q15_t) in;
#else
        *px = (q15_t) ((in & (q31_t) 0xffff0000) >> 16);
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */
        px += numRowsB;

#ifndef ARM_MATH_BIG_ENDIAN
        *px = (q15_t) ((in & (q31_t) 0xffff0000) >> 16);
#else
        *px = (q15_t) in;
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */
        px += numRowsB;

        /* Decrement column loop counter */
        col--;
      }

      /* If the columns of pSrcB is not a multiple of 4, compute any remaining output samples here.
       ** No loop unrolling is used. */
      col = numColsB % 0x4U;

      while (col > 0U)
      {
        /* Read and store input element in destination */
        *px = *pInB++;

        /* Update pointer px to point to next row of transposed matrix */
        px += numRowsB;

        /* Decrement column loop counter */
        col--;
      }

      i++;

      /* Decrement row loop counter */
      row--;

    } while (row > 0U);

    /* Reset variables for usage in following multiplication process */
    row = numRowsA;
    i = 0U;
    px = pDst->pData;

    /* The following loop performs the dot-product of each row in pSrcA with each column in pSrcB */
    /* row loop */
    do
    {
      /* For every row wise process, column loop counter is to be initiated */
      col = numColsB;

      /* For every row wise process, pIn2 pointer is set to starting address of transposed pSrcB data */
      pInB = pSrcBT;

      /* column loop */
      do
      {
        /* Set variable sum, that acts as accumulator, to zero */
        sum = 0;

        /* Initiate pointer pInA to point to starting address of column being processed */
        pInA = pSrcA->pData + i;

        /* Apply loop unrolling and compute 2 MACs simultaneously. */
        colCnt = numColsA >> 2U;

        /* matrix multiplication */
        while (colCnt > 0U)
        {
          /* c(m,n) = a(1,1) * b(1,1) + a(1,2) * b(2,1) + .... + a(m,p) * b(p,n) */

          /* read real and imag values from pSrcA and pSrcB buffer */
          inA1 = read_q15x2_ia ((q15_t **) &pInA);
          inB1 = read_q15x2_ia ((q15_t **) &pInB);

          inA2 = read_q15x2_ia ((q15_t **) &pInA);
          inB2 = read_q15x2_ia ((q15_t **) &pInB);

          /* Multiply and Accumlates */
          sum = __SMLALD(inA1, inB1, sum);
          sum = __SMLALD(inA2, inB2, sum);

          /* Decrement loop counter */
          colCnt--;
        }

        /* process remaining column samples */
        colCnt = numColsA % 0x4U;

        while (colCnt > 0U)
        {
          /* c(m,n) = a(1,1) * b(1,1) + a(1,2) * b(2,1) + .... + a(m,p) * b(p,n) */
          sum += *pInA++ * *pInB++;

          /* Decrement loop counter */
          colCnt--;
        }

        /* Saturate and store result in destination buffer */
        *px = (q15_t) (__SSAT((sum >> 15), 16));
        px++;

        /* Decrement column loop counter */
        col--;

      } while (col > 0U);

      i = i + numColsA;

      /* Decrement row loop counter */
      row--;

    } while (row > 0U);

#else /* #if defined (ARM_MATH_DSP) */

        q15_t *pIn1 = pSrcA->pData;                    /* Input data matrix pointer A */
        q15_t *pIn2 = pSrcB->pData;                    /* Input data matrix pointer B */
        q15_t *pInA = pSrcA->pData;                    /* Input data matrix pointer A of Q15 type */
        q15_t *pInB = pSrcB->pData;                    /* Input data matrix pointer B of Q15 type */
        q15_t *pOut = pDst->pData;                     /* Output data matrix pointer */
        q15_t *px;                                     /* Temporary output data matrix pointer */
        uint16_t numColsB = pSrcB->numCols;            /* Number of columns of input matrix B */
        uint16_t numColsA = pSrcA->numCols;            /* Number of columns of input matrix A */
        uint16_t numRowsA = pSrcA->numRows;            /* Number of rows of input matrix A    */
        uint32_t col, i = 0U, row = numRowsA, colCnt;  /* Loop counters */
        arm_status status;                             /* Status of matrix multiplication */
        (void)pState;

#ifdef ARM_MATH_MATRIX_CHECK

  /* Check for matrix mismatch condition */
  if ((pSrcA->numCols != pSrcB->numRows) ||
      (pSrcA->numRows != pDst->numRows)  ||
      (pSrcB->numCols != pDst->numCols)    )
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else

#endif /* #ifdef ARM_MATH_MATRIX_CHECK */

  {
    /* The following loop performs the dot-product of each row in pSrcA with each column in pSrcB */
    /* row loop */
    do
    {
      /* Output pointer is set to starting address of the row being processed */
      px = pOut + i;

      /* For every row wise process, column loop counter is to be initiated */
      col = numColsB;

      /* For every row wise process, pIn2 pointer is set to starting address of pSrcB data */
      pIn2 = pSrcB->pData;

      /* column loop */
      do
      {
        /* Set the variable sum, that acts as accumulator, to zero */
        sum = 0;

        /* Initiate pointer pIn1 to point to starting address of pSrcA */
        pIn1 = pInA;

        /* Matrix A columns number of MAC operations are to be performed */
        colCnt = numColsA;

        /* matrix multiplication */
        while (colCnt > 0U)
        {
          /* c(m,n) = a(1,1) * b(1,1) + a(1,2) * b(2,1) + .... + a(m,p) * b(p,n) */

          /* Perform multiply-accumulates */
          sum += (q31_t) * pIn1++ * *pIn2;
          pIn2 += numColsB;

          /* Decrement loop counter */
          colCnt--;
        }

        /* Convert result from 34.30 to 1.15 format and store saturated value in destination buffer */

        /* Saturate and store result in destination buffer */
        *px++ = (q15_t) __SSAT((sum >> 15), 16);

        /* Decrement column loop counter */
        col--;

        /* Update pointer pIn2 to point to starting address of next column */
        pIn2 = pInB + (numColsB - col);

      } while (col > 0U);

      /* Update pointer pSrcA to point to starting address of next row */
      i = i + numColsB;
      pInA = pInA + numColsA;

      /* Decrement row loop counter */
      row--;

    } while (row > 0U);

#endif /* #if defined (ARM_MATH_DSP) */

    /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of MatrixMult group
 */
