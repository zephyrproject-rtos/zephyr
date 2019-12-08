/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_correlate_f32.c
 * Description:  Correlation of floating-point sequences
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
  @defgroup Corr Correlation

  Correlation is a mathematical operation that is similar to convolution.
  As with convolution, correlation uses two signals to produce a third signal.
  The underlying algorithms in correlation and convolution are identical except that one of the inputs is flipped in convolution.
  Correlation is commonly used to measure the similarity between two signals.
  It has applications in pattern recognition, cryptanalysis, and searching.
  The CMSIS library provides correlation functions for Q7, Q15, Q31 and floating-point data types.
  Fast versions of the Q15 and Q31 functions are also provided.

  @par           Algorithm
                   Let <code>a[n]</code> and <code>b[n]</code> be sequences of length <code>srcALen</code> and <code>srcBLen</code> samples respectively.
                   The convolution of the two signals is denoted by
  <pre>
      c[n] = a[n] * b[n]
  </pre>
                   In correlation, one of the signals is flipped in time
  <pre>
       c[n] = a[n] * b[-n]
  </pre>
  @par
                   and this is mathematically defined as
                   \image html CorrelateEquation.gif
  @par
                   The <code>pSrcA</code> points to the first input vector of length <code>srcALen</code> and <code>pSrcB</code> points to the second input vector of length <code>srcBLen</code>.
                   The result <code>c[n]</code> is of length <code>2 * max(srcALen, srcBLen) - 1</code> and is defined over the interval <code>n=0, 1, 2, ..., (2 * max(srcALen, srcBLen) - 2)</code>.
                   The output result is written to <code>pDst</code> and the calling function must allocate <code>2 * max(srcALen, srcBLen) - 1</code> words for the result.

  @note
                   The <code>pDst</code> should be initialized to all zeros before being used.

  @par           Fixed-Point Behavior
                   Correlation requires summing up a large number of intermediate products.
                   As such, the Q7, Q15, and Q31 functions run a risk of overflow and saturation.
                   Refer to the function specific documentation below for further details of the particular algorithm used.

  @par           Fast Versions
                   Fast versions are supported for Q31 and Q15.  Cycles for Fast versions are less compared to Q31 and Q15 of correlate and the design requires
                   the input signals should be scaled down to avoid intermediate overflows.

  @par           Opt Versions
                   Opt versions are supported for Q15 and Q7.  Design uses internal scratch buffer for getting good optimisation.
                   These versions are optimised in cycles and consumes more memory (Scratch memory) compared to Q15 and Q7 versions of correlate
 */

/**
  @addtogroup Corr
  @{
 */

