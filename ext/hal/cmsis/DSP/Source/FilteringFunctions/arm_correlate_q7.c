/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_correlate_q7.c
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
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using a 32-bit internal accumulator.
                   Both the inputs are represented in 1.7 format and multiplications yield a 2.14 result.
                   The 2.14 intermediate results are accumulated in a 32-bit accumulator in 18.14 format.
                   This approach provides 17 guard bits and there is no risk of overflow as long as <code>max(srcALen, srcBLen)<131072</code>.
                   The 18.14 result is then truncated to 18.7 format by discarding the low 7 bits and saturated to 1.7 format.

 @remark
                   Refer to \ref arm_correlate_opt_q7() for a faster implementation of this function.
 */
#if defined(ARM_MATH_MVEI)
#include "arm_helium_utils.h"

#include "arm_vec_filtering.h"
void arm_correlate_q7(
  const q7_t * pSrcA,
        uint32_t srcALen,
  const q7_t * pSrcB,
        uint32_t srcBLen,
        q7_t * pDst)
{
    const q7_t     *pIn1 = pSrcA;                     /* inputA pointer               */
    const q7_t     *pIn2 = pSrcB + (srcBLen - 1U);    /* inputB pointer               */
    const q7_t     *pX, *pY;
    const q7_t     *pA, *pB;
    int32_t   i = 0U, j = 0;                    /* loop counters */
    int32_t   inv = 1U;                         /* Reverse order flag */
    uint32_t  tot = 0U;                         /* Length */
    int32_t   block1, block2, block3;
    int32_t   incr;

    tot = ((srcALen + srcBLen) - 2U);
    if (srcALen > srcBLen)
    {
        /*
         * Calculating the number of zeros to be padded to the output
         */
        j = srcALen - srcBLen;
        /*
         * Initialize the pointer after zero padding
         */
        pDst += j;
    }
    else if (srcALen < srcBLen)
    {
        /*
         * Initialization to inputB pointer
         */
        pIn1 = pSrcB;
        /*
         * Initialization to the end of inputA pointer
         */
        pIn2 = pSrcA + (srcALen - 1U);
        /*
         * Initialisation of the pointer after zero padding
         */
        pDst = pDst + tot;
        /*
         * Swapping the lengths
         */
        j = srcALen;
        srcALen = srcBLen;
        srcBLen = j;
        /*
         * Setting the reverse flag
         */
        inv = -1;
    }

    block1 = srcBLen - 1;
    block2 = srcALen - srcBLen + 1;
    block3 = srcBLen - 1;

    pA = pIn1;
    pB = pIn2;
    incr = inv;

    for (i = 0U; i <= block1 - 2; i += 2)
    {
        uint32_t  count = i + 1;
        int32_t   acc0 = 0;
        int32_t   acc1 = 0;

        /*
         * compute 2 accumulators per loop
         * size is incrementing for second accumulator
         * Y pointer is decrementing for second accumulator
         */
        pX = pA;
        pY = pB;
        MVE_INTR_CORR_DUAL_DEC_Y_INC_SIZE_Q7(acc0, acc1, pX, pY, count);

        *pDst = (q7_t) acc0;
        pDst += incr;
        *pDst = (q7_t) acc1;
        pDst += incr;
        pB -= 2;
    }
    for (; i < block1; i++)
    {
        uint32_t  count = i + 1;
        int32_t   acc = 0;

        pX = pA;
        pY = pB;
        MVE_INTR_CORR_SINGLE_Q7(acc, pX, pY, count);

        *pDst = (q7_t) acc;
        pDst += incr;
        pB--;
    }

    for (i = 0U; i <= block2 - 4; i += 4)
    {
        int32_t   acc0 = 0;
        int32_t   acc1 = 0;
        int32_t   acc2 = 0;
        int32_t   acc3 = 0;

        pX = pA;
        pY = pB;
        /*
         * compute 4 accumulators per loop
         * size is fixed for all accumulators
         * X pointer is incrementing for successive accumulators
         */
        MVE_INTR_CORR_QUAD_INC_X_FIXED_SIZE_Q7(acc0, acc1, acc2, acc3, pX, pY, srcBLen);

        *pDst = (q7_t) acc0;
        pDst += incr;
        *pDst = (q7_t) acc1;
        pDst += incr;
        *pDst = (q7_t) acc2;
        pDst += incr;
        *pDst = (q7_t) acc3;
        pDst += incr;

        pA += 4;
    }

    for (; i <= block2 - 2; i += 2)
    {
        int32_t   acc0 = 0LL;
        int32_t   acc1 = 0LL;

        pX = pA;
        pY = pB;
        /*
         * compute 2 accumulators per loop
         * size is fixed for all accumulators
         * X pointer is incrementing for second accumulator
         */
        MVE_INTR_CORR_DUAL_INC_X_FIXED_SIZE_Q7(acc0, acc1, pX, pY, srcBLen);

        *pDst = (q7_t) acc0;
        pDst += incr;
        *pDst = (q7_t) acc1;
        pDst += incr;
        pA += 2;
    }

    if (block2 & 1)
    {
        int32_t   acc = 0LL;

        pX = pA;
        pY = pB;
        MVE_INTR_CORR_SINGLE_Q7(acc, pX, pY, srcBLen);

        *pDst = (q7_t) acc;
        pDst += incr;
        pA++;
    }

    for (i = block3 - 1; i >= 0; i -= 2)
    {
        uint32_t  count = (i + 1);
        int32_t   acc0 = 0LL;
        int32_t   acc1 = 0LL;

        pX = pA;
        pY = pB;
        /*
         * compute 2 accumulators per loop
         * size is decrementing for second accumulator
         * X pointer is incrementing for second accumulator
         */
        MVE_INTR_CORR_DUAL_INC_X_DEC_SIZE_Q7(acc0, acc1, pX, pY, count);

        *pDst = (q7_t) acc0;
        pDst += incr;
        *pDst = (q7_t) acc1;
        pDst += incr;
        pA += 2;

    }
    for (; i >= 0; i--)
    {
        uint32_t  count = (i + 1);
        int64_t   acc = 0LL;

        pX = pA;
        pY = pB;
        MVE_INTR_CORR_SINGLE_Q7(acc, pX, pY, count);

        *pDst = (q7_t) acc;
        pDst += incr;
        pA++;
    }
}

