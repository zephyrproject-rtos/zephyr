/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mat_inverse_f32.c
 * Description:  Floating-point matrix inverse
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
  @defgroup MatrixInv Matrix Inverse

  Computes the inverse of a matrix.

  The inverse is defined only if the input matrix is square and non-singular (the determinant is non-zero).
  The function checks that the input and output matrices are square and of the same size.

  Matrix inversion is numerically sensitive and the CMSIS DSP library only supports matrix
  inversion of floating-point matrices.

  @par Algorithm
  The Gauss-Jordan method is used to find the inverse.
  The algorithm performs a sequence of elementary row-operations until it
  reduces the input matrix to an identity matrix. Applying the same sequence
  of elementary row-operations to an identity matrix yields the inverse matrix.
  If the input matrix is singular, then the algorithm terminates and returns error status
  <code>ARM_MATH_SINGULAR</code>.
  \image html MatrixInverse.gif "Matrix Inverse of a 3 x 3 matrix using Gauss-Jordan Method"
 */

/**
  @addtogroup MatrixInv
  @{
 */

/**
  @brief         Floating-point matrix inverse.
  @param[in]     pSrc      points to input matrix structure
  @param[out]    pDst      points to output matrix structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS       : Operation successful
                   - \ref ARM_MATH_SIZE_MISMATCH : Matrix size check failed
                   - \ref ARM_MATH_SINGULAR      : Input matrix is found to be singular (non-invertible)
 */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

