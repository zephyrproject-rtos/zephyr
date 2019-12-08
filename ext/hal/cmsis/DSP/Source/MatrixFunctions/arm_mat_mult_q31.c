/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mat_mult_q31.c
 * Description:  Q31 matrix multiplication
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
  @brief         Q31 matrix multiplication.
  @param[in]     pSrcA      points to the first input matrix structure
  @param[in]     pSrcB      points to the second input matrix structure
  @param[out]    pDst       points to output matrix structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS       : Operation successful
                   - \ref ARM_MATH_SIZE_MISMATCH : Matrix size check failed

  @par           Scaling and Overflow Behavior
                   The function is implemented using an internal 64-bit accumulator.
                   The accumulator has a 2.62 format and maintains full precision of the intermediate
                   multiplication results but provides only a single guard bit. There is no saturation
                   on intermediate additions. Thus, if the accumulator overflows it wraps around and
                   distorts the result. The input signals should be scaled down to avoid intermediate
                   overflows. The input is thus scaled down by log2(numColsA) bits
                   to avoid overflows, as a total of numColsA additions are performed internally.
                   The 2.62 accumulator is right shifted by 31 bits and saturated to 1.31 format to yield the final result.
  @remark
                   Refer to \ref arm_mat_mult_fast_q31() for a faster but less precise implementation of this function.
 */
#if defined(ARM_MATH_MVEI)

#define MATRIX_DIM2 2
#define MATRIX_DIM3 3
#define MATRIX_DIM4 4

__STATIC_INLINE arm_status arm_mat_mult_q31_2x2_mve(
    const arm_matrix_instance_q31 * pSrcA,
    const arm_matrix_instance_q31 * pSrcB,
    arm_matrix_instance_q31 * pDst)
{
    q31_t       *pInB = pSrcB->pData;  /* input data matrix pointer B */
    q31_t       *pInA = pSrcA->pData;  /* input data matrix pointer A */
    q31_t       *pOut = pDst->pData;   /* output data matrix pointer */
    uint32x4_t   vecColBOffs;
    q31_t       *pInA0 = pInA;
    q31_t       *pInA1 = pInA0 + MATRIX_DIM2;
    q63_t        acc0, acc1;
    q31x4_t      vecB, vecA0, vecA1;
    /* enable predication to disable half of vector elements */
    mve_pred16_t p0 = vctp32q(MATRIX_DIM2);

    vecColBOffs = vidupq_u32(0, 1);
    vecColBOffs = vecColBOffs * MATRIX_DIM2;

    pInB = pSrcB->pData;

    /* load 1st B column (partial load) */
    vecB = vldrwq_gather_shifted_offset_z_s32(pInB, vecColBOffs, p0);

    /* load A rows */
    vecA0 = vldrwq_s32(pInA0);
    vecA1 = vldrwq_s32(pInA1);

    acc0 = vrmlaldavhq(vecA0, vecB);
    acc1 = vrmlaldavhq(vecA1, vecB);

    acc0 = asrl(acc0, 23);
    acc1 = asrl(acc1, 23);

    pOut[0 * MATRIX_DIM2] = (q31_t) acc0;
    pOut[1 * MATRIX_DIM2] = (q31_t) acc1;
    pOut++;

    /* move to next B column */
    pInB = pInB + 1;

    vecB = vldrwq_gather_shifted_offset_z_s32(pInB, vecColBOffs, p0);

    acc0 = vrmlaldavhq(vecA0, vecB);
    acc1 = vrmlaldavhq(vecA1, vecB);

    acc0 = asrl(acc0, 23);
    acc1 = asrl(acc1, 23);

    pOut[0 * MATRIX_DIM2] = (q31_t) acc0;
    pOut[1 * MATRIX_DIM2] = (q31_t) acc1;
    /*
     * Return to application
     */
    return (ARM_MATH_SUCCESS);
}



