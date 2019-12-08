/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mat_trans_q15.c
 * Description:  Q15 matrix transpose
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
  @addtogroup MatrixTrans
  @{
 */

/**
  @brief         Q15 matrix transpose.
  @param[in]     pSrc      points to input matrix
  @param[out]    pDst      points to output matrix
  @return        execution status
                   - \ref ARM_MATH_SUCCESS       : Operation successful
                   - \ref ARM_MATH_SIZE_MISMATCH : Matrix size check failed
 */
 
#if defined(ARM_MATH_MVEI)

__STATIC_INLINE arm_status arm_mat_trans_16bit_2x2(uint16_t * pDataSrc, uint16_t * pDataDest)
{
    pDataDest[0] = pDataSrc[0];
    pDataDest[3] = pDataSrc[3];
    pDataDest[2] = pDataSrc[1];
    pDataDest[1] = pDataSrc[2];

    return (ARM_MATH_SUCCESS);
}

static arm_status arm_mat_trans_16bit_3x3_mve(uint16_t * pDataSrc, uint16_t * pDataDest)
{
    static const uint16_t stridesTr33[8] = { 0, 3, 6, 1, 4, 7, 2, 5 };
    uint16x8_t    vecOffs1;
    uint16x8_t    vecIn1;
    /*
     *
     *  | 0   1   2 |       | 0   3   6 |  8 x 16 flattened version | 0   3   6   1   4   7   2   5 |
     *  | 3   4   5 | =>    | 1   4   7 |            =>             | 8   .   .   .   .   .   .   . |
     *  | 6   7   8 |       | 2   5   8 |       (row major)
     *
     */
    vecOffs1 = vldrhq_u16((uint16_t const *) stridesTr33);
    vecIn1 = vldrhq_u16((uint16_t const *) pDataSrc);

    vstrhq_scatter_shifted_offset_u16(pDataDest, vecOffs1, vecIn1);

    pDataDest[8] = pDataSrc[8];

    return (ARM_MATH_SUCCESS);
}


static arm_status arm_mat_trans_16bit_4x4_mve(uint16_t * pDataSrc, uint16_t * pDataDest)
{
    static const uint16_t stridesTr44_1[8] = { 0, 4, 8, 12, 1, 5, 9, 13 };
    static const uint16_t stridesTr44_2[8] = { 2, 6, 10, 14, 3, 7, 11, 15 };
    uint16x8_t    vecOffs1, vecOffs2;
    uint16x8_t    vecIn1, vecIn2;
    uint16_t const * pDataSrcVec = (uint16_t const *) pDataSrc;

    /*
     * 4x4 Matrix transposition
     *
     * | 0   1   2   3  |       | 0   4   8   12 |   8 x 16 flattened version
     * | 4   5   6   7  |  =>   | 1   5   9   13 |   =>      [0   4   8   12  1   5   9   13]
     * | 8   9   10  11 |       | 2   6   10  14 |           [2   6   10  14  3   7   11  15]
     * | 12  13  14  15 |       | 3   7   11  15 |
     */

    vecOffs1 = vldrhq_u16((uint16_t const *) stridesTr44_1);
    vecOffs2 = vldrhq_u16((uint16_t const *) stridesTr44_2);
    vecIn1 = vldrhq_u16(pDataSrcVec);
    pDataSrcVec += 8;
    vecIn2 = vldrhq_u16(pDataSrcVec);

    vstrhq_scatter_shifted_offset_u16(pDataDest, vecOffs1, vecIn1);
    vstrhq_scatter_shifted_offset_u16(pDataDest, vecOffs2, vecIn2);


    return (ARM_MATH_SUCCESS);
}



static arm_status arm_mat_trans_16bit_generic(
    uint16_t    srcRows,
    uint16_t    srcCols,
    uint16_t  * pDataSrc,
    uint16_t  * pDataDest)
{
    uint16x8_t    vecOffs;
    uint32_t        i;
    uint32_t        blkCnt;
    uint16_t const *pDataC;
    uint16_t       *pDataDestR;
    uint16x8_t    vecIn;

    vecOffs = vidupq_u16(0, 1);
    vecOffs = vecOffs * srcCols;

    i = srcCols;
    while(i > 0U)
    {
        pDataC = (uint16_t const *) pDataSrc;
        pDataDestR = pDataDest;

        blkCnt = srcRows >> 3;
        while (blkCnt > 0U)
        {
            vecIn = vldrhq_gather_shifted_offset_u16(pDataC, vecOffs);
            vstrhq_u16(pDataDestR, vecIn); 
            pDataDestR += 8;
            pDataC = pDataC + srcCols * 8;
            /*
             * Decrement the blockSize loop counter
             */
            blkCnt--;
        }

        /*
         * tail
         */
        blkCnt = srcRows & 7;
        if (blkCnt > 0U)
        {
            mve_pred16_t p0 = vctp16q(blkCnt);
            vecIn = vldrhq_gather_shifted_offset_u16(pDataC, vecOffs);
            vstrhq_p_u16(pDataDestR, vecIn, p0);
        }
        pDataSrc += 1;
        pDataDest += srcRows;
        i--;
    }

    return (ARM_MATH_SUCCESS);
}


arm_status arm_mat_trans_q15(
  const arm_matrix_instance_q15 * pSrc,
        arm_matrix_instance_q15 * pDst)
{
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
    if (pDst->numRows == pDst->numCols)
    {
        if (pDst->numCols == 1)
        {
          pDst->pData[0] = pSrc->pData[0];
          return(ARM_MATH_SUCCESS);
        }
        if (pDst->numCols == 2)
            return arm_mat_trans_16bit_2x2((uint16_t  *)pSrc->pData, (uint16_t  *)pDst->pData);
        if (pDst->numCols == 3)
            return arm_mat_trans_16bit_3x3_mve((uint16_t  *)pSrc->pData, (uint16_t  *)pDst->pData);
        if (pDst->numCols == 4)
            return arm_mat_trans_16bit_4x4_mve((uint16_t  *)pSrc->pData, (uint16_t  *)pDst->pData);
    }

    arm_mat_trans_16bit_generic(pSrc->numRows, pSrc->numCols, (uint16_t  *)pSrc->pData, (uint16_t  *)pDst->pData);
      /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}
#else
arm_status arm_mat_trans_q15(
  const arm_matrix_instance_q15 * pSrc,
        arm_matrix_instance_q15 * pDst)
{
        q15_t *pIn = pSrc->pData;                      /* input data matrix pointer */
        q15_t *pOut = pDst->pData;                     /* output data matrix pointer */
        uint16_t nRows = pSrc->numRows;                /* number of rows */
        uint16_t nCols = pSrc->numCols;                /* number of columns */
        uint32_t col, row = nRows, i = 0U;             /* Loop counters */
        arm_status status;                             /* status of matrix transpose */

#if defined (ARM_MATH_LOOPUNROLL)
        q31_t in;                                      /* variable to hold temporary output  */
#endif

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
      /* Pointer pOut is set to starting address of column being processed */
      pOut = pDst->pData + i;

#if defined (ARM_MATH_LOOPUNROLL)

      /* Loop unrolling: Compute 4 outputs at a time */
      col = nCols >> 2U;

      while (col > 0U)        /* column loop */
      {
        /* Read two elements from row */
        in = read_q15x2_ia ((q15_t **) &pIn);

        /* Unpack and store one element in  destination */
#ifndef ARM_MATH_BIG_ENDIAN
        *pOut = (q15_t) in;
#else
        *pOut = (q15_t) ((in & (q31_t) 0xffff0000) >> 16);
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

        /* Update pointer pOut to point to next row of transposed matrix */
        pOut += nRows;

        /* Unpack and store second element in destination */
#ifndef ARM_MATH_BIG_ENDIAN
        *pOut = (q15_t) ((in & (q31_t) 0xffff0000) >> 16);
#else
        *pOut = (q15_t) in;
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

        /* Update  pointer pOut to point to next row of transposed matrix */
        pOut += nRows;

        /* Read two elements from row */
        in = read_q15x2_ia ((q15_t **) &pIn);

        /* Unpack and store one element in destination */
#ifndef ARM_MATH_BIG_ENDIAN
        *pOut = (q15_t) in;
#else
        *pOut = (q15_t) ((in & (q31_t) 0xffff0000) >> 16);

#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

        /* Update pointer pOut to point to next row of transposed matrix */
        pOut += nRows;

        /* Unpack and store second element in destination */
#ifndef ARM_MATH_BIG_ENDIAN
        *pOut = (q15_t) ((in & (q31_t) 0xffff0000) >> 16);
#else
        *pOut = (q15_t) in;
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

        /* Update pointer pOut to point to next row of transposed matrix */
        pOut += nRows;

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
        *pOut = *pIn++;

        /* Update pointer pOut to point to next row of transposed matrix */
        pOut += nRows;

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
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of MatrixTrans group
 */