arm_status arm_mat_inverse_f32(
  const arm_matrix_instance_f32 * pSrc,
  arm_matrix_instance_f32 * pDst)
{
    float32_t *pIn = pSrc->pData;   /* input data matrix pointer */
    float32_t *pOut = pDst->pData;  /* output data matrix pointer */
    float32_t *pInT1, *pInT2;   /* Temporary input data matrix pointer */
    float32_t *pOutT1, *pOutT2; /* Temporary output data matrix pointer */
    float32_t *pPivotRowIn, *pPRT_in, *pPivotRowDst, *pPRT_pDst;    /* Temporary input and output data matrix pointer */

    uint32_t  numRows = pSrc->numRows;  /* Number of rows in the matrix  */
    uint32_t  numCols = pSrc->numCols;  /* Number of Cols in the matrix  */
    float32_t *pTmpA, *pTmpB;

    float32_t in = 0.0f;        /* Temporary input values  */
    uint32_t  i, rowCnt, flag = 0U, j, loopCnt, k, l;   /* loop counters */
    arm_status status;          /* status of matrix inverse */
    uint32_t  blkCnt;

#ifdef ARM_MATH_MATRIX_CHECK
   /* Check for matrix mismatch condition */
  if ((pSrc->numRows != pSrc->numCols) || (pDst->numRows != pDst->numCols)
     || (pSrc->numRows != pDst->numRows))
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else
#endif /*    #ifdef ARM_MATH_MATRIX_CHECK    */
  {

    /*--------------------------------------------------------------------------------------------------------------
     * Matrix Inverse can be solved using elementary row operations.
     *
     *  Gauss-Jordan Method:
     *
     *     1. First combine the identity matrix and the input matrix separated by a bar to form an
     *        augmented matrix as follows:
     *                      _  _          _     _      _   _         _         _
     *                     |  |  a11  a12  | | | 1   0  |   |       |  X11 X12  |
     *                     |  |            | | |        |   |   =   |           |
     *                     |_ |_ a21  a22 _| | |_0   1 _|  _|       |_ X21 X21 _|
     *
     *      2. In our implementation, pDst Matrix is used as identity matrix.
     *
     *      3. Begin with the first row. Let i = 1.
     *
     *      4. Check to see if the pivot for row i is zero.
     *         The pivot is the element of the main diagonal that is on the current row.
     *         For instance, if working with row i, then the pivot element is aii.
     *         If the pivot is zero, exchange that row with a row below it that does not
     *         contain a zero in column i. If this is not possible, then an inverse
     *         to that matrix does not exist.
     *
     *      5. Divide every element of row i by the pivot.
     *
     *      6. For every row below and  row i, replace that row with the sum of that row and
     *         a multiple of row i so that each new element in column i below row i is zero.
     *
     *      7. Move to the next row and column and repeat steps 2 through 5 until you have zeros
     *         for every element below and above the main diagonal.
     *
     *      8. Now an identical matrix is formed to the left of the bar(input matrix, src).
     *         Therefore, the matrix to the right of the bar is our solution(dst matrix, dst).
     *----------------------------------------------------------------------------------------------------------------*/

        /*
         * Working pointer for destination matrix
         */
        pOutT1 = pOut;
        /*
         * Loop over the number of rows
         */
        rowCnt = numRows;
        /*
         * Making the destination matrix as identity matrix
         */
        while (rowCnt > 0U)
        {
            /*
             * Writing all zeroes in lower triangle of the destination matrix
             */
            j = numRows - rowCnt;
            while (j > 0U)
            {
                *pOutT1++ = 0.0f;
                j--;
            }
            /*
             * Writing all ones in the diagonal of the destination matrix
             */
            *pOutT1++ = 1.0f;
            /*
             * Writing all zeroes in upper triangle of the destination matrix
             */
            j = rowCnt - 1U;
            while (j > 0U)
            {
                *pOutT1++ = 0.0f;
                j--;
            }
            /*
             * Decrement the loop counter
             */
            rowCnt--;
        }

        /*
         * Loop over the number of columns of the input matrix.
         * All the elements in each column are processed by the row operations
         */
        loopCnt = numCols;
        /*
         * Index modifier to navigate through the columns
         */
        l = 0U;
        while (loopCnt > 0U)
        {
            /*
             * Check if the pivot element is zero..
             * If it is zero then interchange the row with non zero row below.
             * If there is no non zero element to replace in the rows below,
             * then the matrix is Singular.
             */

            /*
             * Working pointer for the input matrix that points
             * * to the pivot element of the particular row
             */
            pInT1 = pIn + (l * numCols);
            /*
             * Working pointer for the destination matrix that points
             * * to the pivot element of the particular row
             */
            pOutT1 = pOut + (l * numCols);
            /*
             * Temporary variable to hold the pivot value
             */
            in = *pInT1;
            /*
             * Destination pointer modifier
             */
            k = 1U;

            /*
             * Check if the pivot element is zero
             */
            if (*pInT1 == 0.0f)
            {
                /*
                 * Loop over the number rows present below
                 */
                for (i = (l + 1U); i < numRows; i++)
                {
                    /*
                     * Update the input and destination pointers
                     */
                    pInT2 = pInT1 + (numCols * i);
                    pOutT2 = pOutT1 + (numCols * k);
                    /*
                     * Check if there is a non zero pivot element to
                     * * replace in the rows below
                     */
                    if (*pInT2 != 0.0f)
                    {
                        f32x4_t vecA, vecB;
                        /*
                         * Loop over number of columns
                         * * to the right of the pilot element
                         */
                        pTmpA = pInT1;
                        pTmpB = pInT2;
                        blkCnt = (numCols - l) >> 2;
                        while (blkCnt > 0U)
                        {
                            
                            vecA = vldrwq_f32(pTmpA);
                            vecB = vldrwq_f32(pTmpB);
                            vstrwq_f32(pTmpB, vecA);
                            vstrwq_f32(pTmpA, vecB);

                            pTmpA += 4;
                            pTmpB += 4;
                            /*
                             * Decrement the blockSize loop counter
                             */
                            blkCnt--;
                        }
                        /*
                         * tail
                         * (will be merged thru tail predication)
                         */
                        blkCnt = (numCols - l) & 3;
                        if (blkCnt > 0U)
                        {
                            mve_pred16_t p0 = vctp32q(blkCnt);

                            vecA = vldrwq_f32(pTmpA);
                            vecB = vldrwq_f32(pTmpB);
                            vstrwq_p_f32(pTmpB, vecA, p0);
                            vstrwq_p_f32(pTmpA, vecB, p0);
                        }

                        pInT1 += numCols - l;
                        pInT2 += numCols - l;
                        pTmpA = pOutT1;
                        pTmpB = pOutT2;
                        blkCnt = numCols >> 2;
                        while (blkCnt > 0U)
                        {

                            vecA = vldrwq_f32(pTmpA);
                            vecB = vldrwq_f32(pTmpB);
                            vstrwq_f32(pTmpB, vecA);
                            vstrwq_f32(pTmpA, vecB);
                            pTmpA += 4;
                            pTmpB += 4;
                            /*
                             * Decrement the blockSize loop counter
                             */
                            blkCnt--;
                        }
                        /*
                         * tail
                         */
                        blkCnt = numCols & 3;
                        if (blkCnt > 0U)
                        {
                            mve_pred16_t p0 = vctp32q(blkCnt);

                            vecA = vldrwq_f32(pTmpA);
                            vecB = vldrwq_f32(pTmpB);
                            vstrwq_p_f32(pTmpB, vecA, p0);
                            vstrwq_p_f32(pTmpA, vecB, p0);
                        }

                        pOutT1 += numCols;
                        pOutT2 += numCols;
                        /*
                         * Flag to indicate whether exchange is done or not
                         */
                        flag = 1U;

                        /*
                         * Break after exchange is done
                         */
                        break;
                    }
                    /*
                     * Update the destination pointer modifier
                     */
                    k++;
                }
            }

            /*
             * Update the status if the matrix is singular
             */
            if ((flag != 1U) && (in == 0.0f))
            {
                return ARM_MATH_SINGULAR;
            }

            /*
             * Points to the pivot row of input and destination matrices
             */
            pPivotRowIn = pIn + (l * numCols);
            pPivotRowDst = pOut + (l * numCols);

            /*
             * Temporary pointers to the pivot row pointers
             */
            pInT1 = pPivotRowIn;
            pOutT1 = pPivotRowDst;

            /*
             * Pivot element of the row
             */
            in = *(pIn + (l * numCols));

            pTmpA = pInT1;

            f32x4_t invIn = vdupq_n_f32(1.0f / in);

            blkCnt = (numCols - l) >> 2;
            f32x4_t vecA;
            while (blkCnt > 0U)
            {
                *(f32x4_t *) pTmpA = *(f32x4_t *) pTmpA * invIn;
                pTmpA += 4;
                /*
                 * Decrement the blockSize loop counter
                 */
                blkCnt--;
            }
            /*
             * tail
             */
            blkCnt = (numCols - l) & 3;
            if (blkCnt > 0U)
            {
                mve_pred16_t p0 = vctp32q(blkCnt);
                

                vecA = vldrwq_f32(pTmpA);
                vecA = vecA * invIn;
                vstrwq_p_f32(pTmpA, vecA, p0);
            }

            pInT1 += numCols - l;
            /*
             * Loop over number of columns
             * * to the right of the pilot element
             */

            pTmpA = pOutT1;
            blkCnt = numCols >> 2;
            while (blkCnt > 0U)
            {
                *(f32x4_t *) pTmpA = *(f32x4_t *) pTmpA *invIn;
                pTmpA += 4;
                /*
                 * Decrement the blockSize loop counter
                 */
                blkCnt--;
            }
            /*
             * tail
             * (will be merged thru tail predication)
             */
            blkCnt = numCols & 3;
            if (blkCnt > 0U)
            {
                mve_pred16_t p0 = vctp32q(blkCnt);

                vecA = vldrwq_f32(pTmpA);
                vecA = vecA * invIn;
                vstrwq_p_f32(pTmpA, vecA, p0);
            }

            pOutT1 += numCols;

            /*
             * Replace the rows with the sum of that row and a multiple of row i
             * * so that each new element in column i above row i is zero.
             */

            /*
             * Temporary pointers for input and destination matrices
             */
            pInT1 = pIn;
            pOutT1 = pOut;

            for (i = 0U; i < numRows; i++)
            {
                /*
                 * Check for the pivot element
                 */
                if (i == l)
                {
                    /*
                     * If the processing element is the pivot element,
                     * only the columns to the right are to be processed
                     */
                    pInT1 += numCols - l;
                    pOutT1 += numCols;
                }
                else
                {
                    /*
                     * Element of the reference row
                     */

                    /*
                     * Working pointers for input and destination pivot rows
                     */
                    pPRT_in = pPivotRowIn;
                    pPRT_pDst = pPivotRowDst;
                    /*
                     * Loop over the number of columns to the right of the pivot element,
                     * to replace the elements in the input matrix
                     */

                    in = *pInT1;
                    f32x4_t tmpV = vdupq_n_f32(in);

                    blkCnt = (numCols - l) >> 2;
                    while (blkCnt > 0U)
                    {
                        f32x4_t vec1, vec2;
                        /*
                         * Replace the element by the sum of that row
                         * and a multiple of the reference row
                         */
                        vec1 = vldrwq_f32(pInT1);
                        vec2 = vldrwq_f32(pPRT_in);
                        vec1 = vfmsq_f32(vec1, tmpV, vec2);
                        vstrwq_f32(pInT1, vec1);
                        pPRT_in += 4;
                        pInT1 += 4;
                        /*
                         * Decrement the blockSize loop counter
                         */
                        blkCnt--;
                    }
                    /*
                     * tail
                     * (will be merged thru tail predication)
                     */
                    blkCnt = (numCols - l) & 3;
                    if (blkCnt > 0U)
                    {
                        f32x4_t vec1, vec2;
                        mve_pred16_t p0 = vctp32q(blkCnt);

                        vec1 = vldrwq_f32(pInT1);
                        vec2 = vldrwq_f32(pPRT_in);
                        vec1 = vfmsq_f32(vec1, tmpV, vec2);
                        vstrwq_p_f32(pInT1, vec1, p0);
                        pInT1 += blkCnt;
                    }

                    blkCnt = numCols >> 2;
                    while (blkCnt > 0U)
                    {
                        f32x4_t vec1, vec2;

                        /*
                         * Replace the element by the sum of that row
                         * and a multiple of the reference row
                         */
                        vec1 = vldrwq_f32(pOutT1);
                        vec2 = vldrwq_f32(pPRT_pDst);
                        vec1 = vfmsq_f32(vec1, tmpV, vec2);
                        vstrwq_f32(pOutT1, vec1);
                        pPRT_pDst += 4;
                        pOutT1 += 4;
                        /*
                         * Decrement the blockSize loop counter
                         */
                        blkCnt--;
                    }
                    /*
                     * tail
                     * (will be merged thru tail predication)
                     */
                    blkCnt = numCols & 3;
                    if (blkCnt > 0U)
                    {
                        f32x4_t vec1, vec2;
                        mve_pred16_t p0 = vctp32q(blkCnt);

                        vec1 = vldrwq_f32(pOutT1);
                        vec2 = vldrwq_f32(pPRT_pDst);
                        vec1 = vfmsq_f32(vec1, tmpV, vec2);
                        vstrwq_p_f32(pOutT1, vec1, p0);

                        pInT2 += blkCnt;
                        pOutT1 += blkCnt;
                    }
                }
                /*
                 * Increment the temporary input pointer
                 */
                pInT1 = pInT1 + l;
            }
            /*
             * Increment the input pointer
             */
            pIn++;
            /*
             * Decrement the loop counter
             */
            loopCnt--;
            /*
             * Increment the index modifier
             */
            l++;
        }

        /*
         * Set status as ARM_MATH_SUCCESS
         */
        status = ARM_MATH_SUCCESS;

        if ((flag != 1U) && (in == 0.0f))
        {
            pIn = pSrc->pData;
            for (i = 0; i < numRows * numCols; i++)
            {
                if (pIn[i] != 0.0f)
                    break;
            }

            if (i == numRows * numCols)
                status = ARM_MATH_SINGULAR;
        }
  }
  /* Return to application */
  return (status);
}

