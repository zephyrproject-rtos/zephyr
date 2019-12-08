/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cmplx_mat_mult_q15.c
 * Description:  Q15 complex matrix multiplication
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
  @addtogroup CmplxMatrixMult
  @{
 */

/**
  @brief         Q15 Complex matrix multiplication.
  @param[in]     pSrcA      points to first input complex matrix structure
  @param[in]     pSrcB      points to second input complex matrix structure
  @param[out]    pDst       points to output complex matrix structure
  @param[in]     pScratch   points to an array for storing intermediate results
  @return        execution status
                   - \ref ARM_MATH_SUCCESS       : Operation successful
                   - \ref ARM_MATH_SIZE_MISMATCH : Matrix size check failed

  @par           Conditions for optimum performance
                   Input, output and state buffers should be aligned by 32-bit

  @par           Scaling and Overflow Behavior
                   The function is implemented using an internal 64-bit accumulator. The inputs to the
                   multiplications are in 1.15 format and multiplications yield a 2.30 result.
                   The 2.30 intermediate results are accumulated in a 64-bit accumulator in 34.30 format.
                   This approach provides 33 guard bits and there is no risk of overflow. The 34.30 result is then
                   truncated to 34.15 format by discarding the low 15 bits and then saturated to 1.15 format.
 */
#if defined(ARM_MATH_MVEI)

#define MVE_ASRL_SAT16(acc, shift)          ((sqrshrl_sat48(acc, -(32-shift)) >> 32) & 0xffffffff)

arm_status arm_mat_cmplx_mult_q15(
  const arm_matrix_instance_q15 * pSrcA,
  const arm_matrix_instance_q15 * pSrcB,
        arm_matrix_instance_q15 * pDst,
        q15_t                   * pScratch)
{
    q15_t const *pInA = (q15_t const *) pSrcA->pData;   /* input data matrix pointer A of Q15 type */
    q15_t const *pInB = (q15_t const *) pSrcB->pData;   /* input data matrix pointer B of Q15 type */
    q15_t const *pInB2;
    q15_t       *px;               /* Temporary output data matrix pointer */
    uint32_t     numRowsA = pSrcA->numRows;    /* number of rows of input matrix A    */
    uint32_t     numColsB = pSrcB->numCols;    /* number of columns of input matrix B */
    uint32_t     numColsA = pSrcA->numCols;    /* number of columns of input matrix A */
    uint32_t     numRowsB = pSrcB->numRows;    /* number of rows of input matrix A    */
    uint32_t     col, i = 0u, j, row = numRowsB;   /* loop counters */
    uint32_t  blkCnt;           /* loop counters */
    uint16x8_t vecOffs, vecColBOffs;
    arm_status status;                             /* Status of matrix multiplication */
    (void)pScratch;

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
    vecColBOffs[0] = 0;
    vecColBOffs[1] = 1;
    vecColBOffs[2] = numColsB * CMPLX_DIM;
    vecColBOffs[3] = (numColsB * CMPLX_DIM) + 1;
    vecColBOffs[4] = 2 * numColsB * CMPLX_DIM;
    vecColBOffs[5] = 2 * (numColsB * CMPLX_DIM) + 1;
    vecColBOffs[6] = 3 * numColsB * CMPLX_DIM;
    vecColBOffs[7] = 3 * (numColsB * CMPLX_DIM) + 1;

    /*
     * Reset the variables for the usage in the following multiplication process
     */
    i = 0;
    row = numRowsA;
    px = pDst->pData;

    /*
     * The following loop performs the dot-product of each row in pSrcA with each column in pSrcB
     */

    /*
     * row loop
     */
    while (row > 0u)
    {
        /*
         * For every row wise process, the column loop counter is to be initiated
         */
        col = numColsB >> 1;
        j = 0;
        /*
         * column loop
         */
        while (col > 0u)
        {
            q15_t const *pSrcAVec;
            //, *pSrcBVec, *pSrcB2Vec;
            q15x8_t vecA, vecB, vecB2;
            q63_t     acc0, acc1, acc2, acc3;

            /*
             * Initiate the pointer pIn1 to point to the starting address of the column being processed
             */
            pInA = pSrcA->pData + i;
            pInB = pSrcB->pData + j;
            pInB2 = pInB + CMPLX_DIM;

            j += 2 * CMPLX_DIM;
            /*
             * Decrement the column loop counter
             */
            col--;

            /*
             * Initiate the pointers
             * - current Matrix A rows
             * - 2 x consecutive Matrix B' rows (j increment is 2 x numRowsB)
             */
            pSrcAVec = (q15_t const *) pInA;

            acc0 = 0LL;
            acc1 = 0LL;
            acc2 = 0LL;
            acc3 = 0LL;

            vecOffs = vecColBOffs;
          

            blkCnt = (numColsA * CMPLX_DIM) >> 3;
            while (blkCnt > 0U)
            {
                vecA = vld1q(pSrcAVec); 
                pSrcAVec += 8;
                vecB = vldrhq_gather_shifted_offset(pInB, vecOffs);

                acc0 = vmlsldavaq(acc0, vecA, vecB);
                acc1 = vmlaldavaxq(acc1, vecA, vecB);
                vecB2 = vldrhq_gather_shifted_offset(pInB2, vecOffs);
                /*
                 * move Matrix B read offsets, 4 rows down
                 */
                vecOffs = vaddq(vecOffs, (uint16_t) (numColsB * 4 * CMPLX_DIM));

                acc2 = vmlsldavaq(acc2, vecA, vecB2);
                acc3 = vmlaldavaxq(acc3, vecA, vecB2);

                blkCnt--;
            }

            /*
             * tail
             */
            blkCnt = (numColsA * CMPLX_DIM) & 7;
            if (blkCnt > 0U)
            {
                mve_pred16_t p0 = vctp16q(blkCnt);
                vecB = vldrhq_gather_shifted_offset(pInB, vecOffs);

                vecA = vldrhq_z_s16(pSrcAVec, p0);

                acc0 = vmlsldavaq(acc0, vecA, vecB);
                acc1 = vmlaldavaxq(acc1, vecA, vecB);
                vecB2 = vldrhq_gather_shifted_offset(pInB2, vecOffs);

                /*
                 * move Matrix B read offsets, 4 rows down
                 */
                vecOffs = vaddq(vecOffs, (uint16_t) (numColsB * 4 * CMPLX_DIM));

                acc2 = vmlsldavaq(acc2, vecA, vecB2);
                acc3 = vmlaldavaxq(acc3, vecA, vecB2);

            }
            /*
             * Convert to 1.15, Store the results (1 x 2 block) in the destination buffer
             */
            *px++ = (q15_t)MVE_ASRL_SAT16(acc0, 15);
            *px++ = (q15_t)MVE_ASRL_SAT16(acc1, 15);
            *px++ = (q15_t)MVE_ASRL_SAT16(acc2, 15);
            *px++ = (q15_t)MVE_ASRL_SAT16(acc3, 15);
        }

        col = numColsB & 1;
        /*
         * column loop
         */
        while (col > 0u)
        {

            q15_t const *pSrcAVec;
            //, *pSrcBVec, *pSrcB2Vec;
            q15x8_t vecA, vecB;
            q63_t     acc0, acc1;

            /*
             * Initiate the pointer pIn1 to point to the starting address of the column being processed
             */
            pInA = pSrcA->pData + i;
            pInB = pSrcB->pData + j;

            j += CMPLX_DIM;
            /*
             * Decrement the column loop counter
             */
            col--;

            /*
             * Initiate the pointers
             * - current Matrix A rows
             * - 2 x consecutive Matrix B' rows (j increment is 2 x numRowsB)
             */
            pSrcAVec = (q15_t const *) pInA;

            acc0 = 0LL;
            acc1 = 0LL;
           

            vecOffs = vecColBOffs;
           
            

            blkCnt = (numColsA * CMPLX_DIM) >> 3;
            while (blkCnt > 0U)
            {
                vecA = vld1q(pSrcAVec); 
                pSrcAVec += 8;
                vecB = vldrhq_gather_shifted_offset(pInB, vecOffs);

                acc0 = vmlsldavaq(acc0, vecA, vecB);
                acc1 = vmlaldavaxq(acc1, vecA, vecB);
                /*
                 * move Matrix B read offsets, 4 rows down
                 */
                vecOffs = vaddq(vecOffs, (uint16_t) (numColsB * 4 * CMPLX_DIM));

                blkCnt--;
            }

            /*
             * tail
             */
            blkCnt = (numColsA * CMPLX_DIM) & 7;
            if (blkCnt > 0U)
            {
                mve_pred16_t p0 = vctp16q(blkCnt);
                vecB = vldrhq_gather_shifted_offset(pInB, vecOffs);
                vecA = vldrhq_z_s16(pSrcAVec, p0);

                acc0 = vmlsldavaq(acc0, vecA, vecB);
                acc1 = vmlaldavaxq(acc1, vecA, vecB);
               
            }
            /*
             * Convert to 1.15, Store the results (1 x 2 block) in the destination buffer
             */
            *px++ = (q15_t)MVE_ASRL_SAT16(acc0, 15);
            *px++ = (q15_t)MVE_ASRL_SAT16(acc1, 15);
           
        }

        i = i + numColsA * CMPLX_DIM;
        
        /*
         * Decrement the row loop counter
         */
        row--;
    }


    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}
#else
arm_status arm_mat_cmplx_mult_q15(
  const arm_matrix_instance_q15 * pSrcA,
  const arm_matrix_instance_q15 * pSrcB,
        arm_matrix_instance_q15 * pDst,
        q15_t                   * pScratch)
{
        q15_t *pSrcBT = pScratch;                      /* input data matrix pointer for transpose */
        q15_t *pInA = pSrcA->pData;                    /* input data matrix pointer A of Q15 type */
        q15_t *pInB = pSrcB->pData;                    /* input data matrix pointer B of Q15 type */
        q15_t *px;                                     /* Temporary output data matrix pointer */
        uint16_t numRowsA = pSrcA->numRows;            /* number of rows of input matrix A */
        uint16_t numColsB = pSrcB->numCols;            /* number of columns of input matrix B */
        uint16_t numColsA = pSrcA->numCols;            /* number of columns of input matrix A */
        uint16_t numRowsB = pSrcB->numRows;            /* number of rows of input matrix A */
        q63_t sumReal, sumImag;                        /* accumulator */
        uint32_t col, i = 0U, row = numRowsB, colCnt;  /* Loop counters */
        arm_status status;                             /* Status of matrix multiplication */

#if defined (ARM_MATH_DSP)
        q31_t prod1, prod2;
        q31_t pSourceA, pSourceB;
#else
        q15_t a, b, c, d;
#endif /* #if defined (ARM_MATH_DSP) */

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

#if defined (ARM_MATH_LOOPUNROLL)

      /* Apply loop unrolling and exchange the columns with row elements */
      col = numColsB >> 2;

      /* First part of the processing with loop unrolling.  Compute 4 outputs at a time.
         a second loop below computes the remaining 1 to 3 samples. */
      while (col > 0U)
      {
        /* Read two elements from row */
        write_q15x2 (px, read_q15x2_ia (&pInB));

        /* Update pointer px to point to next row of transposed matrix */
        px += numRowsB * 2;

        /* Read two elements from row */
        write_q15x2 (px, read_q15x2_ia (&pInB));

        /* Update pointer px to point to next row of transposed matrix */
        px += numRowsB * 2;

        /* Read two elements from row */
        write_q15x2 (px, read_q15x2_ia (&pInB));

        /* Update pointer px to point to next row of transposed matrix */
        px += numRowsB * 2;

        /* Read two elements from row */
        write_q15x2 (px, read_q15x2_ia (&pInB));

        /* Update pointer px to point to next row of transposed matrix */
        px += numRowsB * 2;

        /* Decrement column loop counter */
        col--;
      }

      /* If the columns of pSrcB is not a multiple of 4, compute any remaining output samples here.
       ** No loop unrolling is used. */
      col = numColsB % 0x4U;

#else

        /* Initialize blkCnt with number of samples */
        col = numColsB;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

      while (col > 0U)
      {
        /* Read two elements from row */
        write_q15x2 (px, read_q15x2_ia (&pInB));

        /* Update pointer px to point to next row of transposed matrix */
        px += numRowsB * 2;

        /* Decrement column loop counter */
        col--;
      }

      i = i + 2U;

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
        sumReal = 0;
        sumImag = 0;

        /* Initiate pointer pInA to point to starting address of column being processed */
        pInA = pSrcA->pData + i * 2;

        /* Apply loop unrolling and compute 2 MACs simultaneously. */
        colCnt = numColsA >> 1U;

        /* matrix multiplication */
        while (colCnt > 0U)
        {
          /* c(m,n) = a(1,1) * b(1,1) + a(1,2) * b(2,1) + .... + a(m,p) * b(p,n) */

#if defined (ARM_MATH_DSP)

          /* read real and imag values from pSrcA and pSrcB buffer */
          pSourceA = read_q15x2_ia ((q15_t **) &pInA);
          pSourceB = read_q15x2_ia ((q15_t **) &pInB);

          /* Multiply and Accumlates */
#ifdef ARM_MATH_BIG_ENDIAN
          prod1 = -__SMUSD(pSourceA, pSourceB);
#else
          prod1 = __SMUSD(pSourceA, pSourceB);
#endif
          prod2 = __SMUADX(pSourceA, pSourceB);
          sumReal += (q63_t) prod1;
          sumImag += (q63_t) prod2;

          /* read real and imag values from pSrcA and pSrcB buffer */
          pSourceA = read_q15x2_ia ((q15_t **) &pInA);
          pSourceB = read_q15x2_ia ((q15_t **) &pInB);

          /* Multiply and Accumlates */
#ifdef ARM_MATH_BIG_ENDIAN
          prod1 = -__SMUSD(pSourceA, pSourceB);
#else
          prod1 = __SMUSD(pSourceA, pSourceB);
#endif
          prod2 = __SMUADX(pSourceA, pSourceB);
          sumReal += (q63_t) prod1;
          sumImag += (q63_t) prod2;

#else /* #if defined (ARM_MATH_DSP) */

          /* read real and imag values from pSrcA buffer */
          a = *pInA;
          b = *(pInA + 1U);
          /* read real and imag values from pSrcB buffer */
          c = *pInB;
          d = *(pInB + 1U);

          /* Multiply and Accumlates */
          sumReal += (q31_t) a *c;
          sumImag += (q31_t) a *d;
          sumReal -= (q31_t) b *d;
          sumImag += (q31_t) b *c;

          /* read next real and imag values from pSrcA buffer */
          a = *(pInA + 2U);
          b = *(pInA + 3U);
          /* read next real and imag values from pSrcB buffer */
          c = *(pInB + 2U);
          d = *(pInB + 3U);

          /* update pointer */
          pInA += 4U;

          /* Multiply and Accumlates */
          sumReal += (q31_t) a * c;
          sumImag += (q31_t) a * d;
          sumReal -= (q31_t) b * d;
          sumImag += (q31_t) b * c;
          /* update pointer */
          pInB += 4U;

#endif /* #if defined (ARM_MATH_DSP) */

          /* Decrement loop counter */
          colCnt--;
        }

        /* process odd column samples */
        if ((numColsA & 0x1U) > 0U)
        {
          /* c(m,n) = a(1,1) * b(1,1) + a(1,2) * b(2,1) + .... + a(m,p) * b(p,n) */

#if defined (ARM_MATH_DSP)
          /* read real and imag values from pSrcA and pSrcB buffer */
          pSourceA = read_q15x2_ia ((q15_t **) &pInA);
          pSourceB = read_q15x2_ia ((q15_t **) &pInB);

          /* Multiply and Accumlates */
#ifdef ARM_MATH_BIG_ENDIAN
          prod1 = -__SMUSD(pSourceA, pSourceB);
#else
          prod1 = __SMUSD(pSourceA, pSourceB);
#endif
          prod2 = __SMUADX(pSourceA, pSourceB);
          sumReal += (q63_t) prod1;
          sumImag += (q63_t) prod2;

#else /* #if defined (ARM_MATH_DSP) */

          /* read real and imag values from pSrcA and pSrcB buffer */
          a = *pInA++;
          b = *pInA++;
          c = *pInB++;
          d = *pInB++;

          /* Multiply and Accumlates */
          sumReal += (q31_t) a * c;
          sumImag += (q31_t) a * d;
          sumReal -= (q31_t) b * d;
          sumImag += (q31_t) b * c;

#endif /* #if defined (ARM_MATH_DSP) */

        }

        /* Saturate and store result in destination buffer */
        *px++ = (q15_t) (__SSAT(sumReal >> 15, 16));
        *px++ = (q15_t) (__SSAT(sumImag >> 15, 16));

        /* Decrement column loop counter */
        col--;

      } while (col > 0U);

      i = i + numColsA;

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
