/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mat_scale_f32.c
 * Description:  Multiplies a floating-point matrix by a scalar
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
  @defgroup MatrixScale Matrix Scale

  Multiplies a matrix by a scalar.  This is accomplished by multiplying each element in the
  matrix by the scalar.  For example:
  \image html MatrixScale.gif "Matrix Scaling of a 3 x 3 matrix"

  The function checks to make sure that the input and output matrices are of the same size.

  In the fixed-point Q15 and Q31 functions, <code>scale</code> is represented by
  a fractional multiplication <code>scaleFract</code> and an arithmetic shift <code>shift</code>.
  The shift allows the gain of the scaling operation to exceed 1.0.
  The overall scale factor applied to the fixed-point data is
  <pre>
      scale = scaleFract * 2^shift.
  </pre>
 */

/**
  @addtogroup MatrixScale
  @{
 */

/**
  @brief         Floating-point matrix scaling.
  @param[in]     pSrc       points to input matrix
  @param[in]     scale      scale factor to be applied
  @param[out]    pDst       points to output matrix structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS       : Operation successful
                   - \ref ARM_MATH_SIZE_MISMATCH : Matrix size check failed
 */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
arm_status arm_mat_scale_f32(
  const arm_matrix_instance_f32 * pSrc,
  float32_t scale,
  arm_matrix_instance_f32 * pDst)
{
  arm_status status;                             /* status of matrix scaling     */
  #ifdef ARM_MATH_MATRIX_CHECK
  /* Check for matrix mismatch condition */
  if ((pSrc->numRows != pDst->numRows) || (pSrc->numCols != pDst->numCols))
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else
#endif /*    #ifdef ARM_MATH_MATRIX_CHECK    */
  {
    float32_t *pIn = pSrc->pData;   /* input data matrix pointer */
    float32_t *pOut = pDst->pData;  /* output data matrix pointer */
    uint32_t  numSamples;           /* total number of elements in the matrix */
    uint32_t  blkCnt;               /* loop counters */
    f32x4_t vecIn, vecOut;
    float32_t const *pInVec;

    pInVec = (float32_t const *) pIn;
    /*
     * Total number of samples in the input matrix
     */
    numSamples = (uint32_t) pSrc->numRows * pSrc->numCols;
    blkCnt = numSamples >> 2;
    while (blkCnt > 0U)
    {
        /*
         * C(m,n) = A(m,n) * scale
         * Scaling and results are stored in the destination buffer.
         */
        vecIn = vld1q(pInVec); 
        pInVec += 4;

        vecOut = vecIn * scale;

        vst1q(pOut, vecOut); 
        pOut += 4;
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
        vecIn = vld1q(pInVec); 
        vecOut = vecIn * scale;

        vstrwq_p(pOut, vecOut, p0);
    }
    /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);

}
#else
#if defined(ARM_MATH_NEON_EXPERIMENTAL)
arm_status arm_mat_scale_f32(
  const arm_matrix_instance_f32 * pSrc,
  float32_t scale,
  arm_matrix_instance_f32 * pDst)
{
  float32_t *pIn = pSrc->pData;                  /* input data matrix pointer */
  float32_t *pOut = pDst->pData;                 /* output data matrix pointer */
  uint32_t numSamples;                           /* total number of elements in the matrix */
  uint32_t blkCnt;                               /* loop counters */
  arm_status status;                             /* status of matrix scaling     */


  float32_t in1, in2, in3, in4;                  /* temporary variables */
  float32_t out1, out2, out3, out4;              /* temporary variables */


#ifdef ARM_MATH_MATRIX_CHECK
  /* Check for matrix mismatch condition */
  if ((pSrc->numRows != pDst->numRows) || (pSrc->numCols != pDst->numCols))
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else
#endif /*    #ifdef ARM_MATH_MATRIX_CHECK    */
  {
    float32x4_t vec1;
    float32x4_t res;

    /* Total number of samples in the input matrix */
    numSamples = (uint32_t) pSrc->numRows * pSrc->numCols;

    blkCnt = numSamples >> 2;

    /* Compute 4 outputs at a time.
     ** a second loop below computes the remaining 1 to 3 samples. */
    while (blkCnt > 0U)
    {
      /* C(m,n) = A(m,n) * scale */
      /* Scaling and results are stored in the destination buffer. */
      vec1 = vld1q_f32(pIn);
      res = vmulq_f32(vec1, vdupq_n_f32(scale));
      vst1q_f32(pOut, res);

      /* update pointers to process next sampels */
      pIn += 4U;
      pOut += 4U;

      /* Decrement the numSamples loop counter */
      blkCnt--;
    }

    /* If the numSamples is not a multiple of 4, compute any remaining output samples here.
     ** No loop unrolling is used. */
    blkCnt = numSamples % 0x4U;

    while (blkCnt > 0U)
    {
      /* C(m,n) = A(m,n) * scale */
      /* The results are stored in the destination buffer. */
      *pOut++ = (*pIn++) * scale;

      /* Decrement the loop counter */
      blkCnt--;
    }

    /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}
#else
arm_status arm_mat_scale_f32(
  const arm_matrix_instance_f32 * pSrc,
        float32_t                 scale,
        arm_matrix_instance_f32 * pDst)
{
  float32_t *pIn = pSrc->pData;                  /* Input data matrix pointer */
  float32_t *pOut = pDst->pData;                 /* Output data matrix pointer */
  uint32_t numSamples;                           /* Total number of elements in the matrix */
  uint32_t blkCnt;                               /* Loop counters */
  arm_status status;                             /* Status of matrix scaling */

#ifdef ARM_MATH_MATRIX_CHECK

  /* Check for matrix mismatch condition */
  if ((pSrc->numRows != pDst->numRows) ||
      (pSrc->numCols != pDst->numCols)   )
  {
    /* Set status as ARM_MATH_SIZE_MISMATCH */
    status = ARM_MATH_SIZE_MISMATCH;
  }
  else

#endif /* #ifdef ARM_MATH_MATRIX_CHECK */

  {
    /* Total number of samples in input matrix */
    numSamples = (uint32_t) pSrc->numRows * pSrc->numCols;

#if defined (ARM_MATH_LOOPUNROLL)

    /* Loop unrolling: Compute 4 outputs at a time */
    blkCnt = numSamples >> 2U;

    while (blkCnt > 0U)
    {
      /* C(m,n) = A(m,n) * scale */

      /* Scale and store result in destination buffer. */
      *pOut++ = (*pIn++) * scale;
      *pOut++ = (*pIn++) * scale;
      *pOut++ = (*pIn++) * scale;
      *pOut++ = (*pIn++) * scale;

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
      /* C(m,n) = A(m,n) * scale */

      /* Scale and store result in destination buffer. */
      *pOut++ = (*pIn++) * scale;

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
  @} end of MatrixScale group
 */