/**
  @brief         Correlation of floating-point sequences.
  @param[in]     pSrcA      points to the first input sequence
  @param[in]     srcALen    length of the first input sequence
  @param[in]     pSrcB      points to the second input sequence
  @param[in]     srcBLen    length of the second input sequence
  @param[out]    pDst       points to the location where the output result is written.  Length 2 * max(srcALen, srcBLen) - 1.
  @return        none
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"
#include "arm_vec_filtering.h"


void arm_correlate_f32(
  const float32_t * pSrcA,
        uint32_t srcALen,
  const float32_t * pSrcB,
        uint32_t srcBLen,
        float32_t * pDst)
{
    const float32_t *pIn1 = pSrcA;    /* inputA pointer               */
    const float32_t *pIn2 = pSrcB + (srcBLen - 1U);   /* inputB pointer               */
    const float32_t *pX, *pY;
    const float32_t *pA, *pB;
    int32_t   i = 0U, j = 0;    /* loop counters */
    int32_t   inv = 4U;         /* Reverse order flag */
    uint32_t  tot = 0U;         /* Length */
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
        inv = -4;
    }

    block1 = srcBLen - 1;
    block2 = srcALen - srcBLen + 1;
    block3 = srcBLen - 1;

    pA = pIn1;
    pB = pIn2;
    incr = inv / 4;

    for (i = 0U; i <= block1 - 2; i += 2)
    {
        uint32_t  count = i + 1;
        float32_t acc0;
        float32_t acc1;

        /*
         * compute 2 accumulators per loop
         * size is incrementing for second accumulator
         * Y pointer is decrementing for second accumulator
         */
        pX = pA;
        pY = pB;
        MVE_INTR_CORR_DUAL_DEC_Y_INC_SIZE_F32(acc0, acc1, pX, pY, count);

        *pDst = acc0;
        pDst += incr;
        *pDst = acc1;
        pDst += incr;
        pB -= 2;
    }
    for (; i < block1; i++)
    {
        uint32_t  count = i + 1;
        float32_t acc;

        pX = pA;
        pY = pB;
        MVE_INTR_CORR_SINGLE_F32(acc, pX, pY, count);

        *pDst = acc;
        pDst += incr;
        pB--;
    }

    for (i = 0U; i <= block2 - 4; i += 4)
    {
        float32_t acc0;
        float32_t acc1;
        float32_t acc2;
        float32_t acc3;

        pX = pA;
        pY = pB;
        /*
         * compute 4 accumulators per loop
         * size is fixed for all accumulators
         * X pointer is incrementing for successive accumulators
         */
        MVE_INTR_CORR_QUAD_INC_X_FIXED_SIZE_F32(acc0, acc1, acc2, acc3, pX, pY, srcBLen);

        *pDst = acc0;
        pDst += incr;
        *pDst = acc1;
        pDst += incr;
        *pDst = acc2;
        pDst += incr;
        *pDst = acc3;
        pDst += incr;
        pA += 4;
    }

    for (; i <= block2 - 2; i += 2)
    {
        float32_t acc0;
        float32_t acc1;

        pX = pA;
        pY = pB;
        /*
         * compute 2 accumulators per loop
         * size is fixed for all accumulators
         * X pointer is incrementing for second accumulator
         */
        MVE_INTR_CORR_DUAL_INC_X_FIXED_SIZE_F32(acc0, acc1, pX, pY, srcBLen);

        *pDst = acc0;
        pDst += incr;
        *pDst = acc1;
        pDst += incr;
        pA += 2;
    }

    if (block2 & 1)
    {
        float32_t acc;

        pX = pA;
        pY = pB;
        MVE_INTR_CORR_SINGLE_F32(acc, pX, pY, srcBLen);

        *pDst = acc;
        pDst += incr;
        pA++;
    }

    for (i = block3 - 1; i >= 0; i -= 2)
    {

        uint32_t  count = (i + 1);
        float32_t acc0;
        float32_t acc1;

        pX = pA;
        pY = pB;
        /*
         * compute 2 accumulators per loop
         * size is decrementing for second accumulator
         * X pointer is incrementing for second accumulator
         */
        MVE_INTR_CORR_DUAL_INC_X_DEC_SIZE_F32(acc0, acc1, pX, pY, count);

        *pDst = acc0;
        pDst += incr;
        *pDst = acc1;
        pDst += incr;
        pA += 2;

    }
    for (; i >= 0; i--)
    {
        uint32_t  count = (i + 1);
        float32_t acc;

        pX = pA;
        pY = pB;
        MVE_INTR_CORR_SINGLE_F32(acc, pX, pY, count);

        *pDst = acc;
        pDst += incr;
        pA++;
    }
}