#else
void arm_correlate_q7(
  const q7_t * pSrcA,
        uint32_t srcALen,
  const q7_t * pSrcB,
        uint32_t srcBLen,
        q7_t * pDst)
{

#if (1)
//#if !defined(ARM_MATH_CM0_FAMILY)

  const q7_t *pIn1;                                    /* InputA pointer */
  const q7_t *pIn2;                                    /* InputB pointer */
        q7_t *pOut = pDst;                             /* Output pointer */
  const q7_t *px;                                      /* Intermediate inputA pointer */
  const q7_t *py;                                      /* Intermediate inputB pointer */
  const q7_t *pSrc1;                                   /* Intermediate pointers */
        q31_t sum;                                     /* Accumulators */
        uint32_t blockSize1, blockSize2, blockSize3;   /* Loop counters */
        uint32_t j, k, count, blkCnt;                  /* Loop counters */
        uint32_t outBlockSize;
        int32_t inc = 1;

#if defined (ARM_MATH_LOOPUNROLL)
        q31_t acc0, acc1, acc2, acc3;                  /* Accumulators */
        q31_t input1, input2;                          /* Temporary input variables */
        q15_t in1, in2;                                /* Temporary input variables */
        q7_t x0, x1, x2, x3, c0, c1;                   /* Temporary variables for holding input and coefficient values */
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
    outBlockSize = (2U * srcALen) - 1U;

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

  /* The function is internally
   * divided into three stages according to the number of multiplications that has to be
   * taken place between inputA samples and inputB samples. In the first stage of the
   * algorithm, the multiplications increase by one for every iteration.
   * In the second stage of the algorithm, srcBLen number of multiplications are done.
   * In the third stage of the algorithm, the multiplications decrease by one
   * for every iteration. */

  /* The algorithm is implemented in three stages.
     The loop counters of each stage is initiated here. */
  blockSize1 = srcBLen - 1U;
  blockSize2 = srcALen - (srcBLen - 1U);
  blockSize3 = blockSize1;

  /* --------------------------
   * Initializations of stage1
   * -------------------------*/

  /* sum = x[0] * y[srcBlen - 1]
   * sum = x[0] * y[srcBlen - 2] + x[1] * y[srcBlen - 1]
   * ....
   * sum = x[0] * y[0] + x[1] * y[1] +...+ x[srcBLen - 1] * y[srcBLen - 1]
   */

  /* In this stage the MAC operations are increased by 1 for every iteration.
     The count variable holds the number of MAC operations performed */
  count = 1U;

  /* Working pointer of inputA */
  px = pIn1;

  /* Working pointer of inputB */
  pSrc1 = pIn2 + (srcBLen - 1U);
  py = pSrc1;

  /* ------------------------
   * Stage1 process
   * ----------------------*/

  /* The first stage starts here */
  while (blockSize1 > 0U)
  {
    /* Accumulator is made zero for every iteration */
    sum = 0;

#if defined (ARM_MATH_LOOPUNROLL)

    /* Loop unrolling: Compute 4 outputs at a time */
    k = count >> 2U;

    while (k > 0U)
    {
      /* x[0] , x[1] */
      in1 = (q15_t) *px++;
      in2 = (q15_t) *px++;
      input1 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16);

      /* y[srcBLen - 4] , y[srcBLen - 3] */
      in1 = (q15_t) *py++;
      in2 = (q15_t) *py++;
      input2 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16);

      /* x[0] * y[srcBLen - 4] */
      /* x[1] * y[srcBLen - 3] */
      sum = __SMLAD(input1, input2, sum);

      /* x[2] , x[3] */
      in1 = (q15_t) *px++;
      in2 = (q15_t) *px++;
      input1 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16);

      /* y[srcBLen - 2] , y[srcBLen - 1] */
      in1 = (q15_t) *py++;
      in2 = (q15_t) *py++;
      input2 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16);

      /* x[2] * y[srcBLen - 2] */
      /* x[3] * y[srcBLen - 1] */
      sum = __SMLAD(input1, input2, sum);

      /* Decrement loop counter */
      k--;
    }

    /* Loop unrolling: Compute remaining outputs */
    k = count % 0x4U;