__STATIC_INLINE arm_status arm_mat_mult_q31_3x3_mve(
    const arm_matrix_instance_q31 * pSrcA,
    const arm_matrix_instance_q31 * pSrcB,
    arm_matrix_instance_q31 * pDst)
{
    q31_t       *pInB = pSrcB->pData;  /* input data matrix pointer B */
    q31_t       *pInA = pSrcA->pData;  /* input data matrix pointer A */
    q31_t       *pOut = pDst->pData;   /* output data matrix pointer */
    uint32x4_t   vecColBOffs;
    q31_t       *pInA0 = pInA;
    q31_t       *pInA1 = pInA0 + MATRIX_DIM3;
    q31_t       *pInA2 = pInA1 + MATRIX_DIM3;
    q63_t        acc0, acc1, acc2;
    q31x4_t      vecB, vecA;
    /* enable predication to disable last (4th) vector element */
    mve_pred16_t p0 = vctp32q(MATRIX_DIM3);

    vecColBOffs = vidupq_u32(0, 1);
    vecColBOffs = vecColBOffs * MATRIX_DIM3;

    pInB = pSrcB->pData;

    vecB = vldrwq_gather_shifted_offset_z_s32(pInB, vecColBOffs, p0);

    vecA = vldrwq_s32(pInA0);
    acc0 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA1);
    acc1 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA2);
    acc2 = vrmlaldavhq(vecA, vecB);

    acc0 = asrl(acc0, 23);
    acc1 = asrl(acc1, 23);
    acc2 = asrl(acc2, 23);

    pOut[0 * MATRIX_DIM3] = (q31_t) acc0;
    pOut[1 * MATRIX_DIM3] = (q31_t) acc1;
    pOut[2 * MATRIX_DIM3] = (q31_t) acc2;
    pOut++;

    /* move to next B column */
    pInB = pInB + 1;

    vecB = vldrwq_gather_shifted_offset_z_s32(pInB, vecColBOffs, p0);

    vecA = vldrwq_s32(pInA0);
    acc0 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA1);
    acc1 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA2);
    acc2 = vrmlaldavhq(vecA, vecB);

    acc0 = asrl(acc0, 23);
    acc1 = asrl(acc1, 23);
    acc2 = asrl(acc2, 23);

    pOut[0 * MATRIX_DIM3] = (q31_t) acc0;
    pOut[1 * MATRIX_DIM3] = (q31_t) acc1;
    pOut[2 * MATRIX_DIM3] = (q31_t) acc2;
    pOut++;

    /* move to next B column */
    pInB = pInB + 1;

    vecB = vldrwq_gather_shifted_offset_z_s32(pInB, vecColBOffs, p0);

    vecA = vldrwq_s32(pInA0);
    acc0 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA1);
    acc1 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA2);
    acc2 = vrmlaldavhq(vecA, vecB);

    acc0 = asrl(acc0, 23);
    acc1 = asrl(acc1, 23);
    acc2 = asrl(acc2, 23);

    pOut[0 * MATRIX_DIM3] = (q31_t) acc0;
    pOut[1 * MATRIX_DIM3] = (q31_t) acc1;
    pOut[2 * MATRIX_DIM3] = (q31_t) acc2;
    /*
     * Return to application
     */
    return (ARM_MATH_SUCCESS);
}