#else
void arm_correlate_f32(
  const float32_t * pSrcA,
        uint32_t srcALen,
  const float32_t * pSrcB,
        uint32_t srcBLen,
        float32_t * pDst)
{

#if defined(ARM_MATH_DSP) && !defined(ARM_MATH_AUTOVECTORIZE)
  
  const float32_t *pIn1;                               /* InputA pointer */
  const float32_t *pIn2;                               /* InputB pointer */
        float32_t *pOut = pDst;                        /* Output pointer */
  const float32_t *px;                                 /* Intermediate inputA pointer */
  const float32_t *py;                                 /* Intermediate inputB pointer */
  const float32_t *pSrc1;
        float32_t sum;
        uint32_t blockSize1, blockSize2, blockSize3;   /* Loop counters */
        uint32_t j, k, count, blkCnt;                  /* Loop counters */
        uint32_t outBlockSize;                         /* Loop counter */
        int32_t inc = 1;                               /* Destination address modifier */

#if defined (ARM_MATH_LOOPUNROLL) || defined(ARM_MATH_NEON)
    float32_t acc0, acc1, acc2, acc3,c0;                    /* Accumulators */
#if !defined(ARM_MATH_NEON)
    float32_t x0, x1, x2, x3;                        /* temporary variables for holding input and coefficient values */
#endif
#endif

  /* The algorithm implementation is based on the lengths of the inputs. */
  /* srcB is always made to slide across srcA. */
  /* So srcBLen is always considered as shorter or equal to srcALen */
  /* But CORR(x, y) is reverse of CORR(y, x) */
  /* So, when srcBLen > srcALen, output pointer is made to point to the end of the output buffer */
  /* and the destination pointer modifier, inc is set to -1 */
  /* If srcALen > srcBLen, zero pad has to be done to srcB to make the two inputs of same length */
  /* But to improve the performance,
   * we assume zeroes in the output instead of zero padding either of the the inputs*/
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

    /* When srcALen > srcBLen, zero padding has to be done to srcB
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
   * sum = x[0] * y[srcBlen-2] + x[1] * y[srcBlen - 1]
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
    sum = 0.0f;

#if defined (ARM_MATH_LOOPUNROLL) || defined(ARM_MATH_NEON)

    /* Loop unrolling: Compute 4 outputs at a time */
    k = count >> 2U;

#if defined(ARM_MATH_NEON)
    float32x4_t x,y;
    float32x4_t res = vdupq_n_f32(0) ;
    float32x2_t accum = vdup_n_f32(0);

    while (k > 0U)
    {
      x = vld1q_f32(px);
      y = vld1q_f32(py);

      res = vmlaq_f32(res,x, y);

      px += 4;
      py += 4;

      /* Decrement the loop counter */
      k--;
    }

    accum = vpadd_f32(vget_low_f32(res), vget_high_f32(res));
    sum += accum[0] + accum[1];

    k = count & 0x3;
#else
    /* First part of the processing with loop unrolling.  Compute 4 MACs at a time.
     ** a second loop below computes MACs for the remaining 1 to 3 samples. */
    while (k > 0U)
    {
      /* x[0] * y[srcBLen - 4] */
      sum += *px++ * *py++;

      /* x[1] * y[srcBLen - 3] */
      sum += *px++ * *py++;

      /* x[2] * y[srcBLen - 2] */
      sum += *px++ * *py++;

      /* x[3] * y[srcBLen - 1] */
      sum += *px++ * *py++;

      /* Decrement loop counter */
      k--;
    }

    /* Loop unrolling: Compute remaining outputs */
    k = count % 0x4U;

#endif /* #if defined(ARM_MATH_NEON) */
#else

    /* Initialize k with number of samples */
    k = count;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) || defined(ARM_MATH_NEON) */

    while (k > 0U)
    {
      /* Perform the multiply-accumulate */
      /* x[0] * y[srcBLen - 1] */
      sum += *px++ * *py++;

      /* Decrement loop counter */
      k--;
    }

    /* Store the result in the accumulator in the destination buffer. */
    *pOut = sum;
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
   * sum = x[1] * y[0] + x[2] * y[1] +...+ x[srcBLen]   * y[srcBLen-1]
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
#if defined (ARM_MATH_LOOPUNROLL) || defined(ARM_MATH_NEON)

    /* Loop unrolling: Compute 4 outputs at a time */
    blkCnt = blockSize2 >> 2U;

#if defined(ARM_MATH_NEON)
      float32x4_t c;
      float32x4_t x1v;
      float32x4_t x2v;
      float32x4_t x;
      float32x4_t res = vdupq_n_f32(0) ;