#else

    /* Initialize k with number of samples */
    k = count;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

    while (k > 0U)
    {
      /* Perform the multiply-accumulate */
      /* x[0] * y[srcBLen - 1] */
      sum += (q31_t) ((q15_t) *px++ * *py++);

      /* Decrement loop counter */
      k--;
    }

    /* Store the result in the accumulator in the destination buffer. */
    *pOut = (q7_t) (__SSAT(sum >> 7U, 8));
    /* Destination pointer is updated according to the address modifier, inc */
    pOut += inc;

    /* Update the inputA and inputB pointers for next MAC calculation */
    py = pSrc1 - count;
    px = pIn1;

    /* Increment MAC count */
    count++;

    /* Decrement loop counter */
    blockSize1--;
  }

  /* --------------------------
   * Initializations of stage2
   * ------------------------*/

  /* sum = x[0] * y[0] + x[1] * y[1] +...+ x[srcBLen-1] * y[srcBLen-1]
   * sum = x[1] * y[0] + x[2] * y[1] +...+ x[srcBLen] * y[srcBLen-1]
   * ....
   * sum = x[srcALen-srcBLen-2] * y[0] + x[srcALen-srcBLen-1] * y[1] +...+ x[srcALen-1] * y[srcBLen-1]
   */

  /* Working pointer of inputA */
  px = pIn1;

  /* Working pointer of inputB */
  py = pIn2;

  /* count is index by which the pointer pIn1 to be incremented */
  count = 0U;

  /* -------------------
   * Stage2 process
   * ------------------*/

  /* Stage2 depends on srcBLen as in this stage srcBLen number of MACS are performed.
   * So, to loop unroll over blockSize2,
   * srcBLen should be greater than or equal to 4 */
  if (srcBLen >= 4U)
  {
#if defined (ARM_MATH_LOOPUNROLL)

    /* Loop unrolling: Compute 4 outputs at a time */
    blkCnt = blockSize2 >> 2U;

    while (blkCnt > 0U)
    {
      /* Set all accumulators to zero */
      acc0 = 0;
      acc1 = 0;
      acc2 = 0;
      acc3 = 0;

      /* read x[0], x[1], x[2] samples */
      x0 = *px++;
      x1 = *px++;
      x2 = *px++;

      /* Apply loop unrolling and compute 4 MACs simultaneously. */
      k = srcBLen >> 2U;

      /* First part of the processing with loop unrolling.  Compute 4 MACs at a time.
       ** a second loop below computes MACs for the remaining 1 to 3 samples. */
      do
      {
        /* Read y[0] sample */
        c0 = *py++;
        /* Read y[1] sample */
        c1 = *py++;

        /* Read x[3] sample */
        x3 = *px++;

        /* x[0] and x[1] are packed */
        in1 = (q15_t) x0;
        in2 = (q15_t) x1;

        input1 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

        /* y[0] and y[1] are packed */
        in1 = (q15_t) c0;
        in2 = (q15_t) c1;

        input2 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

        /* acc0 += x[0] * y[0] + x[1] * y[1]  */
        acc0 = __SMLAD(input1, input2, acc0);

        /* x[1] and x[2] are packed */
        in1 = (q15_t) x1;
        in2 = (q15_t) x2;

        input1 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

        /* acc1 += x[1] * y[0] + x[2] * y[1] */
        acc1 = __SMLAD(input1, input2, acc1);

        /* x[2] and x[3] are packed */
        in1 = (q15_t) x2;
        in2 = (q15_t) x3;

        input1 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

        /* acc2 += x[2] * y[0] + x[3] * y[1]  */
        acc2 = __SMLAD(input1, input2, acc2);

        /* Read x[4] sample */
        x0 = *px++;

        /* x[3] and x[4] are packed */
        in1 = (q15_t) x3;
        in2 = (q15_t) x0;

        input1 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

        /* acc3 += x[3] * y[0] + x[4] * y[1]  */
        acc3 = __SMLAD(input1, input2, acc3);

        /* Read y[2] sample */
        c0 = *py++;
        /* Read y[3] sample */
        c1 = *py++;

        /* Read x[5] sample */
        x1 = *px++;

        /* x[2] and x[3] are packed */
        in1 = (q15_t) x2;
        in2 = (q15_t) x3;

        input1 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

        /* y[2] and y[3] are packed */
        in1 = (q15_t) c0;
        in2 = (q15_t) c1;

        input2 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

        /* acc0 += x[2] * y[2] + x[3] * y[3]  */
        acc0 = __SMLAD(input1, input2, acc0);

        /* x[3] and x[4] are packed */
        in1 = (q15_t) x3;
        in2 = (q15_t) x0;

        input1 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

        /* acc1 += x[3] * y[2] + x[4] * y[3]  */
        acc1 = __SMLAD(input1, input2, acc1);

        /* x[4] and x[5] are packed */
        in1 = (q15_t) x0;
        in2 = (q15_t) x1;

        input1 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

        /* acc2 += x[4] * y[2] + x[5] * y[3]  */
        acc2 = __SMLAD(input1, input2, acc2);

        /* Read x[6] sample */
        x2 = *px++;

        /* x[5] and x[6] are packed */
        in1 = (q15_t) x1;
        in2 = (q15_t) x2;

        input1 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

        /* acc3 += x[5] * y[2] + x[6] * y[3]  */
        acc3 = __SMLAD(input1, input2, acc3);

      } while (--k);

      /* If the srcBLen is not a multiple of 4, compute any remaining MACs here.
       ** No loop unrolling is used. */
      k = srcBLen % 0x4U;

      while (k > 0U)
      {
        /* Read y[4] sample */
        c0 = *py++;
        /* Read x[7] sample */
        x3 = *px++;

        /* Perform the multiply-accumulates */
        /* acc0 +=  x[4] * y[4] */
        acc0 += ((q15_t) x0 * c0);
        /* acc1 +=  x[5] * y[4] */
        acc1 += ((q15_t) x1 * c0);
        /* acc2 +=  x[6] * y[4] */
        acc2 += ((q15_t) x2 * c0);
        /* acc3 +=  x[7] * y[4] */
        acc3 += ((q15_t) x3 * c0);

        /* Reuse the present samples for the next MAC */
        x0 = x1;
        x1 = x2;
        x2 = x3;

        /* Decrement loop counter */
        k--;
      }

      /* Store the result in the accumulator in the destination buffer. */
      *pOut = (q7_t) (__SSAT(acc0 >> 7, 8));
      /* Destination pointer is updated according to the address modifier, inc */
      pOut += inc;

      *pOut = (q7_t) (__SSAT(acc1 >> 7, 8));
      pOut += inc;

      *pOut = (q7_t) (__SSAT(acc2 >> 7, 8));
      pOut += inc;

      *pOut = (q7_t) (__SSAT(acc3 >> 7, 8));
      pOut += inc;

	  count += 4U;
      /* Update the inputA and inputB pointers for next MAC calculation */
      px = pIn1 + count;
      py = pIn2;

      /* Decrement loop counter */
      blkCnt--;
    }

    /* Loop unrolling: Compute remaining outputs */
    blkCnt = blockSize2 % 0x4U;

#else

    /* Initialize blkCnt with number of samples */
    blkCnt = blockSize2;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

    while (blkCnt > 0U)
    {
      /* Accumulator is made zero for every iteration */
      sum = 0;

#if defined (ARM_MATH_LOOPUNROLL)

    /* Loop unrolling: Compute 4 outputs at a time */
      k = srcBLen >> 2U;

      while (k > 0U)
      {

        /* Reading two inputs of SrcA buffer and packing */
        in1 = (q15_t) *px++;
        in2 = (q15_t) *px++;
        input1 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

        /* Reading two inputs of SrcB buffer and packing */
        in1 = (q15_t) *py++;
        in2 = (q15_t) *py++;
        input2 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

        /* Perform the multiply-accumulate */
        sum = __SMLAD(input1, input2, sum);

        /* Reading two inputs of SrcA buffer and packing */
        in1 = (q15_t) *px++;
        in2 = (q15_t) *px++;
        input1 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

        /* Reading two inputs of SrcB buffer and packing */
        in1 = (q15_t) *py++;
        in2 = (q15_t) *py++;
        input2 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

        /* Perform the multiply-accumulate */
        sum = __SMLAD(input1, input2, sum);

        /* Decrement loop counter */
        k--;
      }

      /* Loop unrolling: Compute remaining outputs */
      k = srcBLen % 0x4U;

#else

      /* Initialize blkCnt with number of samples */
      k = srcBLen;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

      while (k > 0U)
      {
        /* Perform the multiply-accumulate */
        sum += ((q15_t) *px++ * *py++);

        /* Decrement the loop counter */
        k--;
      }

      /* Store the result in the accumulator in the destination buffer. */
      *pOut = (q7_t) (__SSAT(sum >> 7U, 8));
      /* Destination pointer is updated according to the address modifier, inc */
      pOut += inc;

      /* Increment the pointer pIn1 index, count by 1 */
      count++;

      /* Update the inputA and inputB pointers for next MAC calculation */
      px = pIn1 + count;
      py = pIn2;

      /* Decrement the loop counter */
      blkCnt--;
    }
  }
  else
  {
    /* If the srcBLen is not a multiple of 4,
     * the blockSize2 loop cannot be unrolled by 4 */
    blkCnt = blockSize2;

    while (blkCnt > 0U)
    {
      /* Accumulator is made zero for every iteration */
      sum = 0;

      /* srcBLen number of MACS should be performed */
      k = srcBLen;

      while (k > 0U)
      {
        /* Perform the multiply-accumulate */
        sum += ((q15_t) *px++ * *py++);

        /* Decrement the loop counter */
        k--;
      }

      /* Store the result in the accumulator in the destination buffer. */
      *pOut = (q7_t) (__SSAT(sum >> 7U, 8));
      /* Destination pointer is updated according to the address modifier, inc */
      pOut += inc;

      /* Increment the MAC count */
      count++;

      /* Update the inputA and inputB pointers for next MAC calculation */
      px = pIn1 + count;
      py = pIn2;

      /* Decrement loop counter */
      blkCnt--;
    }
  }


  /* --------------------------
   * Initializations of stage3
   * -------------------------*/

  /* sum += x[srcALen-srcBLen+1] * y[0] + x[srcALen-srcBLen+2] * y[1] +...+ x[srcALen-1] * y[srcBLen-1]
   * sum += x[srcALen-srcBLen+2] * y[0] + x[srcALen-srcBLen+3] * y[1] +...+ x[srcALen-1] * y[srcBLen-1]
   * ....
   * sum +=  x[srcALen-2] * y[0] + x[srcALen-1] * y[1]
   * sum +=  x[srcALen-1] * y[0]
   */

  /* In this stage the MAC operations are decreased by 1 for every iteration.
     The count variable holds the number of MAC operations performed */
  count = srcBLen - 1U;

  /* Working pointer of inputA */
  pSrc1 = pIn1 + (srcALen - (srcBLen - 1U));
  px = pSrc1;

  /* Working pointer of inputB */
  py = pIn2;

  /* -------------------
   * Stage3 process
   * ------------------*/

  while (blockSize3 > 0U)
  {
    /* Accumulator is made zero for every iteration */
    sum = 0;

#if defined (ARM_MATH_LOOPUNROLL)

    /* Loop unrolling: Compute 4 outputs at a time */
    k = count >> 2U;

    while (k > 0U)
    {
      /* x[srcALen - srcBLen + 1] , x[srcALen - srcBLen + 2]  */
      in1 = (q15_t) *px++;
      in2 = (q15_t) *px++;
      input1 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

      /* y[0] , y[1] */
      in1 = (q15_t) *py++;
      in2 = (q15_t) *py++;
      input2 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

      /* sum += x[srcALen - srcBLen + 1] * y[0] */
      /* sum += x[srcALen - srcBLen + 2] * y[1] */
      sum = __SMLAD(input1, input2, sum);

      /* x[srcALen - srcBLen + 3] , x[srcALen - srcBLen + 4] */
      in1 = (q15_t) *px++;
      in2 = (q15_t) *px++;
      input1 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

      /* y[2] , y[3] */
      in1 = (q15_t) *py++;
      in2 = (q15_t) *py++;
      input2 = ((q31_t) in1 & 0x0000FFFF) | ((q31_t) in2 << 16U);

      /* sum += x[srcALen - srcBLen + 3] * y[2] */
      /* sum += x[srcALen - srcBLen + 4] * y[3] */
      sum = __SMLAD(input1, input2, sum);

      /* Decrement loop counter */
      k--;
    }

    /* Loop unrolling: Compute remaining outputs */
    k = count % 0x4U;

#else

    /* Initialize blkCnt with number of samples */
    k = count;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

    while (k > 0U)
    {
      /* Perform the multiply-accumulate */
      sum += ((q15_t) *px++ * *py++);

      /* Decrement loop counter */
      k--;
    }

    /* Store the result in the accumulator in the destination buffer. */
    *pOut = (q7_t) (__SSAT(sum >> 7U, 8));
    /* Destination pointer is updated according to the address modifier, inc */
    pOut += inc;

    /* Update the inputA and inputB pointers for next MAC calculation */
    px = ++pSrc1;
    py = pIn2;

    /* Decrement MAC count */
    count--;

    /* Decrement loop counter */
    blockSize3--;
  }