#else
#if defined(ARM_MATH_NEON)
arm_status arm_mat_inverse_f32(
  const arm_matrix_instance_f32 * pSrc,
  arm_matrix_instance_f32 * pDst)
{
  float32_t *pIn = pSrc->pData;                  /* input data matrix pointer */
  float32_t *pOut = pDst->pData;                 /* output data matrix pointer */
  float32_t *pInT1, *pInT2;                      /* Temporary input data matrix pointer */
  float32_t *pOutT1, *pOutT2;                    /* Temporary output data matrix pointer */
  float32_t *pPivotRowIn, *pPRT_in, *pPivotRowDst, *pPRT_pDst;  /* Temporary input and output data matrix pointer */
  uint32_t numRows = pSrc->numRows;              /* Number of rows in the matrix  */
  uint32_t numCols = pSrc->numCols;              /* Number of Cols in the matrix  */


  float32_t Xchg, in = 0.0f, in1;                /* Temporary input values  */
  uint32_t i, rowCnt, flag = 0U, j, loopCnt, k, l;      /* loop counters */
  arm_status status;                             /* status of matrix inverse */
  float32x4_t vec1;
  float32x4_t vec2;
  float32x4_t tmpV;

#ifdef ARM_MATH_MATRIX_CHECK

  /* Check for matrix mismatch condition */
  if ((pSrc->numRows != pSrc->numCols) || (pDst->numRows != pDst->numCols)
     || (pSrc->numRows != pDst->numRows))
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else
#endif /*    #ifdef ARM_MATH_MATRIX_CHECK    */

  {
   /*--------------------------------------------------------------------------------------------------------------
   * Matrix Inverse can be solved using elementary row operations.
   *
   *  Gauss-Jordan Method:
   *
   *     1. First combine the identity matrix and the input matrix separated by a bar to form an
   *        augmented matrix as follows:
   *              _                  _         _         _
   *             |  a11  a12 | 1   0  |       |  X11 X12  |
   *             |           |        |   =   |           |
   *             |_ a21  a22 | 0   1 _|       |_ X21 X21 _|
   *
   *    2. In our implementation, pDst Matrix is used as identity matrix.
   *
   *    3. Begin with the first row. Let i = 1.
   *
   *    4. Check to see if the pivot for row i is zero.
   *       The pivot is the element of the main diagonal that is on the current row.
   *       For instance, if working with row i, then the pivot element is aii.
   *       If the pivot is zero, exchange that row with a row below it that does not
   *       contain a zero in column i. If this is not possible, then an inverse
   *       to that matrix does not exist.
   *
   *      5. Divide every element of row i by the pivot.
   *
   *      6. For every row below and  row i, replace that row with the sum of that row and
   *       a multiple of row i so that each new element in column i below row i is zero.
   *
   *      7. Move to the next row and column and repeat steps 2 through 5 until you have zeros
   *       for every element below and above the main diagonal.
   *
   *    8. Now an identical matrix is formed to the left of the bar(input matrix, pSrc).
   *       Therefore, the matrix to the right of the bar is our solution(pDst matrix, pDst).
   *----------------------------------------------------------------------------------------------------------------*/

    /* Working pointer for destination matrix */
    pOutT1 = pOut;

    /* Loop over the number of rows */
    rowCnt = numRows;

    /* Making the destination matrix as identity matrix */
    while (rowCnt > 0U)
    {
      /* Writing all zeroes in lower triangle of the destination matrix */
      j = numRows - rowCnt;
      while (j > 0U)
      {
        *pOutT1++ = 0.0f;
        j--;
      }

      /* Writing all ones in the diagonal of the destination matrix */
      *pOutT1++ = 1.0f;

      /* Writing all zeroes in upper triangle of the destination matrix */
      j = rowCnt - 1U;

      while (j > 0U)
      {
        *pOutT1++ = 0.0f;
        j--;
      }

      /* Decrement the loop counter */
      rowCnt--;
    }

    /* Loop over the number of columns of the input matrix.
       All the elements in each column are processed by the row operations */
    loopCnt = numCols;

    /* Index modifier to navigate through the columns */
    l = 0U;

    while (loopCnt > 0U)
    {
      /* Check if the pivot element is zero..
       * If it is zero then interchange the row with non zero row below.
       * If there is no non zero element to replace in the rows below,
       * then the matrix is Singular. */

      /* Working pointer for the input matrix that points
       * to the pivot element of the particular row  */
      pInT1 = pIn + (l * numCols);

      /* Working pointer for the destination matrix that points
       * to the pivot element of the particular row  */
      pOutT1 = pOut + (l * numCols);

      /* Temporary variable to hold the pivot value */
      in = *pInT1;

    
      /* Destination pointer modifier */
      k = 1U;

      /* Check if the pivot element is zero */
      if (*pInT1 == 0.0f)
      {
        /* Loop over the number rows present below */
        for (i = (l + 1U); i < numRows; i++)
        {
          /* Update the input and destination pointers */
          pInT2 = pInT1 + (numCols * i);
          pOutT2 = pOutT1 + (numCols * k);

          /* Check if there is a non zero pivot element to
           * replace in the rows below */
          if (*pInT2 != 0.0f)
          {
            /* Loop over number of columns
             * to the right of the pilot element */
            j = numCols - l;

            while (j > 0U)
            {
              /* Exchange the row elements of the input matrix */
              Xchg = *pInT2;
              *pInT2++ = *pInT1;
              *pInT1++ = Xchg;

              /* Decrement the loop counter */
              j--;
            }

            /* Loop over number of columns of the destination matrix */
            j = numCols;

            while (j > 0U)
            {
              /* Exchange the row elements of the destination matrix */
              Xchg = *pOutT2;
              *pOutT2++ = *pOutT1;
              *pOutT1++ = Xchg;

              /* Decrement the loop counter */
              j--;
            }

            /* Flag to indicate whether exchange is done or not */
            flag = 1U;

            /* Break after exchange is done */
            break;
          }

          /* Update the destination pointer modifier */
          k++;
        }
      }

      /* Update the status if the matrix is singular */
      if ((flag != 1U) && (in == 0.0f))
      {
        return ARM_MATH_SINGULAR;
      }

      /* Points to the pivot row of input and destination matrices */
      pPivotRowIn = pIn + (l * numCols);
      pPivotRowDst = pOut + (l * numCols);

      /* Temporary pointers to the pivot row pointers */
      pInT1 = pPivotRowIn;
      pInT2 = pPivotRowDst;

      /* Pivot element of the row */
      in = *pPivotRowIn;
      tmpV = vdupq_n_f32(1.0f/in);

      /* Loop over number of columns
       * to the right of the pilot element */
      j = (numCols - l) >> 2;

      while (j > 0U)
      {
        /* Divide each element of the row of the input matrix
         * by the pivot element */
        vec1 = vld1q_f32(pInT1);

        vec1 = vmulq_f32(vec1, tmpV);
        vst1q_f32(pInT1, vec1);
        pInT1 += 4;

        /* Decrement the loop counter */
        j--;
      }

      /* Tail */
      j = (numCols - l) & 3;

      while (j > 0U)
      {
        /* Divide each element of the row of the input matrix
         * by the pivot element */
        in1 = *pInT1;
        *pInT1++ = in1 / in;

        /* Decrement the loop counter */
        j--;
      }

      /* Loop over number of columns of the destination matrix */
      j = numCols >> 2;

      while (j > 0U)
      {
        /* Divide each element of the row of the destination matrix
         * by the pivot element */
        vec1 = vld1q_f32(pInT2);

        vec1 = vmulq_f32(vec1, tmpV);
        vst1q_f32(pInT2, vec1);
        pInT2 += 4;
      
        /* Decrement the loop counter */
        j--;
      }

      /* Tail */
      j = numCols & 3;

      while (j > 0U)
      {
        /* Divide each element of the row of the destination matrix
         * by the pivot element */
        in1 = *pInT2;
        *pInT2++ = in1 / in;

        /* Decrement the loop counter */
        j--;
      }

      /* Replace the rows with the sum of that row and a multiple of row i
       * so that each new element in column i above row i is zero.*/

      /* Temporary pointers for input and destination matrices */
      pInT1 = pIn;
      pInT2 = pOut;

      /* index used to check for pivot element */
      i = 0U;

      /* Loop over number of rows */
      /*  to be replaced by the sum of that row and a multiple of row i */
      k = numRows;

      while (k > 0U)
      {
        /* Check for the pivot element */
        if (i == l)
        {
          /* If the processing element is the pivot element,
             only the columns to the right are to be processed */
          pInT1 += numCols - l;

          pInT2 += numCols;
        }
        else
        {
          /* Element of the reference row */
          in = *pInT1;
          tmpV = vdupq_n_f32(in);

          /* Working pointers for input and destination pivot rows */
          pPRT_in = pPivotRowIn;
          pPRT_pDst = pPivotRowDst;

          /* Loop over the number of columns to the right of the pivot element,
             to replace the elements in the input matrix */
          j = (numCols - l) >> 2;
	  
          while (j > 0U)
          {
            /* Replace the element by the sum of that row
               and a multiple of the reference row  */
            vec1 = vld1q_f32(pInT1);
            vec2 = vld1q_f32(pPRT_in);
            vec1 = vmlsq_f32(vec1, tmpV, vec2);
            vst1q_f32(pInT1, vec1);
            pPRT_in += 4;
            pInT1 += 4;

            /* Decrement the loop counter */
            j--;
          }

	  /* Tail */
          j = (numCols - l) & 3;

          while (j > 0U)
          {
            /* Replace the element by the sum of that row
               and a multiple of the reference row  */
            in1 = *pInT1;
            *pInT1++ = in1 - (in * *pPRT_in++);

            /* Decrement the loop counter */
            j--;
          }

          /* Loop over the number of columns to
             replace the elements in the destination matrix */
          j = numCols >> 2;

          while (j > 0U)
          {
            /* Replace the element by the sum of that row
               and a multiple of the reference row  */
            vec1 = vld1q_f32(pInT2);
            vec2 = vld1q_f32(pPRT_pDst);
            vec1 = vmlsq_f32(vec1, tmpV, vec2);
            vst1q_f32(pInT2, vec1);
            pPRT_pDst += 4;
            pInT2 += 4;

            /* Decrement the loop counter */
            j--;
          }

	  /* Tail */
          j = numCols & 3;

          while (j > 0U)
          {
            /* Replace the element by the sum of that row
               and a multiple of the reference row  */
            in1 = *pInT2;
            *pInT2++ = in1 - (in * *pPRT_pDst++);

            /* Decrement the loop counter */
            j--;
          }

        }

        /* Increment the temporary input pointer */
        pInT1 = pInT1 + l;

        /* Decrement the loop counter */
        k--;

        /* Increment the pivot index */
        i++;
      }

      /* Increment the input pointer */
      pIn++;

      /* Decrement the loop counter */
      loopCnt--;

      /* Increment the index modifier */
      l++;
    }

    /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;

    if ((flag != 1U) && (in == 0.0f))
    {
      pIn = pSrc->pData;
      for (i = 0; i < numRows * numCols; i++)
      {
        if (pIn[i] != 0.0f)
            break;
      }

      if (i == numRows * numCols)
        status = ARM_MATH_SINGULAR;
    }
  }
  /* Return to application */
  return (status);
}
#else
arm_status arm_mat_inverse_f32(
  const arm_matrix_instance_f32 * pSrc,
        arm_matrix_instance_f32 * pDst)
{
  float32_t *pIn = pSrc->pData;                  /* input data matrix pointer */
  float32_t *pOut = pDst->pData;                 /* output data matrix pointer */
  float32_t *pInT1, *pInT2;                      /* Temporary input data matrix pointer */
  float32_t *pOutT1, *pOutT2;                    /* Temporary output data matrix pointer */
  float32_t *pPivotRowIn, *pPRT_in, *pPivotRowDst, *pPRT_pDst;  /* Temporary input and output data matrix pointer */
  uint32_t numRows = pSrc->numRows;              /* Number of rows in the matrix  */
  uint32_t numCols = pSrc->numCols;              /* Number of Cols in the matrix  */

#if defined (ARM_MATH_DSP)

  float32_t Xchg, in = 0.0f, in1;                /* Temporary input values  */
  uint32_t i, rowCnt, flag = 0U, j, loopCnt, k, l;      /* loop counters */
  arm_status status;                             /* status of matrix inverse */

#ifdef ARM_MATH_MATRIX_CHECK

  /* Check for matrix mismatch condition */
  if ((pSrc->numRows != pSrc->numCols) ||
      (pDst->numRows != pDst->numCols) ||
      (pSrc->numRows != pDst->numRows)   )
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else

#endif /* #ifdef ARM_MATH_MATRIX_CHECK */

  {

    /*--------------------------------------------------------------------------------------------------------------
     * Matrix Inverse can be solved using elementary row operations.
     *
     *  Gauss-Jordan Method:
     *
     *      1. First combine the identity matrix and the input matrix separated by a bar to form an
     *        augmented matrix as follows:
     *                      _                  _         _         _
     *                     |  a11  a12 | 1   0  |       |  X11 X12  |
     *                     |           |        |   =   |           |
     *                     |_ a21  a22 | 0   1 _|       |_ X21 X21 _|
     *
     *      2. In our implementation, pDst Matrix is used as identity matrix.
     *
     *      3. Begin with the first row. Let i = 1.
     *
     *      4. Check to see if the pivot for row i is zero.
     *         The pivot is the element of the main diagonal that is on the current row.
     *         For instance, if working with row i, then the pivot element is aii.
     *         If the pivot is zero, exchange that row with a row below it that does not
     *         contain a zero in column i. If this is not possible, then an inverse
     *         to that matrix does not exist.
     *
     *      5. Divide every element of row i by the pivot.
     *
     *      6. For every row below and  row i, replace that row with the sum of that row and
     *         a multiple of row i so that each new element in column i below row i is zero.
     *
     *      7. Move to the next row and column and repeat steps 2 through 5 until you have zeros
     *         for every element below and above the main diagonal.
     *
     *      8. Now an identical matrix is formed to the left of the bar(input matrix, pSrc).
     *         Therefore, the matrix to the right of the bar is our solution(pDst matrix, pDst).
     *----------------------------------------------------------------------------------------------------------------*/

    /* Working pointer for destination matrix */
    pOutT1 = pOut;

    /* Loop over the number of rows */
    rowCnt = numRows;

    /* Making the destination matrix as identity matrix */
    while (rowCnt > 0U)
    {
      /* Writing all zeroes in lower triangle of the destination matrix */
      j = numRows - rowCnt;
      while (j > 0U)
      {
        *pOutT1++ = 0.0f;
        j--;
      }

      /* Writing all ones in the diagonal of the destination matrix */
      *pOutT1++ = 1.0f;

      /* Writing all zeroes in upper triangle of the destination matrix */
      j = rowCnt - 1U;
      while (j > 0U)
      {
        *pOutT1++ = 0.0f;
        j--;
      }

      /* Decrement loop counter */
      rowCnt--;
    }

    /* Loop over the number of columns of the input matrix.
       All the elements in each column are processed by the row operations */
    loopCnt = numCols;

    /* Index modifier to navigate through the columns */
    l = 0U;

    while (loopCnt > 0U)
    {
      /* Check if the pivot element is zero..
       * If it is zero then interchange the row with non zero row below.
       * If there is no non zero element to replace in the rows below,
       * then the matrix is Singular. */

      /* Working pointer for the input matrix that points
       * to the pivot element of the particular row  */
      pInT1 = pIn + (l * numCols);

      /* Working pointer for the destination matrix that points
       * to the pivot element of the particular row  */
      pOutT1 = pOut + (l * numCols);

      /* Temporary variable to hold the pivot value */
      in = *pInT1;

      
      /* Destination pointer modifier */
      k = 1U;

      /* Check if the pivot element is zero */
      if (*pInT1 == 0.0f)
      {
        /* Loop over the number rows present below */

        for (i = (l + 1U); i < numRows; i++)
        {
          /* Update the input and destination pointers */
          pInT2 = pInT1 + (numCols * i);
          pOutT2 = pOutT1 + (numCols * k);

          /* Check if there is a non zero pivot element to
           * replace in the rows below */
          if (*pInT2 != 0.0f)
          {
            /* Loop over number of columns
             * to the right of the pilot element */
            j = numCols - l;

            while (j > 0U)
            {
              /* Exchange the row elements of the input matrix */
              Xchg = *pInT2;
              *pInT2++ = *pInT1;
              *pInT1++ = Xchg;

              /* Decrement the loop counter */
              j--;
            }

            /* Loop over number of columns of the destination matrix */
            j = numCols;

            while (j > 0U)
            {
              /* Exchange the row elements of the destination matrix */
              Xchg = *pOutT2;
              *pOutT2++ = *pOutT1;
              *pOutT1++ = Xchg;

              /* Decrement loop counter */
              j--;
            }

            /* Flag to indicate whether exchange is done or not */
            flag = 1U;

            /* Break after exchange is done */
            break;
          }

          /* Update the destination pointer modifier */
          k++;

          /* Decrement loop counter */
        }
      }

      /* Update the status if the matrix is singular */
      if ((flag != 1U) && (in == 0.0f))
      {
        return ARM_MATH_SINGULAR;
      }

      /* Points to the pivot row of input and destination matrices */
      pPivotRowIn = pIn + (l * numCols);
      pPivotRowDst = pOut + (l * numCols);

      /* Temporary pointers to the pivot row pointers */
      pInT1 = pPivotRowIn;
      pInT2 = pPivotRowDst;

      /* Pivot element of the row */
      in = *pPivotRowIn;

      /* Loop over number of columns
       * to the right of the pilot element */
      j = (numCols - l);

      while (j > 0U)
      {
        /* Divide each element of the row of the input matrix
         * by the pivot element */
        in1 = *pInT1;
        *pInT1++ = in1 / in;

        /* Decrement the loop counter */
        j--;
      }

      /* Loop over number of columns of the destination matrix */
      j = numCols;

      while (j > 0U)
      {
        /* Divide each element of the row of the destination matrix
         * by the pivot element */
        in1 = *pInT2;
        *pInT2++ = in1 / in;

        /* Decrement the loop counter */
        j--;
      }

      /* Replace the rows with the sum of that row and a multiple of row i
       * so that each new element in column i above row i is zero.*/

      /* Temporary pointers for input and destination matrices */
      pInT1 = pIn;
      pInT2 = pOut;

      /* index used to check for pivot element */
      i = 0U;

      /* Loop over number of rows */
      /*  to be replaced by the sum of that row and a multiple of row i */
      k = numRows;

      while (k > 0U)
      {
        /* Check for the pivot element */
        if (i == l)
        {
          /* If the processing element is the pivot element,
             only the columns to the right are to be processed */
          pInT1 += numCols - l;

          pInT2 += numCols;
        }
        else
        {
          /* Element of the reference row */
          in = *pInT1;

          /* Working pointers for input and destination pivot rows */
          pPRT_in = pPivotRowIn;
          pPRT_pDst = pPivotRowDst;

          /* Loop over the number of columns to the right of the pivot element,
             to replace the elements in the input matrix */
          j = (numCols - l);

          while (j > 0U)
          {
            /* Replace the element by the sum of that row
               and a multiple of the reference row  */
            in1 = *pInT1;
            *pInT1++ = in1 - (in * *pPRT_in++);

            /* Decrement the loop counter */
            j--;
          }

          /* Loop over the number of columns to
             replace the elements in the destination matrix */
          j = numCols;

          while (j > 0U)
          {
            /* Replace the element by the sum of that row
               and a multiple of the reference row  */
            in1 = *pInT2;
            *pInT2++ = in1 - (in * *pPRT_pDst++);

            /* Decrement loop counter */
            j--;
          }

        }

        /* Increment temporary input pointer */
        pInT1 = pInT1 + l;

        /* Decrement loop counter */
        k--;

        /* Increment pivot index */
        i++;
      }

      /* Increment the input pointer */
      pIn++;

      /* Decrement the loop counter */
      loopCnt--;

      /* Increment the index modifier */
      l++;
    }


#else

  float32_t Xchg, in = 0.0f;                     /* Temporary input values  */
  uint32_t i, rowCnt, flag = 0U, j, loopCnt, k, l;      /* loop counters */
  arm_status status;                             /* status of matrix inverse */

#ifdef ARM_MATH_MATRIX_CHECK

  /* Check for matrix mismatch condition */
  if ((pSrc->numRows != pSrc->numCols) ||
      (pDst->numRows != pDst->numCols) ||
      (pSrc->numRows != pDst->numRows)   )
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else

#endif /* #ifdef ARM_MATH_MATRIX_CHECK */

  {

    /*--------------------------------------------------------------------------------------------------------------
     * Matrix Inverse can be solved using elementary row operations.
     *
     *  Gauss-Jordan Method:
     *
     *      1. First combine the identity matrix and the input matrix separated by a bar to form an
     *        augmented matrix as follows:
     *                      _  _          _     _      _   _         _         _
     *                     |  |  a11  a12  | | | 1   0  |   |       |  X11 X12  |
     *                     |  |            | | |        |   |   =   |           |
     *                     |_ |_ a21  a22 _| | |_0   1 _|  _|       |_ X21 X21 _|
     *
     *      2. In our implementation, pDst Matrix is used as identity matrix.
     *
     *      3. Begin with the first row. Let i = 1.
     *
     *      4. Check to see if the pivot for row i is zero.
     *         The pivot is the element of the main diagonal that is on the current row.
     *         For instance, if working with row i, then the pivot element is aii.
     *         If the pivot is zero, exchange that row with a row below it that does not
     *         contain a zero in column i. If this is not possible, then an inverse
     *         to that matrix does not exist.
     *
     *      5. Divide every element of row i by the pivot.
     *
     *      6. For every row below and  row i, replace that row with the sum of that row and
     *         a multiple of row i so that each new element in column i below row i is zero.
     *
     *      7. Move to the next row and column and repeat steps 2 through 5 until you have zeros
     *         for every element below and above the main diagonal.
     *
     *      8. Now an identical matrix is formed to the left of the bar(input matrix, src).
     *         Therefore, the matrix to the right of the bar is our solution(dst matrix, dst).
     *----------------------------------------------------------------------------------------------------------------*/

    /* Working pointer for destination matrix */
    pOutT1 = pOut;

    /* Loop over the number of rows */
    rowCnt = numRows;

    /* Making the destination matrix as identity matrix */
    while (rowCnt > 0U)
    {
      /* Writing all zeroes in lower triangle of the destination matrix */
      j = numRows - rowCnt;
      while (j > 0U)
      {
        *pOutT1++ = 0.0f;
        j--;
      }

      /* Writing all ones in the diagonal of the destination matrix */
      *pOutT1++ = 1.0f;

      /* Writing all zeroes in upper triangle of the destination matrix */
      j = rowCnt - 1U;
      while (j > 0U)
      {
        *pOutT1++ = 0.0f;
        j--;
      }

      /* Decrement loop counter */
      rowCnt--;
    }

    /* Loop over the number of columns of the input matrix.
       All the elements in each column are processed by the row operations */
    loopCnt = numCols;

    /* Index modifier to navigate through the columns */
    l = 0U;

    while (loopCnt > 0U)
    {
      /* Check if the pivot element is zero..
       * If it is zero then interchange the row with non zero row below.
       * If there is no non zero element to replace in the rows below,
       * then the matrix is Singular. */

      /* Working pointer for the input matrix that points
       * to the pivot element of the particular row  */
      pInT1 = pIn + (l * numCols);

      /* Working pointer for the destination matrix that points
       * to the pivot element of the particular row  */
      pOutT1 = pOut + (l * numCols);

      /* Temporary variable to hold the pivot value */
      in = *pInT1;

      /* Destination pointer modifier */
      k = 1U;

      /* Check if the pivot element is zero */
      if (*pInT1 == 0.0f)
      {
        /* Loop over the number rows present below */
        for (i = (l + 1U); i < numRows; i++)
        {
          /* Update the input and destination pointers */
          pInT2 = pInT1 + (numCols * i);
          pOutT2 = pOutT1 + (numCols * k);

          /* Check if there is a non zero pivot element to
           * replace in the rows below */
          if (*pInT2 != 0.0f)
          {
            /* Loop over number of columns
             * to the right of the pilot element */
            for (j = 0U; j < (numCols - l); j++)
            {
              /* Exchange the row elements of the input matrix */
              Xchg = *pInT2;
              *pInT2++ = *pInT1;
              *pInT1++ = Xchg;
            }

            for (j = 0U; j < numCols; j++)
            {
              Xchg = *pOutT2;
              *pOutT2++ = *pOutT1;
              *pOutT1++ = Xchg;
            }

            /* Flag to indicate whether exchange is done or not */
            flag = 1U;

            /* Break after exchange is done */
            break;
          }

          /* Update the destination pointer modifier */
          k++;
        }
      }

      /* Update the status if the matrix is singular */
      if ((flag != 1U) && (in == 0.0f))
      {
        return ARM_MATH_SINGULAR;
      }

      /* Points to the pivot row of input and destination matrices */
      pPivotRowIn = pIn + (l * numCols);
      pPivotRowDst = pOut + (l * numCols);

      /* Temporary pointers to the pivot row pointers */
      pInT1 = pPivotRowIn;
      pOutT1 = pPivotRowDst;

      /* Pivot element of the row */
      in = *(pIn + (l * numCols));

      /* Loop over number of columns
       * to the right of the pilot element */
      for (j = 0U; j < (numCols - l); j++)
      {
        /* Divide each element of the row of the input matrix
         * by the pivot element */
        *pInT1 = *pInT1 / in;
        pInT1++;
      }
      for (j = 0U; j < numCols; j++)
      {
        /* Divide each element of the row of the destination matrix
         * by the pivot element */
        *pOutT1 = *pOutT1 / in;
        pOutT1++;
      }

      /* Replace the rows with the sum of that row and a multiple of row i
       * so that each new element in column i above row i is zero.*/

      /* Temporary pointers for input and destination matrices */
      pInT1 = pIn;
      pOutT1 = pOut;

      for (i = 0U; i < numRows; i++)
      {
        /* Check for the pivot element */
        if (i == l)
        {
          /* If the processing element is the pivot element,
             only the columns to the right are to be processed */
          pInT1 += numCols - l;
          pOutT1 += numCols;
        }
        else
        {
          /* Element of the reference row */
          in = *pInT1;

          /* Working pointers for input and destination pivot rows */
          pPRT_in = pPivotRowIn;
          pPRT_pDst = pPivotRowDst;

          /* Loop over the number of columns to the right of the pivot element,
             to replace the elements in the input matrix */
          for (j = 0U; j < (numCols - l); j++)
          {
            /* Replace the element by the sum of that row
               and a multiple of the reference row  */
            *pInT1 = *pInT1 - (in * *pPRT_in++);
            pInT1++;
          }

          /* Loop over the number of columns to
             replace the elements in the destination matrix */
          for (j = 0U; j < numCols; j++)
          {
            /* Replace the element by the sum of that row
               and a multiple of the reference row  */
            *pOutT1 = *pOutT1 - (in * *pPRT_pDst++);
            pOutT1++;
          }

        }

        /* Increment temporary input pointer */
        pInT1 = pInT1 + l;
      }

      /* Increment the input pointer */
      pIn++;

      /* Decrement the loop counter */
      loopCnt--;

      /* Increment the index modifier */
      l++;
    }

#endif /* #if defined (ARM_MATH_DSP) */

    /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;

    if ((flag != 1U) && (in == 0.0f))
    {
      pIn = pSrc->pData;
      for (i = 0; i < numRows * numCols; i++)
      {
        if (pIn[i] != 0.0f)
            break;
      }

      if (i == numRows * numCols)
        status = ARM_MATH_SINGULAR;
    }
  }

  /* Return to application */
  return (status);
}
#endif /* #if defined(ARM_MATH_NEON) */
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of MatrixInv group
 */
