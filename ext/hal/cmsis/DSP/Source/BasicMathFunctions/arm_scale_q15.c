/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_scale_q15.c
 * Description:  Multiplies a Q15 vector by a scalar
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
  @brief         Multiplies a Q15 vector by a scalar.
  @param[in]     pSrc       points to the input vector
  @param[in]     scaleFract fractional portion of the scale value
  @param[in]     shift      number of bits to shift the result by
  @param[out]    pDst       points to the output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The input data <code>*pSrc</code> and <code>scaleFract</code> are in 1.15 format.
                   These are multiplied to yield a 2.30 intermediate result and this is shifted with saturation to 1.15 format.
 */

#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

void arm_scale_q15(
    const q15_t * pSrc,
    q15_t   scaleFract,
    int8_t  shift,
    q15_t * pDst,
    uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q15x8_t vecSrc;
    q15x8_t vecDst;


    /* Compute 8 outputs at a time */
    blkCnt = blockSize >> 3;

    while (blkCnt > 0U)
    {
        /*
         * C = A * scale
         * Scale the input and then store the result in the destination buffer.
         */
        vecSrc = vld1q(pSrc);
        vecDst = vmulhq(vecSrc, vdupq_n_s16(scaleFract));
        vecDst = vqshlq_r(vecDst, shift + 1);
        vst1q(pDst, vecDst);
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
        /*
         * advance vector source and destination pointers
         */
        pSrc += 8;
        pDst += 8;
    }
    /*
     * tail
     */
    blkCnt = blockSize & 7;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp16q(blkCnt);;
        vecSrc = vld1q(pSrc);
        vecDst = vmulhq(vecSrc, vdupq_n_s16(scaleFract));
        vecDst = vqshlq_r(vecDst, shift + 1);
        vstrhq_p(pDst, vecDst, p0);
    }

}


#else
void arm_scale_q15(
  const q15_t *pSrc,
        q15_t scaleFract,
        int8_t shift,
        q15_t *pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */
        int8_t kShift = 15 - shift;                    /* Shift to apply after scaling */

#if defined (ARM_MATH_LOOPUNROLL)
#if defined (ARM_MATH_DSP)
  q31_t inA1, inA2;
  q31_t out1, out2, out3, out4;                  /* Temporary output variables */
  q15_t in1, in2, in3, in4;                      /* Temporary input variables */
#endif
#endif

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A * scale */

#if defined (ARM_MATH_DSP)
    /* read 2 times 2 samples at a time from source */
    inA1 = read_q15x2_ia ((q15_t **) &pSrc);
    inA2 = read_q15x2_ia ((q15_t **) &pSrc);

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
    write_q15x2_ia (&pDst, __PKHBT(in2, in1, 16));
    write_q15x2_ia (&pDst, __PKHBT(in4, in3, 16));
#else
    *pDst++ = (q15_t) (__SSAT(((q31_t) *pSrc++ * scaleFract) >> kShift, 16));
    *pDst++ = (q15_t) (__SSAT(((q31_t) *pSrc++ * scaleFract) >> kShift, 16));
    *pDst++ = (q15_t) (__SSAT(((q31_t) *pSrc++ * scaleFract) >> kShift, 16));
    *pDst++ = (q15_t) (__SSAT(((q31_t) *pSrc++ * scaleFract) >> kShift, 16));
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
    *pDst++ = (q15_t) (__SSAT(((q31_t) *pSrc++ * scaleFract) >> kShift, 16));

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of BasicScale group
 */