#endif /* #if defined(ARM_MATH_NEON) */

    while (blkCnt > 0U)
    {
      /* Set all accumulators to zero */
      acc0 = 0.0f;
      acc1 = 0.0f;
      acc2 = 0.0f;
      acc3 = 0.0f;

#if defined(ARM_MATH_NEON)
      /* Compute 4 MACs simultaneously. */
      k = srcBLen >> 2U;

      res = vdupq_n_f32(0) ;

      x1v = vld1q_f32(px);
      px += 4;
      do
      {
        x2v = vld1q_f32(px);
        c = vld1q_f32(py);

        py += 4;

        x = x1v;
        res = vmlaq_n_f32(res,x,c[0]);

        x = vextq_f32(x1v,x2v,1);

        res = vmlaq_n_f32(res,x,c[1]);

        x = vextq_f32(x1v,x2v,2);

	res = vmlaq_n_f32(res,x,c[2]);

        x = vextq_f32(x1v,x2v,3);

	res = vmlaq_n_f32(res,x,c[3]);

        x1v = x2v;
        px+=4;
      } while (--k);
      
      /* If the srcBLen is not a multiple of 4, compute any remaining MACs here.
       ** No loop unrolling is used. */
      k = srcBLen & 0x3;

      while (k > 0U)
      {
        /* Read y[srcBLen - 5] sample */
        c0 = *(py++);

        res = vmlaq_n_f32(res,x1v,c0);

        /* Reuse the present samples for the next MAC */
        x1v[0] = x1v[1];
        x1v[1] = x1v[2];
        x1v[2] = x1v[3];

        x1v[3] = *(px++);

        /* Decrement the loop counter */
        k--;
      }

      px-=1;

      acc0 = res[0];
      acc1 = res[1];
      acc2 = res[2];
      acc3 = res[3];
#else
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
        c0 = *(py++);
        /* Read x[3] sample */
        x3 = *(px++);

        /* Perform the multiply-accumulate */
        /* acc0 +=  x[0] * y[0] */
        acc0 += x0 * c0;
        /* acc1 +=  x[1] * y[0] */
        acc1 += x1 * c0;
        /* acc2 +=  x[2] * y[0] */
        acc2 += x2 * c0;
        /* acc3 +=  x[3] * y[0] */
        acc3 += x3 * c0;

        /* Read y[1] sample */
        c0 = *(py++);
        /* Read x[4] sample */
        x0 = *(px++);

        /* Perform the multiply-accumulate */
        /* acc0 +=  x[1] * y[1] */
        acc0 += x1 * c0;
        /* acc1 +=  x[2] * y[1] */
        acc1 += x2 * c0;
        /* acc2 +=  x[3] * y[1] */
        acc2 += x3 * c0;
        /* acc3 +=  x[4] * y[1] */
        acc3 += x0 * c0;

        /* Read y[2] sample */
        c0 = *(py++);
        /* Read x[5] sample */
        x1 = *(px++);

        /* Perform the multiply-accumulate */
        /* acc0 +=  x[2] * y[2] */
        acc0 += x2 * c0;
        /* acc1 +=  x[3] * y[2] */
        acc1 += x3 * c0;
        /* acc2 +=  x[4] * y[2] */
        acc2 += x0 * c0;
        /* acc3 +=  x[5] * y[2] */
        acc3 += x1 * c0;

        /* Read y[3] sample */
        c0 = *(py++);
        /* Read x[6] sample */
        x2 = *(px++);

        /* Perform the multiply-accumulate */
        /* acc0 +=  x[3] * y[3] */
        acc0 += x3 * c0;
        /* acc1 +=  x[4] * y[3] */
        acc1 += x0 * c0;
        /* acc2 +=  x[5] * y[3] */
        acc2 += x1 * c0;
        /* acc3 +=  x[6] * y[3] */
        acc3 += x2 * c0;

      } while (--k);

      /* If the srcBLen is not a multiple of 4, compute any remaining MACs here.
       ** No loop unrolling is used. */
      k = srcBLen % 0x4U;

      while (k > 0U)
      {
        /* Read y[4] sample */
        c0 = *(py++);
        /* Read x[7] sample */
        x3 = *(px++);

        /* Perform the multiply-accumulate */
        /* acc0 +=  x[4] * y[4] */
        acc0 += x0 * c0;
        /* acc1 +=  x[5] * y[4] */
        acc1 += x1 * c0;
        /* acc2 +=  x[6] * y[4] */
        acc2 += x2 * c0;
        /* acc3 +=  x[7] * y[4] */
        acc3 += x3 * c0;

        /* Reuse the present samples for the next MAC */
        x0 = x1;
        x1 = x2;
        x2 = x3;

        /* Decrement the loop counter */
        k--;
      }

#endif /* #if defined(ARM_MATH_NEON) */

      /* Store the result in the accumulator in the destination buffer. */
      *pOut = acc0;
      /* Destination pointer is updated according to the address modifier, inc */
      pOut += inc;

      *pOut = acc1;
      pOut += inc;

      *pOut = acc2;
      pOut += inc;

      *pOut = acc3;
      pOut += inc;

      /* Increment the pointer pIn1 index, count by 4 */
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

