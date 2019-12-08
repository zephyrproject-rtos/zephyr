/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mult_q15.c
 * Description:  Q15 vector multiplication
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
  @addtogroup BasicMult
  @{
 */

/**
  @brief         Q15 vector multiplication
  @param[in]     pSrcA      points to first input vector
  @param[in]     pSrcB      points to second input vector
  @param[out]    pDst       points to output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   Results outside of the allowable Q15 range [0x8000 0x7FFF] are saturated.
 */
#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

void arm_mult_q15(
    const q15_t * pSrcA,
    const q15_t * pSrcB,
    q15_t * pDst,
    uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q15x8_t vecA, vecB;

    /* Compute 8 outputs at a time */
    blkCnt = blockSize >> 3;
    while (blkCnt > 0U)
    {
        /*
         * C = A * B
         * Multiply the inputs and then store the results in the destination buffer.
         */
        vecA = vld1q(pSrcA);
        vecB = vld1q(pSrcB);
        vst1q(pDst, vqdmulhq(vecA, vecB));
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
        /*
         * advance vector source and destination pointers
         */
        pSrcA  += 8;
        pSrcB  += 8;
        pDst   += 8;
    }
    /*
     * tail
     */
    blkCnt = blockSize & 7;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp16q(blkCnt);
        vecA = vld1q(pSrcA);
        vecB = vld1q(pSrcB);
        vstrhq_p(pDst, vqdmulhq(vecA, vecB), p0);
    }
}

#else
void arm_mult_q15(
  const q15_t * pSrcA,
  const q15_t * pSrcB,
        q15_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */

#if defined (ARM_MATH_LOOPUNROLL)

#if defined (ARM_MATH_DSP)
  q31_t inA1, inA2, inB1, inB2;                  /* Temporary input variables */
  q15_t out1, out2, out3, out4;                  /* Temporary output variables */
  q31_t mul1, mul2, mul3, mul4;                  /* Temporary variables */
#endif

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A * B */

#if defined (ARM_MATH_DSP)
    /* read 2 samples at a time from sourceA */
    inA1 = read_q15x2_ia ((q15_t **) &pSrcA);
    /* read 2 samples at a time from sourceB */
    inB1 = read_q15x2_ia ((q15_t **) &pSrcB);
    /* read 2 samples at a time from sourceA */
    inA2 = read_q15x2_ia ((q15_t **) &pSrcA);
    /* read 2 samples at a time from sourceB */
    inB2 = read_q15x2_ia ((q15_t **) &pSrcB);

    /* multiply mul = sourceA * sourceB */
    mul1 = (q31_t) ((q15_t) (inA1 >> 16) * (q15_t) (inB1 >> 16));
    mul2 = (q31_t) ((q15_t) (inA1      ) * (q15_t) (inB1      ));
    mul3 = (q31_t) ((q15_t) (inA2 >> 16) * (q15_t) (inB2 >> 16));
    mul4 = (q31_t) ((q15_t) (inA2      ) * (q15_t) (inB2      ));

    /* saturate result to 16 bit */
    out1 = (q15_t) __SSAT(mul1 >> 15, 16);
    out2 = (q15_t) __SSAT(mul2 >> 15, 16);
    out3 = (q15_t) __SSAT(mul3 >> 15, 16);
    out4 = (q15_t) __SSAT(mul4 >> 15, 16);

    /* store result to destination */
#ifndef ARM_MATH_BIG_ENDIAN
    write_q15x2_ia (&pDst, __PKHBT(out2, out1, 16));
    write_q15x2_ia (&pDst, __PKHBT(out4, out3, 16));
#else
    write_q15x2_ia (&pDst, __PKHBT(out1, out2, 16));
    write_q15x2_ia (&pDst, __PKHBT(out3, out4, 16));
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

#else
    *pDst++ = (q15_t) __SSAT((((q31_t) (*pSrcA++) * (*pSrcB++)) >> 15), 16);
    *pDst++ = (q15_t) __SSAT((((q31_t) (*pSrcA++) * (*pSrcB++)) >> 15), 16);
    *pDst++ = (q15_t) __SSAT((((q31_t) (*pSrcA++) * (*pSrcB++)) >> 15), 16);
    *pDst++ = (q15_t) __SSAT((((q31_t) (*pSrcA++) * (*pSrcB++)) >> 15), 16);
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
    /* C = A * B */

    /* Multiply inputs and store result in destination buffer. */
    *pDst++ = (q15_t) __SSAT((((q31_t) (*pSrcA++) * (*pSrcB++)) >> 15), 16);

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of BasicMult group
 */
