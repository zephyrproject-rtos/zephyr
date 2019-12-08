/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_correlate_opt_q15.c
 * Description:  Correlation of Q15 sequences
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
  @ingroup groupFilters
 */

/**
  @addtogroup Corr
  @{
 */

/**
  @brief         Correlation of Q15 sequences.
  @param[in]     pSrcA      points to the first input sequence
  @param[in]     srcALen    length of the first input sequence
  @param[in]     pSrcB      points to the second input sequence
  @param[in]     srcBLen    length of the second input sequence
  @param[out]    pDst       points to the location where the output result is written.  Length 2 * max(srcALen, srcBLen) - 1.
  @param[in]     pScratch   points to scratch buffer of size max(srcALen, srcBLen) + 2*min(srcALen, srcBLen) - 2.
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using a 64-bit internal accumulator.
                   Both inputs are in 1.15 format and multiplications yield a 2.30 result.
                   The 2.30 intermediate results are accumulated in a 64-bit accumulator in 34.30 format.
                   This approach provides 33 guard bits and there is no risk of overflow.
                   The 34.30 result is then truncated to 34.15 format by discarding the low 15 bits and then saturated to 1.15 format.

 @remark
                   Refer to \ref arm_correlate_fast_q15() for a faster but less precise version of this function.
 */