__STATIC_INLINE arm_status arm_mat_mult_q31_4x4_mve(
    const arm_matrix_instance_q31 * pSrcA,
    const arm_matrix_instance_q31 * pSrcB,
    arm_matrix_instance_q31 * pDst)
{
    q31_t       *pInB = pSrcB->pData;  /* input data matrix pointer B */
    q31_t       *pInA = pSrcA->pData;  /* input data matrix pointer A */
    q31_t       *pOut = pDst->pData;   /* output data matrix pointer */
    uint32x4_t   vecColBOffs;
    q31_t       *pInA0 = pInA;
    q31_t       *pInA1 = pInA0 + MATRIX_DIM4;
    q31_t       *pInA2 = pInA1 + MATRIX_DIM4;
    q31_t       *pInA3 = pInA2 + MATRIX_DIM4;
    q63_t        acc0, acc1, acc2, acc3;
    q31x4_t      vecB, vecA;

    vecColBOffs = vidupq_u32(0, 4);

    pInB = pSrcB->pData;

    vecB = vldrwq_gather_shifted_offset_s32(pInB, vecColBOffs);

    vecA = vldrwq_s32(pInA0);
    acc0 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA1);
    acc1 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA2);
    acc2 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA3);
    acc3 = vrmlaldavhq(vecA, vecB);

    acc0 = asrl(acc0, 23);
    acc1 = asrl(acc1, 23);
    acc2 = asrl(acc2, 23);
    acc3 = asrl(acc3, 23);

    pOut[0 * MATRIX_DIM4] = (q31_t) acc0;
    pOut[1 * MATRIX_DIM4] = (q31_t) acc1;
    pOut[2 * MATRIX_DIM4] = (q31_t) acc2;
    pOut[3 * MATRIX_DIM4] = (q31_t) acc3;
    pOut++;

    /* move to next B column */
    pInB = pInB + 1;

    vecB = vldrwq_gather_shifted_offset_s32(pInB, vecColBOffs);

    vecA = vldrwq_s32(pInA0);
    acc0 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA1);
    acc1 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA2);
    acc2 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA3);
    acc3 = vrmlaldavhq(vecA, vecB);

    acc0 = asrl(acc0, 23);
    acc1 = asrl(acc1, 23);
    acc2 = asrl(acc2, 23);
    acc3 = asrl(acc3, 23);

    pOut[0 * MATRIX_DIM4] = (q31_t) acc0;
    pOut[1 * MATRIX_DIM4] = (q31_t) acc1;
    pOut[2 * MATRIX_DIM4] = (q31_t) acc2;
    pOut[3 * MATRIX_DIM4] = (q31_t) acc3;

    pOut++;

    /* move to next B column */
    pInB = pInB + 1;

    vecB = vldrwq_gather_shifted_offset_s32(pInB, vecColBOffs);

    vecA = vldrwq_s32(pInA0);
    acc0 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA1);
    acc1 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA2);
    acc2 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA3);
    acc3 = vrmlaldavhq(vecA, vecB);

    acc0 = asrl(acc0, 23);
    acc1 = asrl(acc1, 23);
    acc2 = asrl(acc2, 23);
    acc3 = asrl(acc3, 23);

    pOut[0 * MATRIX_DIM4] = (q31_t) acc0;
    pOut[1 * MATRIX_DIM4] = (q31_t) acc1;
    pOut[2 * MATRIX_DIM4] = (q31_t) acc2;
    pOut[3 * MATRIX_DIM4] = (q31_t) acc3;

    pOut++;

    /* move to next B column */
    pInB = pInB + 1;

    vecB = vldrwq_gather_shifted_offset_s32(pInB, vecColBOffs);

    vecA = vldrwq_s32(pInA0);
    acc0 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA1);
    acc1 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA2);
    acc2 = vrmlaldavhq(vecA, vecB);
    vecA = vldrwq_s32(pInA3);
    acc3 = vrmlaldavhq(vecA, vecB);

    acc0 = asrl(acc0, 23);
    acc1 = asrl(acc1, 23);
    acc2 = asrl(acc2, 23);
    acc3 = asrl(acc3, 23);

    pOut[0 * MATRIX_DIM4] = (q31_t) acc0;
    pOut[1 * MATRIX_DIM4] = (q31_t) acc1;
    pOut[2 * MATRIX_DIM4] = (q31_t) acc2;
    pOut[3 * MATRIX_DIM4] = (q31_t) acc3;
    /*
     * Return to application
     */
    return (ARM_MATH_SUCCESS);
}

