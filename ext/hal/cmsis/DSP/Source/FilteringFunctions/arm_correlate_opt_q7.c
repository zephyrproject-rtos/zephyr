/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_correlate_opt_q7.c
 * Description:  Correlation of Q7 sequences
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
  @brief         Correlation of Q7 sequences.
  @param[in]     pSrcA      points to the first input sequence
  @param[in]     srcALen    length of the first input sequence
  @param[in]     pSrcB      points to the second input sequence
  @param[in]     srcBLen    length of the second input sequence
  @param[out]    pDst       points to the location where the output result is written.  Length 2 * max(srcALen, srcBLen) - 1.
  @param[in]     pScratch1  points to scratch buffer(of type q15_t) of size max(srcALen, srcBLen) + 2*min(srcALen, srcBLen) - 2.
  @param[in]     pScratch2  points to scratch buffer (of type q15_t) of size min(srcALen, srcBLen).
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using a 32-bit internal accumulator.
                   Both the inputs are represented in 1.7 format and multiplications yield a 2.14 result.
                   The 2.14 intermediate results are accumulated in a 32-bit accumulator in 18.14 format.
                   This approach provides 17 guard bits and there is no risk of overflow as long as <code>max(srcALen, srcBLen)<131072</code>.
                   The 18.14 result is then truncated to 18.7 format by discarding the low 7 bits and then saturated to 1.7 format.
 */

