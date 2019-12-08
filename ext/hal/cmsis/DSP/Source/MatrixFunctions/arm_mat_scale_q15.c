/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mat_scale_q15.c
 * Description:  Multiplies a Q15 matrix by a scalar
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
  @addtogroup MatrixScale
  @{
 */

/**
  @brief         Q15 matrix scaling.
  @param[in]     pSrc        points to input matrix
  @param[in]     scaleFract  fractional portion of the scale factor
  @param[in]     shift       number of bits to shift the result by
  @param[out]    pDst        points to output matrix structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS       : Operation successful
                   - \ref ARM_MATH_SIZE_MISMATCH : Matrix size check failed

  @par           Scaling and Overflow Behavior
                   The input data <code>*pSrc</code> and <code>scaleFract</code> are in 1.15 format.
                   These are multiplied to yield a 2.30 intermediate result and this is shifted with saturation to 1.15 format.
 */
#if defined(ARM_MATH_MVEI)
arm_status arm_mat_scale_q15(
  const arm_matrix_instance_q15 * pSrc,
        q15_t                     scaleFract,
        int32_t                   shift,
        arm_matrix_instance_q15 * pDst)
{
  arm_status status;                             /* Status of matrix scaling */
  q15_t *pIn = pSrc->pData;       /* input data matrix pointer */
  q15_t *pOut = pDst->pData;      /* output data matrix pointer */
  uint32_t  numSamples;           /* total number of elements in the matrix */
  uint32_t  blkCnt;               /* loop counters */
  q15x8_t vecIn, vecOut;
  q15_t const *pInVec;
  int32_t totShift = shift + 1;   /* shift to apply after scaling */
  
  pInVec = (q15_t const *) pIn;

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
    /*
     * Total number of samples in the input matrix
     */
    numSamples = (uint32_t) pSrc->numRows * pSrc->numCols;
    blkCnt = numSamples >> 3;
    while (blkCnt > 0U)
    {
        /*
         * C(m,n) = A(m,n) * scale
         * Scaling and results are stored in the destination buffer.
         */
        vecIn = vld1q(pInVec); pInVec += 8;

        /* multiply input with scaler value */
        vecOut = vmulhq(vecIn, vdupq_n_s16(scaleFract));
         /* apply shifting */
        vecOut = vqshlq_r(vecOut, totShift);

        vst1q(pOut, vecOut); pOut += 8;

        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
    }
    /*
     * tail
     * (will be merged thru tail predication)
     */
    blkCnt = numSamples & 7;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp16q(blkCnt);
        vecIn = vld1q(pInVec); pInVec += 8;
        vecOut = vmulhq(vecIn, vdupq_n_s16(scaleFract));
        vecOut = vqshlq_r(vecOut, totShift);
        vstrhq_p(pOut, vecOut, p0);
    }
     /* Set status as ARM_MATH_SUCCESS */
    status = ARM_MATH_SUCCESS;
  }

  /* Return to application */
  return (status);
}

#else
arm_status arm_mat_scale_q15(
  const arm_matrix_instance_q15 * pSrc,
        q15_t                     scaleFract,
        int32_t                   shift,
        arm_matrix_instance_q15 * pDst)
{
        q15_t *pIn = pSrc->pData;                      /* Input data matrix pointer */
        q15_t *pOut = pDst->pData;                     /* Output data matrix pointer */
        uint32_t numSamples;                           /* Total number of elements in the matrix */
        uint32_t blkCnt;                               /* Loop counter */
        arm_status status;                             /* Status of matrix scaling */
        int32_t kShift = 15 - shift;                   /* Total shift to apply after scaling */

#if defined (ARM_MATH_LOOPUNROLL) && defined (ARM_MATH_DSP)
        q31_t inA1, inA2;
        q31_t out1, out2, out3, out4;                  /* Temporary output variables */
        q15_t in1, in2, in3, in4;                      /* Temporary input variables */
#endif

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
      /* C(m,n) = A(m,n) * k */

#if defined (ARM_MATH_DSP)
      /* read 2 times 2 samples at a time from source */
      inA1 = read_q15x2_ia ((q15_t **) &pIn);
      inA2 = read_q15x2_ia ((q15_t **) &pIn);

      /* Scale inputs and store result in temporary variables
       * in single cycle by packing the outputs */
      out1 = (q31_t) ((q15_t) (inA1 >> 16) * scaleFract);
      out2 = (q31_t) ((q15_t) (inA1      ) * scaleFract);
      out3 = (q31_t) ((q15_t) (inA2 >> 16) * scaleFract);
      out4 = (q31_t) ((q15_t) (inA2      ) * scaleFract);

      /* apply shifting */
      out1 = out1 >> kShift;
      out2 = out2 >> kShift;
      out3 = out3 >> kShift;
      out4 = out4 >> kShift;

      /* saturate the output */
      in1 = (q15_t) (__SSAT(out1, 16));
      in2 = (q15_t) (__SSAT(out2, 16));
      in3 = (q15_t) (__SSAT(out3, 16));
      in4 = (q15_t) (__SSAT(out4, 16));

      /* store result to destination */
      write_q15x2_ia (&pOut, __PKHBT(in2, in1, 16));
      write_q15x2_ia (&pOut, __PKHBT(in4, in3, 16));

#else
      *pOut++ = (q15_t) (__SSAT(((q31_t) (*pIn++) * scaleFract) >> kShift, 16));
      *pOut++ = (q15_t) (__SSAT(((q31_t) (*pIn++) * scaleFract) >> kShift, 16));
      *pOut++ = (q15_t) (__SSAT(((q31_t) (*pIn++) * scaleFract) >> kShift, 16));
      *pOut++ = (q15_t) (__SSAT(((q31_t) (*pIn++) * scaleFract) >> kShift, 16));
#endif

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
      /* C(m,n) = A(m,n) * k */

      /* Scale, saturate and store result in destination buffer. */
      *pOut++ = (q15_t) (__SSAT(((q31_t) (*pIn++) * scaleFract) >> kShift, 16));

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
  @} end of MatrixScale group
 */
