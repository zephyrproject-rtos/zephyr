/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_scale_q7.c
 * Description:  Multiplies a Q7 vector by a scalar
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
  @addtogroup BasicScale
  @{
 */

/**
  @brief         Multiplies a Q7 vector by a scalar.
  @param[in]     pSrc       points to the input vector
  @param[in]     scaleFract fractional portion of the scale value
  @param[in]     shift      number of bits to shift the result by
  @param[out]    pDst       points to the output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The input data <code>*pSrc</code> and <code>scaleFract</code> are in 1.7 format.
                   These are multiplied to yield a 2.14 intermediate result and this is shifted with saturation to 1.7 format.
 */

#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"


void arm_scale_q7(
    const q7_t * pSrc,
    q7_t   scaleFract,
    int8_t  shift,
    q7_t * pDst,
    uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q7x16_t vecSrc;
    q7x16_t vecDst;


    /* Compute 16 outputs at a time */
    blkCnt = blockSize >> 4;

    while (blkCnt > 0U)
    {
        /*
         * C = A * scale
         * Scale the input and then store the result in the destination buffer.
         */
        vecSrc = vld1q(pSrc);
        vecDst = vmulhq(vecSrc, vdupq_n_s8(scaleFract));
        vecDst = vqshlq_r(vecDst, shift + 1);
        vst1q(pDst, vecDst);
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
        /*
         * advance vector source and destination pointers
         */
        pSrc += 16;
        pDst += 16;
    }
    /*
     * tail
     */
    blkCnt = blockSize & 0xF;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp8q(blkCnt);
        vecSrc = vld1q(pSrc);
        vecDst = vmulhq(vecSrc, vdupq_n_s8(scaleFract));
        vecDst = vqshlq_r(vecDst, shift + 1);
        vstrbq_p(pDst, vecDst, p0);
    }

}

#else
void arm_scale_q7(
  const q7_t * pSrc,
        q7_t scaleFract,
        int8_t shift,
        q7_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */
        int8_t kShift = 7 - shift;                     /* Shift to apply after scaling */

#if defined (ARM_MATH_LOOPUNROLL)

#if defined (ARM_MATH_DSP)
  q7_t in1,  in2,  in3,  in4;                    /* Temporary input variables */
  q7_t out1, out2, out3, out4;                   /* Temporary output variables */
#endif

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A * scale */

#if defined (ARM_MATH_DSP)
    /* Reading 4 inputs from memory */
    in1 = *pSrc++;
    in2 = *pSrc++;
    in3 = *pSrc++;
    in4 = *pSrc++;

    /* Scale inputs and store result in the temporary variable. */
    out1 = (q7_t) (__SSAT(((in1) * scaleFract) >> kShift, 8));
    out2 = (q7_t) (__SSAT(((in2) * scaleFract) >> kShift, 8));
    out3 = (q7_t) (__SSAT(((in3) * scaleFract) >> kShift, 8));
    out4 = (q7_t) (__SSAT(((in4) * scaleFract) >> kShift, 8));

    /* Pack and store result in destination buffer (in single write) */
    write_q7x4_ia (&pDst, __PACKq7(out1, out2, out3, out4));
#else
    *pDst++ = (q7_t) (__SSAT((((q15_t) *pSrc++ * scaleFract) >> kShift), 8));
    *pDst++ = (q7_t) (__SSAT((((q15_t) *pSrc++ * scaleFract) >> kShift), 8));
    *pDst++ = (q7_t) (__SSAT((((q15_t) *pSrc++ * scaleFract) >> kShift), 8));
    *pDst++ = (q7_t) (__SSAT((((q15_t) *pSrc++ * scaleFract) >> kShift), 8));
#endif

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = blockSize % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* C = A * scale */

    /* Scale input and store result in destination buffer. */
    *pDst++ = (q7_t) (__SSAT((((q15_t) *pSrc++ * scaleFract) >> kShift), 8));

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of BasicScale group
 */