arm_status arm_mat_mult_q31(
  const arm_matrix_instance_q31 * pSrcA,
  const arm_matrix_instance_q31 * pSrcB,
        arm_matrix_instance_q31 * pDst)
{
    q31_t const *pInB = (q31_t const *)pSrcB->pData;  /* input data matrix pointer B */
    q31_t const *pInA = (q31_t const *)pSrcA->pData;  /* input data matrix pointer A */
    q31_t      *pOut = pDst->pData;   /* output data matrix pointer */
    q31_t      *px;               /* Temporary output data matrix pointer */
    uint16_t    numRowsA = pSrcA->numRows;    /* number of rows of input matrix A    */
    uint16_t    numColsB = pSrcB->numCols;    /* number of columns of input matrix B */
    uint16_t    numColsA = pSrcA->numCols;    /* number of columns of input matrix A */
    uint16_t    col, i = 0U, row = numRowsA, colCnt;  /* loop counters */
    arm_status  status;          /* status of matrix multiplication */
    uint32x4_t  vecOffs, vecColBOffs;
    uint32_t    blkCnt, rowCnt;           /* loop counters */

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
     /* small squared matrix specialized routines */
    if(numRowsA == numColsB && numColsB == numColsA) {
        if (numRowsA == 1)
        {
          q63_t sum =  (q63_t) *pInA * *pInB;
          pOut[0] = (q31_t)(sum >> 31);
          return (ARM_MATH_SUCCESS);
        }
        else if(numRowsA == 2)
            return arm_mat_mult_q31_2x2_mve(pSrcA, pSrcB, pDst);
        else if(numRowsA == 3)
            return arm_mat_mult_q31_3x3_mve(pSrcA, pSrcB, pDst);
        else if (numRowsA == 4)
            return arm_mat_mult_q31_4x4_mve(pSrcA, pSrcB, pDst);
    }

    vecColBOffs = vidupq_u32(0, 1);
    vecColBOffs = vecColBOffs * (uint32_t) (numColsB);

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
        pInB = (q31_t const *)pSrcB->pData;
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

            q31_t const *pSrcA0Vec, *pSrcA1Vec, *pSrcA2Vec, *pSrcA3Vec;
            q31_t const   *pInA0 = pInA;
            q31_t const   *pInA1 = pInA0 + numColsA;
            q31_t const   *pInA2 = pInA1 + numColsA;
            q31_t const   *pInA3 = pInA2 + numColsA;
            q63_t          acc0, acc1, acc2, acc3;

            acc0 = 0LL;
            acc1 = 0LL;
            acc2 = 0LL;
            acc3 = 0LL;

            pSrcA0Vec = (q31_t const *) pInA0;
            pSrcA1Vec = (q31_t const *) pInA1;
            pSrcA2Vec = (q31_t const *) pInA2;
            pSrcA3Vec = (q31_t const *) pInA3;

            vecOffs = vecColBOffs;

            /* process 1 x 4 block output */
            blkCnt = numColsA >> 2;
            while (blkCnt > 0U)
            {
                q31x4_t vecB, vecA;

                vecB = vldrwq_gather_shifted_offset(pInB, vecOffs);
                /* move Matrix B read offsets, 4 rows down */
                vecOffs = vecOffs + (uint32_t) (numColsB * 4);

                vecA = vld1q(pSrcA0Vec);  pSrcA0Vec += 4;
                acc0 = vrmlaldavhaq(acc0, vecA, vecB);
                vecA = vld1q(pSrcA1Vec);  pSrcA1Vec += 4;
                acc1 = vrmlaldavhaq(acc1, vecA, vecB);
                vecA = vld1q(pSrcA2Vec);  pSrcA2Vec += 4;
                acc2 = vrmlaldavhaq(acc2, vecA, vecB);
                vecA = vld1q(pSrcA3Vec);  pSrcA3Vec += 4;
                acc3 = vrmlaldavhaq(acc3, vecA, vecB);
                blkCnt--;
            }

            /*
             * tail
             * (will be merged thru tail predication)
             */
            blkCnt = numColsA & 3;
            if (blkCnt > 0U)
            {
                mve_pred16_t p0 = vctp32q(blkCnt);
                q31x4_t   vecB, vecA;

                vecB = vldrwq_gather_shifted_offset_z(pInB, vecOffs, p0);
                //vecOffs = vecOffs + (uint32_t) (numColsB * 4);

                vecA = vld1q(pSrcA0Vec);  pSrcA0Vec += 4;
                acc0 = vrmlaldavhaq(acc0, vecA, vecB);
                vecA = vld1q(pSrcA1Vec);  pSrcA1Vec += 4;
                acc1 = vrmlaldavhaq(acc1, vecA, vecB);
                vecA = vld1q(pSrcA2Vec);  pSrcA2Vec += 4;
                acc2 = vrmlaldavhaq(acc2, vecA, vecB);
                vecA = vld1q(pSrcA3Vec);  pSrcA3Vec += 4;
                acc3 = vrmlaldavhaq(acc3, vecA, vecB);
            }

            acc0 = asrl(acc0, 23);
            acc1 = asrl(acc1, 23);
            acc2 = asrl(acc2, 23);
            acc3 = asrl(acc3, 23);

            px[0] = (q31_t) acc0;
            px[1 * numColsB] = (q31_t) acc1;
            px[2 * numColsB] = (q31_t) acc2;
            px[3 * numColsB] = (q31_t) acc3;
            px++;
            /*
             * Decrement the column loop counter
             */
            col--;
            /*
             * Update the pointer pInB to point to the  starting address of the next column
             */
            pInB = (q31_t const *)pSrcB->pData + (numColsB - col);
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
        pInB = (q31_t const *)pSrcB->pData;
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

            q31_t const *pSrcA0Vec;
            q31_t const   *pInA0 = pInA;
            q63_t          acc0;

            acc0 = 0LL;
           

            pSrcA0Vec = (q31_t const *) pInA0;
           
            vecOffs = vecColBOffs;

            /* process 1 x 4 block output */
            blkCnt = numColsA >> 2;
            while (blkCnt > 0U)
            {
                q31x4_t vecB, vecA;

                vecB = vldrwq_gather_shifted_offset(pInB, vecOffs);
                /* move Matrix B read offsets, 4 rows down */
                vecOffs = vecOffs + (uint32_t) (numColsB * 4);

                vecA = vld1q(pSrcA0Vec);  pSrcA0Vec += 4;
                acc0 = vrmlaldavhaq(acc0, vecA, vecB);
              
                blkCnt--;
            }

            /*
             * tail
             * (will be merged thru tail predication)
             */
            blkCnt = numColsA & 3;
            if (blkCnt > 0U)
            {
                mve_pred16_t p0 = vctp32q(blkCnt);
                q31x4_t   vecB, vecA;

                vecB = vldrwq_gather_shifted_offset_z(pInB, vecOffs, p0);
                //vecOffs = vecOffs + (uint32_t) (numColsB * 4);

                vecA = vld1q(pSrcA0Vec);  
                pSrcA0Vec += 4;
                acc0 = vrmlaldavhaq(acc0, vecA, vecB);
                
            }

            acc0 = asrl(acc0, 23);
           

            px[0] = (q31_t) acc0;
            px++;
            /*
             * Decrement the column loop counter
             */
            col--;
            /*
             * Update the pointer pInB to point to the  starting address of the next column
             */
            pInB = (q31_t const *)pSrcB->pData + (numColsB - col);
        }

        /*
         * Update the pointer pInA to point to the  starting address of the next row
         */
        pInA += numColsA;
        /*
         * Decrement the row loop counter
         */
        rowCnt--;
    }

    /*
     * set status as ARM_MATH_SUCCESS
     */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}

