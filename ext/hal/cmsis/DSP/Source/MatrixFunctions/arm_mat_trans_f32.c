/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mat_trans_f32.c
 * Description:  Floating-point matrix transpose
 *
 * $Date:        18. March 2019
 * $Revision:    V1.6.0
 *
 * Target Processor: Cortex-M cores
 * -------------------------------------------------------------------- */
/*
 * Copyright (C) 2010-2017 ARM Limited or its affiliates. All rights reserved.
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
  @defgroup MatrixTrans Matrix Transpose

  Tranposes a matrix.

  Transposing an <code>M x N</code> matrix flips it around the center diagonal and results in an <code>N x M</code> matrix.
  \image html MatrixTranspose.gif "Transpose of a 3 x 3 matrix"
 */

/**
  @addtogroup MatrixTrans
  @{
 */

/**
  @brief         Floating-point matrix transpose.
  @param[in]     pSrc      points to input matrix
  @param[out]    pDst      points to output matrix
  @return        execution status
                   - \ref ARM_MATH_SUCCESS       : Operation successful
                   - \ref ARM_MATH_SIZE_MISMATCH : Matrix size check failed
 */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"

arm_status arm_mat_trans_f32(
  const arm_matrix_instance_f32 * pSrc,
  arm_matrix_instance_f32 * pDst)
{
  arm_status status;                             /* status of matrix transpose  */

#ifdef ARM_MATH_MATRIX_CHECK

  /* Check for matrix mismatch condition */
  if ((pSrc->numRows != pDst->numCols) || (pSrc->numCols != pDst->numRows))
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else
#endif /*    #ifdef ARM_MATH_MATRIX_CHECK    */
  {
    if (pDst->numRows == pDst->numCols)
    {
        if (pDst->numCols == 2)
            return arm_mat_trans_32bit_2x2_mve((uint32_t *)pSrc->pData, (uint32_t *)pDst->pData);
        if (pDst->numCols == 3)
            return arm_mat_trans_32bit_3x3_mve((uint32_t *)pSrc->pData, (uint32_t *)pDst->pData);
        if (pDst->numCols == 4)
            return arm_mat_trans_32bit_4x4_mve((uint32_t *)pSrc->pData, (uint32_t *)pDst->pData);
    }

    arm_mat_trans_32bit_generic_mve(pSrc->numRows, pSrc->numCols, (uint32_t *)pSrc->pData, (uint32_t *)pDst->pData);
    /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}

#else
#if defined(ARM_MATH_NEON)

arm_status arm_mat_trans_f32(
  const arm_matrix_instance_f32 * pSrc,
  arm_matrix_instance_f32 * pDst)
{
  float32_t *pIn = pSrc->pData;                  /* input data matrix pointer */
  float32_t *pOut = pDst->pData;                 /* output data matrix pointer */
  float32_t *px;                                 /* Temporary output data matrix pointer */
  uint16_t nRows = pSrc->numRows;                /* number of rows */
  uint16_t nColumns = pSrc->numCols;             /* number of columns */

  uint16_t blkCnt, rowCnt, i = 0U, row = nRows;          /* loop counters */
  arm_status status;                             /* status of matrix transpose  */

#ifdef ARM_MATH_MATRIX_CHECK

  /* Check for matrix mismatch condition */
  if ((pSrc->numRows != pDst->numCols) || (pSrc->numCols != pDst->numRows))
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else
#endif /*    #ifdef ARM_MATH_MATRIX_CHECK    */

  {
    /* Matrix transpose by exchanging the rows with columns */
    /* Row loop */
    rowCnt = row >> 2;
    while (rowCnt > 0U)
    {
      float32x4_t row0V,row1V,row2V,row3V;
      float32x4x2_t ra0,ra1,rb0,rb1;

      blkCnt = nColumns >> 2;

      /* The pointer px is set to starting address of the column being processed */
      px = pOut + i;

      /* Compute 4 outputs at a time.
       ** a second loop below computes the remaining 1 to 3 samples. */
      while (blkCnt > 0U)        /* Column loop */
      {
        row0V = vld1q_f32(pIn);
        row1V = vld1q_f32(pIn + 1 * nColumns);
        row2V = vld1q_f32(pIn + 2 * nColumns);
        row3V = vld1q_f32(pIn + 3 * nColumns);
        pIn += 4;

        ra0 = vzipq_f32(row0V,row2V);
        ra1 = vzipq_f32(row1V,row3V);

        rb0 = vzipq_f32(ra0.val[0],ra1.val[0]);
        rb1 = vzipq_f32(ra0.val[1],ra1.val[1]);

        vst1q_f32(px,rb0.val[0]);
        px += nRows;

        vst1q_f32(px,rb0.val[1]);
        px += nRows;

        vst1q_f32(px,rb1.val[0]);
        px += nRows;

        vst1q_f32(px,rb1.val[1]);
        px += nRows;

        /* Decrement the column loop counter */
        blkCnt--;
      }

      /* Perform matrix transpose for last 3 samples here. */
      blkCnt = nColumns % 0x4U;

      while (blkCnt > 0U)
      {
        /* Read and store the input element in the destination */
        *px++ = *pIn;
        *px++ = *(pIn + 1 * nColumns);
        *px++ = *(pIn + 2 * nColumns);
        *px++ = *(pIn + 3 * nColumns);
        
        px += (nRows - 4);
        pIn++;

        /* Decrement the column loop counter */
        blkCnt--;
      }

      i += 4;
      pIn += 3 * nColumns;

      /* Decrement the row loop counter */
      rowCnt--;

    }         /* Row loop end  */

    rowCnt = row & 3;
    while (rowCnt > 0U)
    {
      blkCnt = nColumns ;
      /* The pointer px is set to starting address of the column being processed */
      px = pOut + i;

      while (blkCnt > 0U)
      {
        /* Read and store the input element in the destination */
        *px = *pIn++;

        /* Update the pointer px to point to the next row of the transposed matrix */
        px += nRows;

        /* Decrement the column loop counter */
        blkCnt--;
      }
      i++;
      rowCnt -- ;
    }

    /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}
#else
arm_status arm_mat_trans_f32(
  const arm_matrix_instance_f32 * pSrc,
        arm_matrix_instance_f32 * pDst)
{
  float32_t *pIn = pSrc->pData;                  /* input data matrix pointer */
  float32_t *pOut = pDst->pData;                 /* output data matrix pointer */
  float32_t *px;                                 /* Temporary output data matrix pointer */
  uint16_t nRows = pSrc->numRows;                /* number of rows */
  uint16_t nCols = pSrc->numCols;                /* number of columns */
  uint32_t col, row = nRows, i = 0U;             /* Loop counters */
  arm_status status;                             /* status of matrix transpose */

#ifdef ARM_MATH_MATRIX_CHECK

  /* Check for matrix mismatch condition */
  if ((pSrc->numRows != pDst->numCols) ||
      (pSrc->numCols != pDst->numRows)   )
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else

#endif /* #ifdef ARM_MATH_MATRIX_CHECK */

  {
    /* Matrix transpose by exchanging the rows with columns */
    /* row loop */
    do
    {
      /* Pointer px is set to starting address of column being processed */
      px = pOut + i;

#if defined (ARM_MATH_LOOPUNROLL)

      /* Loop unrolling: Compute 4 outputs at a time */
      col = nCols >> 2U;

      while (col > 0U)        /* column loop */
      {
        /* Read and store input element in destination */
        *px = *pIn++;
        /* Update pointer px to point to next row of transposed matrix */
        px += nRows;

        *px = *pIn++;
        px += nRows;

        *px = *pIn++;
        px += nRows;

        *px = *pIn++;
        px += nRows;

        /* Decrement column loop counter */
        col--;
      }

      /* Loop unrolling: Compute remaining outputs */
      col = nCols % 0x4U;

#else

      /* Initialize col with number of samples */
      col = nCols;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

      while (col > 0U)
      {
        /* Read and store input element in destination */
        *px = *pIn++;

        /* Update pointer px to point to next row of transposed matrix */
        px += nRows;

        /* Decrement column loop counter */
        col--;
      }

      i++;

      /* Decrement row loop counter */
      row--;

    } while (row > 0U);          /* row loop end */

    /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}
#endif /* #if defined(ARM_MATH_NEON) */
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
 * @} end of MatrixTrans group
 */
