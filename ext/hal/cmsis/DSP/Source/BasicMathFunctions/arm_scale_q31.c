/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_scale_q31.c
 * Description:  Multiplies a Q31 vector by a scalar
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
  @brief         Multiplies a Q31 vector by a scalar.
  @param[in]     pSrc       points to the input vector
  @param[in]     scaleFract fractional portion of the scale value
  @param[in]     shift      number of bits to shift the result by
  @param[out]    pDst       points to the output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The input data <code>*pSrc</code> and <code>scaleFract</code> are in 1.31 format.
                   These are multiplied to yield a 2.62 intermediate result and this is shifted with saturation to 1.31 format.
 */

#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

void arm_scale_q31(
    const q31_t * pSrc,
    q31_t   scaleFract,
    int8_t  shift,
    q31_t * pDst,
    uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q31x4_t vecSrc;
    q31x4_t vecDst;

    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2;
    while (blkCnt > 0U)
    {
        /*
         * C = A * scale
         * Scale the input and then store the result in the destination buffer.
         */
        vecSrc = vld1q(pSrc);
        vecDst = vmulhq(vecSrc, vdupq_n_s32(scaleFract));
        vecDst = vqshlq_r(vecDst, shift + 1);
        vst1q(pDst, vecDst);
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
        /*
         * advance vector source and destination pointers
         */
        pSrc += 4;
        pDst += 4;
    }
    /*
     * tail
     */
    blkCnt = blockSize & 3;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp32q(blkCnt);
        vecSrc = vld1q(pSrc);
        vecDst = vmulhq(vecSrc, vdupq_n_s32(scaleFract));
        vecDst = vqshlq_r(vecDst, shift + 1);
        vstrwq_p(pDst, vecDst, p0);
    }
}

#else
void arm_scale_q31(
  const q31_t *pSrc,
        q31_t scaleFract,
        int8_t shift,
        q31_t *pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */
        q31_t in, out;                                 /* Temporary variables */
        int8_t kShift = shift + 1;                     /* Shift to apply after scaling */
        int8_t sign = (kShift & 0x80);

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  if (sign == 0U)
  {
    while (blkCnt > 0U)
    {
      /* C = A * scale */

      /* Scale input and store result in destination buffer. */
      in = *pSrc++;                                /* read input from source */
      in = ((q63_t) in * scaleFract) >> 32;        /* multiply input with scaler value */
      out = in << kShift;                          /* apply shifting */
      if (in != (out >> kShift))                   /* saturate the result */
        out = 0x7FFFFFFF ^ (in >> 31);
      *pDst++ = out;                               /* Store result destination */

      in = *pSrc++;
      in = ((q63_t) in * scaleFract) >> 32;
      out = in << kShift;
      if (in != (out >> kShift))
        out = 0x7FFFFFFF ^ (in >> 31);
      *pDst++ = out;

      in = *pSrc++;
      in = ((q63_t) in * scaleFract) >> 32;
      out = in << kShift;
      if (in != (out >> kShift))
        out = 0x7FFFFFFF ^ (in >> 31);
      *pDst++ = out;

      in = *pSrc++;
      in = ((q63_t) in * scaleFract) >> 32;
      out = in << kShift;
      if (in != (out >> kShift))
        out = 0x7FFFFFFF ^ (in >> 31);
      *pDst++ = out;

      /* Decrement loop counter */
      blkCnt--;
    }
  }
  else
  {
    while (blkCnt > 0U)
    {
      /* C = A * scale */

      /* Scale input and store result in destination buffer. */
      in = *pSrc++;                                /* read four inputs from source */
      in = ((q63_t) in * scaleFract) >> 32;        /* multiply input with scaler value */
      out = in >> -kShift;                         /* apply shifting */
      *pDst++ = out;                               /* Store result destination */

      in = *pSrc++;
      in = ((q63_t) in * scaleFract) >> 32;
      out = in >> -kShift;
      *pDst++ = out;

      in = *pSrc++;
      in = ((q63_t) in * scaleFract) >> 32;
      out = in >> -kShift;
      *pDst++ = out;

      in = *pSrc++;
      in = ((q63_t) in * scaleFract) >> 32;
      out = in >> -kShift;
      *pDst++ = out;

      /* Decrement loop counter */
      blkCnt--;
    }
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = blockSize % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  if (sign == 0U)
  {
    while (blkCnt > 0U)
    {
      /* C = A * scale */

      /* Scale input and store result in destination buffer. */
      in = *pSrc++;
      in = ((q63_t) in * scaleFract) >> 32;
      out = in << kShift;
      if (in != (out >> kShift))
          out = 0x7FFFFFFF ^ (in >> 31);
      *pDst++ = out;

      /* Decrement loop counter */
      blkCnt--;
    }
  }
  else
  {
    while (blkCnt > 0U)
    {
      /* C = A * scale */

      /* Scale input and store result in destination buffer. */
      in = *pSrc++;
      in = ((q63_t) in * scaleFract) >> 32;
      out = in >> -kShift;
      *pDst++ = out;

      /* Decrement loop counter */
      blkCnt--;
    }
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of BasicScale group
 */