#else
/* alternate version for CM0_FAMILY */

  const q7_t *pIn1 = pSrcA;                            /* InputA pointer */
  const q7_t *pIn2 = pSrcB + (srcBLen - 1U);           /* InputB pointer */
        q31_t sum;                                     /* Accumulator */
        uint32_t i = 0U, j;                            /* Loop counters */
        uint32_t inv = 0U;                             /* Reverse order flag */
        uint32_t tot = 0U;                             /* Length */

  /* The algorithm implementation is based on the lengths of the inputs. */
  /* srcB is always made to slide across srcA. */
  /* So srcBLen is always considered as shorter or equal to srcALen */
  /* But CORR(x, y) is reverse of CORR(y, x) */
  /* So, when srcBLen > srcALen, output pointer is made to point to the end of the output buffer */
  /* and a varaible, inv is set to 1 */
  /* If lengths are not equal then zero pad has to be done to  make the two
   * inputs of same length. But to improve the performance, we include zeroes
   * in the output instead of zero padding either of the the inputs*/
  /* If srcALen > srcBLen, (srcALen - srcBLen) zeroes has to included in the
   * starting of the output buffer */
  /* If srcALen < srcBLen, (srcALen - srcBLen) zeroes has to included in the
   * ending of the output buffer */
  /* Once the zero padding is done the remaining of the output is calcualted
   * using convolution but with the shorter signal time shifted. */

  /* Calculate the length of the remaining sequence */
  tot = ((srcALen + srcBLen) - 2U);

  if (srcALen > srcBLen)
  {
    /* Calculating the number of zeros to be padded to the output */
    j = srcALen - srcBLen;

    /* Initialise the pointer after zero padding */
    pDst += j;
  }

  else if (srcALen < srcBLen)
  {
    /* Initialization to inputB pointer */
    pIn1 = pSrcB;

    /* Initialization to the end of inputA pointer */
    pIn2 = pSrcA + (srcALen - 1U);

    /* Initialisation of the pointer after zero padding */
    pDst = pDst + tot;

    /* Swapping the lengths */
    j = srcALen;
    srcALen = srcBLen;
    srcBLen = j;

    /* Setting the reverse flag */
    inv = 1;
  }

  /* Loop to calculate convolution for output length number of times */
  for (i = 0U; i <= tot; i++)
  {
    /* Initialize sum with zero to carry out MAC operations */
    sum = 0;

    /* Loop to perform MAC operations according to convolution equation */
    for (j = 0U; j <= i; j++)
    {
      /* Check the array limitations */
      if (((i - j) < srcBLen) && (j < srcALen))
      {
        /* z[i] += x[i-j] * y[j] */
        sum += ((q15_t) pIn1[j] * pIn2[-((int32_t) i - j)]);
      }
    }

    /* Store the output in the destination buffer */
    if (inv == 1)
      *pDst-- = (q7_t) __SSAT((sum >> 7U), 8U);
    else
      *pDst++ = (q7_t) __SSAT((sum >> 7U), 8U);
  }

#endif /* #if !defined(ARM_MATH_CM0_FAMILY) */

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of Corr group
 */