void arm_correlate_opt_q7(
  const q7_t * pSrcA,
        uint32_t srcALen,
  const q7_t * pSrcB,
        uint32_t srcBLen,
        q7_t * pDst,
        q15_t * pScratch1,
        q15_t * pScratch2)
{
        q15_t *pScr1 = pScratch1;                      /* Temporary pointer for scratch */
        q15_t *pScr2 = pScratch2;                      /* Temporary pointer for scratch */
        q15_t x4;                                      /* Temporary input variable */
        q15_t *py;                                     /* Temporary input2 pointer */
        q31_t acc0, acc1, acc2, acc3;                  /* Accumulators */
  const q7_t *pIn1, *pIn2;                             /* InputA and inputB pointer */
        uint32_t j, k, blkCnt, tapCnt;                 /* Loop counter */
        int32_t inc = 1;                               /* Output pointer increment */
        uint32_t outBlockSize;                         /* Loop counter */
        q31_t x1, x2, x3, y1;                          /* Temporary input variables */
        q7_t *pOut = pDst;                             /* Output pointer */

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


  /* Copy (srcBLen) samples in scratch buffer */
  k = srcBLen >> 2U;

  /* First part of the processing with loop unrolling copies 4 data points at a time.
     a second loop below copies for the remaining 1 to 3 samples. */
  while (k > 0U)
  {
    /* copy second buffer in reversal manner */
    x4 = (q15_t) *pIn2++;
    *pScr2++ = x4;
    x4 = (q15_t) *pIn2++;
    *pScr2++ = x4;
    x4 = (q15_t) *pIn2++;
    *pScr2++ = x4;
    x4 = (q15_t) *pIn2++;
    *pScr2++ = x4;

    /* Decrement loop counter */
    k--;
  }

  /* If the count is not a multiple of 4, copy remaining samples here.
     No loop unrolling is used. */
  k = srcBLen % 0x4U;

  while (k > 0U)
  {
    /* copy second buffer in reversal manner for remaining samples */
    x4 = (q15_t) *pIn2++;
    *pScr2++ = x4;

    /* Decrement loop counter */
    k--;
  }

  /* Fill (srcBLen - 1U) zeros in scratch buffer */
  arm_fill_q15(0, pScr1, (srcBLen - 1U));

  /* Update temporary scratch pointer */
  pScr1 += (srcBLen - 1U);

  /* Copy (srcALen) samples in scratch buffer */
  /* Apply loop unrolling and do 4 Copies simultaneously. */
  k = srcALen >> 2U;

  /* First part of the processing with loop unrolling copies 4 data points at a time.
     a second loop below copies for the remaining 1 to 3 samples. */
  while (k > 0U)
  {
    /* copy second buffer in reversal manner */
    x4 = (q15_t) *pIn1++;
    *pScr1++ = x4;
    x4 = (q15_t) *pIn1++;
    *pScr1++ = x4;
    x4 = (q15_t) *pIn1++;
    *pScr1++ = x4;
    x4 = (q15_t) *pIn1++;
    *pScr1++ = x4;

    /* Decrement loop counter */
    k--;
  }

  /* If the count is not a multiple of 4, copy remaining samples here.
     No loop unrolling is used. */
  k = srcALen % 0x4U;

  while (k > 0U)
  {
    /* copy second buffer in reversal manner for remaining samples */
    x4 = (q15_t) * pIn1++;
    *pScr1++ = x4;

    /* Decrement the loop counter */
    k--;
  }

  /* Fill (srcBLen - 1U) zeros at end of scratch buffer */
  arm_fill_q15(0, pScr1, (srcBLen - 1U));

  /* Update pointer */
  pScr1 += (srcBLen - 1U);

  /* Temporary pointer for scratch2 */
  py = pScratch2;

  /* Initialization of pScr2 pointer */
  pScr2 = pScratch2;

  /* Actual correlation process starts here */
  blkCnt = (srcALen + srcBLen - 1U) >> 2;

  while (blkCnt > 0)
  {
    /* Initialze temporary scratch pointer as scratch1 */
    pScr1 = pScratch1;

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
      y1 = read_q15x2_ia (&pScr2);

      /* multiply and accumlate */
      acc0 = __SMLAD(x1, y1, acc0);
      acc2 = __SMLAD(x2, y1, acc2);

      /* pack input data */
#ifndef ARM_MATH_BIG_ENDIAN
      x3 = __PKHBT(x2, x1, 0);
#else
      x3 = __PKHBT(x1, x2, 0);
#endif

      /* multiply and accumlate */
      acc1 = __SMLADX(x3, y1, acc1);

      /* Read next two samples from scratch1 buffer */
      x1 = read_q15x2_ia (&pScr1);

      /* pack input data */
#ifndef ARM_MATH_BIG_ENDIAN
      x3 = __PKHBT(x1, x2, 0);
#else
      x3 = __PKHBT(x2, x1, 0);
#endif

      acc3 = __SMLADX(x3, y1, acc3);

      /* Read four samples from smaller buffer */
      y1 = read_q15x2_ia (&pScr2);

      acc0 = __SMLAD(x2, y1, acc0);

      acc2 = __SMLAD(x1, y1, acc2);

      acc1 = __SMLADX(x3, y1, acc1);

      x2 = read_q15x2_ia (&pScr1);

#ifndef ARM_MATH_BIG_ENDIAN
      x3 = __PKHBT(x2, x1, 0);
#else
      x3 = __PKHBT(x1, x2, 0);
#endif

      acc3 = __SMLADX(x3, y1, acc3);

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
      acc0 += (*pScr1++ * *pScr2);
      acc1 += (*pScr1++ * *pScr2);
      acc2 += (*pScr1++ * *pScr2);
      acc3 += (*pScr1++ * *pScr2++);

      pScr1 -= 3U;

      /* Decrement loop counter */
      tapCnt--;
    }

    blkCnt--;

    /* Store the result in the accumulator in the destination buffer. */
    *pOut = (q7_t) (__SSAT(acc0 >> 7U, 8));
    pOut += inc;
    *pOut = (q7_t) (__SSAT(acc1 >> 7U, 8));
    pOut += inc;
    *pOut = (q7_t) (__SSAT(acc2 >> 7U, 8));
    pOut += inc;
    *pOut = (q7_t) (__SSAT(acc3 >> 7U, 8));
    pOut += inc;

    /* Initialization of inputB pointer */
    pScr2 = py;

    pScratch1 += 4U;
  }

  blkCnt = (srcALen + srcBLen - 1U) & 0x3;

  /* Calculate correlation for remaining samples of Bigger length sequence */
  while (blkCnt > 0)
  {
    /* Initialze temporary scratch pointer as scratch1 */
    pScr1 = pScratch1;

    /* Clear Accumlators */
    acc0 = 0;

    tapCnt = (srcBLen) >> 1U;

    while (tapCnt > 0U)
    {
      acc0 += (*pScr1++ * *pScr2++);
      acc0 += (*pScr1++ * *pScr2++);

      /* Decrement loop counter */
      tapCnt--;
    }

    tapCnt = (srcBLen) & 1U;

    /* apply same above for remaining samples of smaller length sequence */
    while (tapCnt > 0U)
    {
      /* accumlate the results */
      acc0 += (*pScr1++ * *pScr2++);

      /* Decrement loop counter */
      tapCnt--;
    }

    blkCnt--;

    /* Store the result in the accumulator in the destination buffer. */
    *pOut = (q7_t) (__SSAT(acc0 >> 7U, 8));
    pOut += inc;

    /* Initialization of inputB pointer */
    pScr2 = py;

    pScratch1 += 1U;
  }

}

/**
  @} end of Corr group
 */