#endif /* #if defined (ARM_MATH_LOOPUNROLL) || defined(ARM_MATH_NEON) */

    while (blkCnt > 0U)
    {
      /* Accumulator is made zero for every iteration */
      sum = 0.0f;

#if defined (ARM_MATH_LOOPUNROLL) || defined(ARM_MATH_NEON)

    /* Loop unrolling: Compute 4 outputs at a time */
      k = srcBLen >> 2U;

#if defined(ARM_MATH_NEON)
    float32x4_t x,y;
    float32x4_t res = vdupq_n_f32(0) ;
    float32x2_t accum = vdup_n_f32(0);

    while (k > 0U)
    {
      x = vld1q_f32(px);
      y = vld1q_f32(py);

      res = vmlaq_f32(res,x, y);

      px += 4;
      py += 4;
      /* Decrement the loop counter */
      k--;
    }

    accum = vpadd_f32(vget_low_f32(res), vget_high_f32(res));
    sum += accum[0] + accum[1];
#else
      /* First part of the processing with loop unrolling.  Compute 4 MACs at a time.
       ** a second loop below computes MACs for the remaining 1 to 3 samples. */
      while (k > 0U)
      {
        /* Perform the multiply-accumulate */
        sum += *px++ * *py++;
        sum += *px++ * *py++;
        sum += *px++ * *py++;
        sum += *px++ * *py++;

        /* Decrement loop counter */
        k--;
      }
#endif /* #if defined(ARM_MATH_NEON) */
      /* If the srcBLen is not a multiple of 4, compute any remaining MACs here.
       ** No loop unrolling is used. */
      k = srcBLen % 0x4U;
#else

      /* Initialize blkCnt with number of samples */
      k = srcBLen;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) || defined(ARM_MATH_NEON) */

      while (k > 0U)
      {
        /* Perform the multiply-accumulate */
        sum += *px++ * *py++;

        /* Decrement the loop counter */
        k--;
      }

      /* Store the result in the accumulator in the destination buffer. */
      *pOut = sum;

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
      sum = 0.0f;

      /* Loop over srcBLen */
      k = srcBLen;

      while (k > 0U)
      {
        /* Perform the multiply-accumulate */
        sum += *px++ * *py++;

        /* Decrement the loop counter */
        k--;
      }

      /* Store the result in the accumulator in the destination buffer. */
      *pOut = sum;
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
    sum = 0.0f;

#if defined (ARM_MATH_LOOPUNROLL) || defined(ARM_MATH_NEON)

    /* Loop unrolling: Compute 4 outputs at a time */
    k = count >> 2U;

#if defined(ARM_MATH_NEON)
    float32x4_t x,y;
    float32x4_t res = vdupq_n_f32(0) ;
    float32x2_t accum = vdup_n_f32(0);

    while (k > 0U)
    {
      x = vld1q_f32(px);
      y = vld1q_f32(py);

      res = vmlaq_f32(res,x, y);

      px += 4;
      py += 4;

      /* Decrement the loop counter */
      k--;
    }

    accum = vpadd_f32(vget_low_f32(res), vget_high_f32(res));
    sum += accum[0] + accum[1];
#else
    /* First part of the processing with loop unrolling.  Compute 4 MACs at a time.
     ** a second loop below computes MACs for the remaining 1 to 3 samples. */
    while (k > 0U)
    {
      /* Perform the multiply-accumulate */
      /* sum += x[srcALen - srcBLen + 4] * y[3] */
      sum += *px++ * *py++;

      /* sum += x[srcALen - srcBLen + 3] * y[2] */
      sum += *px++ * *py++;

      /* sum += x[srcALen - srcBLen + 2] * y[1] */
      sum += *px++ * *py++;

      /* sum += x[srcALen - srcBLen + 1] * y[0] */
      sum += *px++ * *py++;

      /* Decrement loop counter */
      k--;
    }

#endif /* #if defined (ARM_MATH_NEON) */
    /* Loop unrolling: Compute remaining outputs */
    k = count % 0x4U;

#else

    /* Initialize blkCnt with number of samples */
    k = count;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) || defined(ARM_MATH_NEON) */

    while (k > 0U)
    {
      /* Perform the multiply-accumulate */
      sum += *px++ * *py++;

      /* Decrement loop counter */
      k--;
    }

    /* Store the result in the accumulator in the destination buffer. */
    *pOut = sum;
    /* Destination pointer is updated according to the address modifier, inc */
    pOut += inc;

    /* Update the inputA and inputB pointers for next MAC calculation */
    px = ++pSrc1;
    py = pIn2;

    /* Decrement MAC count */
    count--;

    /* Decrement the loop counter */
    blockSize3--;
  }

#else
/* alternate version for CM0_FAMILY */

  const float32_t *pIn1 = pSrcA;                       /* inputA pointer */
  const float32_t *pIn2 = pSrcB + (srcBLen - 1U);      /* inputB pointer */
        float32_t sum;                                 /* Accumulator */
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
   * inputs of same length. But to improve the performance, we assume zeroes
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
    sum = 0.0f;

    /* Loop to perform MAC operations according to convolution equation */
    for (j = 0U; j <= i; j++)
    {
      /* Check the array limitations */
      if ((((i - j) < srcBLen) && (j < srcALen)))
      {
        /* z[i] += x[i-j] * y[j] */
        sum += pIn1[j] * pIn2[-((int32_t) i - j)];
      }
    }

    /* Store the output in the destination buffer */
    if (inv == 1)
      *pDst-- = sum;
    else
      *pDst++ = sum;
  }

#endif /* #if !defined(ARM_MATH_CM0_FAMILY) */

}
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of Corr group
 */