#else
arm_status arm_mat_mult_q31(
  const arm_matrix_instance_q31 * pSrcA,
  const arm_matrix_instance_q31 * pSrcB,
        arm_matrix_instance_q31 * pDst)
{
  q31_t *pIn1 = pSrcA->pData;                    /* Input data matrix pointer A */
  q31_t *pIn2 = pSrcB->pData;                    /* Input data matrix pointer B */
  q31_t *pInA = pSrcA->pData;                    /* Input data matrix pointer A */
  q31_t *pInB = pSrcB->pData;                    /* Input data matrix pointer B */
  q31_t *pOut = pDst->pData;                     /* Output data matrix pointer */
  q31_t *px;                                     /* Temporary output data matrix pointer */
  q63_t sum;                                     /* Accumulator */
  uint16_t numRowsA = pSrcA->numRows;            /* Number of rows of input matrix A */
  uint16_t numColsB = pSrcB->numCols;            /* Number of columns of input matrix B */
  uint16_t numColsA = pSrcA->numCols;            /* Number of columns of input matrix A */
  uint32_t col, i = 0U, row = numRowsA, colCnt;  /* Loop counters */
  arm_status status;                             /* Status of matrix multiplication */

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
      /* Output pointer is set to starting address of row being processed */
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

        /* Initialize pointer pIn1 to point to starting address of column being processed */
        pIn1 = pInA;

#if defined (ARM_MATH_LOOPUNROLL)

        /* Loop unrolling: Compute 4 MACs at a time. */
        colCnt = numColsA >> 2U;

        /* matrix multiplication */
        while (colCnt > 0U)
        {
          /* c(m,n) = a(1,1) * b(1,1) + a(1,2) * b(2,1) + .... + a(m,p) * b(p,n) */

          /* Perform the multiply-accumulates */
          sum += (q63_t) *pIn1++ * *pIn2;
          pIn2 += numColsB;

          sum += (q63_t) *pIn1++ * *pIn2;
          pIn2 += numColsB;

          sum += (q63_t) *pIn1++ * *pIn2;
          pIn2 += numColsB;

          sum += (q63_t) *pIn1++ * *pIn2;
          pIn2 += numColsB;

          /* Decrement loop counter */
          colCnt--;
        }

        /* Loop unrolling: Compute remaining MACs */
        colCnt = numColsA % 0x4U;

#else

        /* Initialize cntCnt with number of columns */
        colCnt = numColsA;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

        while (colCnt > 0U)
        {
          /* c(m,n) = a(1,1) * b(1,1) + a(1,2) * b(2,1) + .... + a(m,p) * b(p,n) */

          /* Perform the multiply-accumulates */
          sum += (q63_t) *pIn1++ * *pIn2;
          pIn2 += numColsB;

          /* Decrement loop counter */
          colCnt--;
        }

        /* Convert result from 2.62 to 1.31 format and store in destination buffer */
        *px++ = (q31_t) (sum >> 31);

        /* Decrement column loop counter */
        col--;

        /* Update pointer pIn2 to point to starting address of next column */
        pIn2 = pInB + (numColsB - col);

      } while (col > 0U);

      /* Update pointer pInA to point to starting address of next row */
      i = i + numColsB;
      pInA = pInA + numColsA;

      /* Decrement row loop counter */
      row--;

    } while (row > 0U);

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
