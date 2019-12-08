/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_scale_f32.c
 * Description:  Multiplies a floating-point vector by a scalar
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
  @ingroup groupMath
 */

/**
  @defgroup BasicScale Vector Scale

  Multiply a vector by a scalar value.  For floating-point data, the algorithm used is:

  <pre>
      pDst[n] = pSrc[n] * scale,   0 <= n < blockSize.
  </pre>

  In the fixed-point Q7, Q15, and Q31 functions, <code>scale</code> is represented by
  a fractional multiplication <code>scaleFract</code> and an arithmetic shift <code>shift</code>.
  The shift allows the gain of the scaling operation to exceed 1.0.
  The algorithm used with fixed-point data is:

  <pre>
      pDst[n] = (pSrc[n] * scaleFract) << shift,   0 <= n < blockSize.
  </pre>

  The overall scale factor applied to the fixed-point data is
  <pre>
      scale = scaleFract * 2^shift.
  </pre>

  The functions support in-place computation allowing the source and destination
  pointers to reference the same memory buffer.
 */

/**
  @addtogroup BasicScale
  @{
 */

/**
  @brief         Multiplies a floating-point vector by a scalar.
  @param[in]     pSrc       points to the input vector
  @param[in]     scale      scale factor to be applied
  @param[out]    pDst       points to the output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"

void arm_scale_f32(
  const float32_t * pSrc,
        float32_t scale,
        float32_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */

    f32x4_t vec1;
    f32x4_t res;

    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2U;

    while (blkCnt > 0U)
    {
        /* C = A + offset */
 
        /* Add offset and then store the results in the destination buffer. */
        vec1 = vld1q(pSrc);
        res = vmulq(vec1,scale);
        vst1q(pDst, res);

        /* Increment pointers */
        pSrc += 4;
        pDst += 4;
        
        /* Decrement the loop counter */
        blkCnt--;
    }

    /* Tail */
    blkCnt = blockSize & 0x3;

    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp32q(blkCnt);
        vec1 = vld1q((float32_t const *) pSrc);
        vstrwq_p(pDst, vmulq(vec1, scale), p0);
    }


}

#else
void arm_scale_f32(
  const float32_t *pSrc,
        float32_t scale,
        float32_t *pDst,
        uint32_t blockSize)
{
  uint32_t blkCnt;                               /* Loop counter */
#if defined(ARM_MATH_NEON_EXPERIMENTAL)
    f32x4_t vec1;
    f32x4_t res;

    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2U;

    while (blkCnt > 0U)
    {
        /* C = A * scale */

    	/* Scale the input and then store the results in the destination buffer. */
        vec1 = vld1q_f32(pSrc);
        res = vmulq_f32(vec1, vdupq_n_f32(scale));
        vst1q_f32(pDst, res);

        /* Increment pointers */
        pSrc += 4; 
        pDst += 4;
        
        /* Decrement the loop counter */
        blkCnt--;
    }

    /* Tail */
    blkCnt = blockSize & 0x3;

#else
#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A * scale */

    /* Scale input and store result in destination buffer. */
    *pDst++ = (*pSrc++) * scale;

    *pDst++ = (*pSrc++) * scale;

    *pDst++ = (*pSrc++) * scale;

    *pDst++ = (*pSrc++) * scale;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = blockSize % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */
#endif /* #if defined(ARM_MATH_NEON_EXPERIMENTAL) */

  while (blkCnt > 0U)
  {
    /* C = A * scale */

    /* Scale input and store result in destination buffer. */
    *pDst++ = (*pSrc++) * scale;

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of BasicScale group
 */