void arm_correlate_opt_q15(
  const q15_t * pSrcA,
        uint32_t srcALen,
  const q15_t * pSrcB,
        uint32_t srcBLen,
        q15_t * pDst,
        q15_t * pScratch)
{
        q63_t acc0;                                    /* Accumulators */
        q15_t *pOut = pDst;                            /* Output pointer */
        q15_t *pScr1;                                  /* Temporary pointer for scratch1 */
  const q15_t *pIn1;                                   /* InputA pointer */
  const q15_t *pIn2;                                   /* InputB pointer */
  const q15_t *py;                                     /* Intermediate inputB pointer */
        uint32_t j, blkCnt, outBlockSize;              /* Loop counter */
        int32_t inc = 1;                               /* Output pointer increment */
        uint32_t tapCnt;

#if defined (ARM_MATH_LOOPUNROLL)
        q63_t acc1, acc2, acc3;                        /* Accumulators */
        q31_t x1, x2, x3;                              /* Temporary variables for holding input1 and input2 values */
        q31_t y1, y2;                                  /* State variables */
#endif

  /* The algorithm implementation is based on the lengths of the inputs. */
  /* srcB is always made to slide across srcA. */
  /* So srcBLen is always considered as shorter or equal to srcALen */
  /* But CORR(x, y) is reverse of CORR(y, x) */
  /* So, when srcBLen > srcALen, output pointer is made to point to the end of the output buffer */
  /* and the destination pointer modifier, inc is set to -1 */
  /* If srcALen > srcBLen, zero pad has to be done to srcB to make the two inputs of same length */
  /* But to improve the performance,
   * we include zeroes in the output instead of zero padding either of the the inputs*/
  /* If srcALen > srcBLen,
   * (srcALen - srcBLen) zeroes has to included in the starting of the output buffer */
  /* If srcALen < srcBLen,
   * (srcALen - srcBLen) zeroes has to included in the ending of the output buffer */
  if (srcALen >= srcBLen)
  {
    /* Initialization of inputA pointer */
    pIn1 = pSrcA;

    /* Initialization of inputB pointer */
    pIn2 = pSrcB;

    /* Number of output samples is calculated */
    outBlockSize = (srcALen * 2U) - 1U;

    /* When srcALen > srcBLen, zero padding is done to srcB
     * to make their lengths equal.
     * Instead, (outBlockSize - (srcALen + srcBLen - 1))
     * number of output samples are made zero */
    j = outBlockSize - (srcALen + (srcBLen - 1U));

    /* Updating the pointer position to non zero value */
    pOut += j;
  }
  else
  {
    /* Initialization of inputA pointer */
    pIn1 = pSrcB;

    /* Initialization of inputB pointer */
    pIn2 = pSrcA;

    /* srcBLen is always considered as shorter or equal to srcALen */
    j = srcBLen;
    srcBLen = srcALen;
    srcALen = j;

    /* CORR(x, y) = Reverse order(CORR(y, x)) */
    /* Hence set the destination pointer to point to the last output sample */
    pOut = pDst + ((srcALen + srcBLen) - 2U);

    /* Destination address modifier is set to -1 */
    inc = -1;
  }

  pScr1 = pScratch;

  /* Fill (srcBLen - 1U) zeros in scratch buffer */
  arm_fill_q15(0, pScr1, (srcBLen - 1U));

  /* Update temporary scratch pointer */
  pScr1 += (srcBLen - 1U);

  /* Copy (srcALen) samples in scratch buffer */
  arm_copy_q15(pIn1, pScr1, srcALen);

  /* Update pointers */
  pScr1 += srcALen;


  /* Fill (srcBLen - 1U) zeros at end of scratch buffer */
  arm_fill_q15(0, pScr1, (srcBLen - 1U));

  /* Update pointer */
  pScr1 += (srcBLen - 1U);

  /* Temporary pointer for scratch2 */
  py = pIn2;


  /* Actual correlation process starts here */
#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = (srcALen + srcBLen - 1U) >> 2;

  while (blkCnt > 0)
  {
    /* Initialze temporary scratch pointer as scratch1 */
    pScr1 = pScratch;

    /* Clear Accumlators */
    acc0 = 0;
    acc1 = 0;
    acc2 = 0;
    acc3 = 0;

    /* Read two samples from scratch1 buffer */
    x1 = read_q15x2_ia (&pScr1);

    /* Read next two samples from scratch1 buffer */
    x2 = read_q15x2_ia (&pScr1);

    tapCnt = (srcBLen) >> 2U;

    while (tapCnt > 0U)
    {
      /* Read four samples from smaller buffer */
      y1 = read_q15x2_ia ((q15_t **) &pIn2);
      y2 = read_q15x2_ia ((q15_t **) &pIn2);

      /* multiply and accumlate */
      acc0 = __SMLALD(x1, y1, acc0);
      acc2 = __SMLALD(x2, y1, acc2);

      /* pack input data */
#ifndef ARM_MATH_BIG_ENDIAN
      x3 = __PKHBT(x2, x1, 0);
#else
      x3 = __PKHBT(x1, x2, 0);
#endif

      /* multiply and accumlate */
      acc1 = __SMLALDX(x3, y1, acc1);

      /* Read next two samples from scratch1 buffer */
      x1 = read_q15x2_ia (&pScr1);

      /* multiply and accumlate */
      acc0 = __SMLALD(x2, y2, acc0);
      acc2 = __SMLALD(x1, y2, acc2);

      /* pack input data */
#ifndef ARM_MATH_BIG_ENDIAN
      x3 = __PKHBT(x1, x2, 0);
#else
      x3 = __PKHBT(x2, x1, 0);
#endif

      acc3 = __SMLALDX(x3, y1, acc3);
      acc1 = __SMLALDX(x3, y2, acc1);

      x2 = read_q15x2_ia (&pScr1);

#ifndef ARM_MATH_BIG_ENDIAN
      x3 = __PKHBT(x2, x1, 0);
#else
      x3 = __PKHBT(x1, x2, 0);
#endif

      acc3 = __SMLALDX(x3, y2, acc3);

      /* Decrement loop counter */
      tapCnt--;
    }

    /* Update scratch pointer for remaining samples of smaller length sequence */
    pScr1 -= 4U;

    /* apply same above for remaining samples of smaller length sequence */
    tapCnt = (srcBLen) & 3U;

    while (tapCnt > 0U)
    {
      /* accumlate the results */
      acc0 += (*pScr1++ * *pIn2);
      acc1 += (*pScr1++ * *pIn2);
      acc2 += (*pScr1++ * *pIn2);
      acc3 += (*pScr1++ * *pIn2++);

      pScr1 -= 3U;

      /* Decrement loop counter */
      tapCnt--;
    }

    blkCnt--;


    /* Store the results in the accumulators in the destination buffer. */
    *pOut = (__SSAT(acc0 >> 15U, 16));
    pOut += inc;
    *pOut = (__SSAT(acc1 >> 15U, 16));
    pOut += inc;
    *pOut = (__SSAT(acc2 >> 15U, 16));
    pOut += inc;
    *pOut = (__SSAT(acc3 >> 15U, 16));
    pOut += inc;

    /* Initialization of inputB pointer */
    pIn2 = py;

    pScratch += 4U;
  }


  /* Loop unrolling: Compute remaining outputs */
  blkCnt = (srcALen + srcBLen - 1U) & 0x3;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = (srcALen + srcBLen - 1U);

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  /* Calculate correlation for remaining samples of Bigger length sequence */
  while (blkCnt > 0)
  {
    /* Initialze temporary scratch pointer as scratch1 */
    pScr1 = pScratch;

    /* Clear Accumlators */
    acc0 = 0;

    tapCnt = (srcBLen) >> 1U;

    while (tapCnt > 0U)
    {

      /* Read next two samples from scratch1 buffer */
      acc0 += (*pScr1++ * *pIn2++);
      acc0 += (*pScr1++ * *pIn2++);

      /* Decrement loop counter */
      tapCnt--;
    }

    tapCnt = (srcBLen) & 1U;

    /* apply same above for remaining samples of smaller length sequence */
    while (tapCnt > 0U)
    {
      /* accumlate the results */
      acc0 += (*pScr1++ * *pIn2++);

      /* Decrement loop counter */
      tapCnt--;
    }

    blkCnt--;

    /* The result is in 2.30 format.  Convert to 1.15 with saturation.
       Then store the output in the destination buffer. */
    *pOut = (q15_t) (__SSAT((acc0 >> 15), 16));
    pOut += inc;

    /* Initialization of inputB pointer */
    pIn2 = py;

    pScratch += 1U;
  }

}

/**
  @} end of Corr group
 */
