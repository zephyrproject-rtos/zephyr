/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_fir_q7.c
 * Description:  Q7 FIR filter processing function
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
  @addtogroup FIR
  @{
 */

/**
  @brief         Processing function for Q7 FIR filter.
  @param[in]     S          points to an instance of the Q7 FIR filter structure
  @param[in]     pSrc       points to the block of input data
  @param[out]    pDst       points to the block of output data
  @param[in]     blockSize  number of samples to process
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using a 32-bit internal accumulator.
                   Both coefficients and state variables are represented in 1.7 format and multiplications yield a 2.14 result.
                   The 2.14 intermediate results are accumulated in a 32-bit accumulator in 18.14 format.
                   There is no risk of internal overflow with this approach and the full precision of intermediate multiplications is preserved.
                   The accumulator is converted to 18.7 format by discarding the low 7 bits.
                   Finally, the result is truncated to 1.7 format.
 */

#if defined(ARM_MATH_MVEI)

void arm_fir_q7_1_16_mve(const arm_fir_instance_q7 * S, const q7_t * pSrc, q7_t * pDst, uint32_t blockSize)
{
    q7_t     *pState = S->pState;   /* State pointer */
    const q7_t     *pCoeffs = S->pCoeffs; /* Coefficient pointer */
    q7_t     *pStateCur;        /* Points to the current sample of the state */
    const q7_t     *pSamples;         /* Temporary pointer to the sample buffer */
    q7_t     *pOutput;          /* Temporary pointer to the output buffer */
    const q7_t     *pTempSrc;         /* Temporary pointer to the source data */
    q7_t     *pTempDest;        /* Temporary pointer to the destination buffer */
    uint32_t  numTaps = S->numTaps; /* Number of filter coefficients in the filter */
    uint32_t  blkCnt;
    q7x16_t  vecIn0;
    q31_t     acc0, acc1, acc2, acc3;
    q7x16_t  vecCoeffs;

    /*
     * pState points to state array which contains previous frame (numTaps - 1) samples
     * pStateCur points to the location where the new input data should be written
     */
    pStateCur   = &(pState[(numTaps - 1u)]);
    pSamples    = pState;
    pTempSrc    = pSrc;
    pOutput     = pDst;
    blkCnt      = blockSize >> 2;

    /*
     * load 16 coefs
     */
    vecCoeffs = *(q7x16_t *) pCoeffs;

    while (blkCnt > 0U)
    {
        /*
         * Save 16 input samples in the history buffer
         */
        vst1q(pStateCur, vld1q(pTempSrc));
        pStateCur += 16;
        pTempSrc += 16;

        vecIn0 = vld1q(pSamples);
        acc0 = vmladavq(vecIn0, vecCoeffs);

        vecIn0 = vld1q(&pSamples[1]);;
        acc1 = vmladavq(vecIn0, vecCoeffs);

        vecIn0 = vld1q(&pSamples[2]);;
        acc2 = vmladavq(vecIn0, vecCoeffs);

        vecIn0 = vld1q(&pSamples[3]);
        acc3 = vmladavq(vecIn0, vecCoeffs);

        /*
         * Store the 1.7 format filter output in destination buffer
         */
        *pOutput++ = (q7_t) __SSAT((acc0 >> 7U), 8);
        *pOutput++ = (q7_t) __SSAT((acc1 >> 7U), 8);
        *pOutput++ = (q7_t) __SSAT((acc2 >> 7U), 8);
        *pOutput++ = (q7_t) __SSAT((acc3 >> 7U), 8);

        pSamples += 4;
        /*
         * Decrement the sample block loop counter
         */
        blkCnt--;
    }

    uint32_t  residual = blockSize & 3;
    switch (residual)
    {
    case 3:
        {
            vst1q(pStateCur, vld1q(pTempSrc));
            pStateCur += 16;
            pTempSrc += 16;

            vecIn0 = vld1q(pSamples);
            acc0 = vmladavq(vecIn0, vecCoeffs);

            vecIn0 = vld1q(&pSamples[1]);
            acc1 = vmladavq(vecIn0, vecCoeffs);

            vecIn0 = vld1q(&pSamples[2]);
            acc2 = vmladavq(vecIn0, vecCoeffs);

            *pOutput++ = (q7_t) __SSAT((acc0 >> 7U), 8);
            *pOutput++ = (q7_t) __SSAT((acc1 >> 7U), 8);
            *pOutput++ = (q7_t) __SSAT((acc2 >> 7U), 8);
        }
        break;

    case 2:
        {
            vst1q(pStateCur, vld1q(pTempSrc));
            pStateCur += 16;
            pTempSrc += 16;

            vecIn0 = vld1q(pSamples);
            acc0 = vmladavq(vecIn0, vecCoeffs);

            vecIn0 = vld1q(&pSamples[1]);
            acc1 = vmladavq(vecIn0, vecCoeffs);

            *pOutput++ = (q7_t) __SSAT((acc0 >> 7U), 8);
            *pOutput++ = (q7_t) __SSAT((acc1 >> 7U), 8);
        }
        break;

    case 1:
        {
            vst1q(pStateCur, vld1q(pTempSrc));
            pStateCur += 16;
            pTempSrc += 16;

            vecIn0 = vld1q(pSamples);
            acc0 = vmladavq(vecIn0, vecCoeffs);

            *pOutput++ = (q7_t) __SSAT((acc0 >> 7U), 8);
        }
        break;
    }

    /*
     * Copy the samples back into the history buffer start
     */
    pTempSrc = &pState[blockSize];
    pTempDest = pState;

    blkCnt = numTaps >> 4;
    while (blkCnt > 0U)
    {
        vst1q(pTempDest, vld1q(pTempSrc));
        pTempSrc += 16;
        pTempDest += 16;
        blkCnt--;
    }
    blkCnt = numTaps & 0xF;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp8q(blkCnt);
        vstrbq_p_s8(pTempDest, vld1q(pTempSrc), p0);
    }
}

void arm_fir_q7(
  const arm_fir_instance_q7 * S,
  const q7_t * pSrc,
        q7_t * pDst,
        uint32_t blockSize)
{
    q7_t     *pState = S->pState;   /* State pointer */
    const q7_t     *pCoeffs = S->pCoeffs; /* Coefficient pointer */
    q7_t     *pStateCur;        /* Points to the current sample of the state */
    const q7_t     *pSamples;         /* Temporary pointer to the sample buffer */
    q7_t     *pOutput;          /* Temporary pointer to the output buffer */
    const q7_t     *pTempSrc;         /* Temporary pointer to the source data */
    q7_t     *pTempDest;        /* Temporary pointer to the destination buffer */
    uint32_t  numTaps = S->numTaps; /* Number of filter coefficients in the filter */
    uint32_t  blkCnt;
    q7x16_t  vecIn0;
    uint32_t  tapsBlkCnt = (numTaps + 15) / 16;
    q31_t     acc0, acc1, acc2, acc3;
    q7x16_t  vecCoeffs;

    if (blockSize >= 20)
    {
        if (numTaps <= 16)
        {
            /*
             * [1 to 16 taps] specialized routine
             */
            arm_fir_q7_1_16_mve(S, pSrc, pDst, blockSize);
            return;
        }
    }

    if (blockSize >= 20)
    {
      /*
       * pState points to state array which contains previous frame (numTaps - 1) samples
       * pStateCur points to the location where the new input data should be written
       */
      pStateCur   = &(pState[(numTaps - 1u)]);
      pSamples    = pState;
      pTempSrc    = pSrc;
      pOutput     = pDst;
      blkCnt      = blockSize >> 2;
  
      /*
       * outer samples loop
       */
      while (blkCnt > 0U)
      {
          const q7_t     *pCoeffsTmp = pCoeffs;
          const q7_t     *pSamplesTmp = pSamples;
  
          acc0 = 0;
          acc1 = 0;
          acc2 = 0;
          acc3 = 0;
          /*
           * Save 16 input samples in the history buffer
           */
          vst1q(pStateCur, vld1q(pTempSrc));
          pStateCur += 16;
          pTempSrc += 16;
  
          /*
           * inner coefficients loop
           */
          uint32_t       i = tapsBlkCnt;
          while (i > 0U)
          {
              /*
               * load 16 coefs
               */
              vecCoeffs = *(q7x16_t *) pCoeffsTmp;
  
              vecIn0 = vld1q(pSamplesTmp);
              acc0 = vmladavaq(acc0, vecIn0, vecCoeffs);
  
              vecIn0 = vld1q(&pSamplesTmp[1]);
              acc1 = vmladavaq(acc1, vecIn0, vecCoeffs);
  
              vecIn0 = vld1q(&pSamplesTmp[2]);
              acc2 = vmladavaq(acc2, vecIn0, vecCoeffs);
  
              vecIn0 = vld1q(&pSamplesTmp[3]);
              acc3 = vmladavaq(acc3, vecIn0, vecCoeffs);
  
              pSamplesTmp += 16;
              pCoeffsTmp += 16;
              /*
               * Decrement the taps block loop counter
               */
              i--;
          }
          /*
           * Store the 1.7 format filter output in destination buffer
           */
          *pOutput++ = (q7_t) __SSAT((acc0 >> 7U), 8);
          *pOutput++ = (q7_t) __SSAT((acc1 >> 7U), 8);
          *pOutput++ = (q7_t) __SSAT((acc2 >> 7U), 8);
          *pOutput++ = (q7_t) __SSAT((acc3 >> 7U), 8);
  
          pSamples += 4;
          /*
           * Decrement the sample block loop counter
           */
          blkCnt--;
      }
  
      uint32_t  residual = blockSize & 3;
      switch (residual)
      {
      case 3:
          {
              const q7_t     *pCoeffsTmp = pCoeffs;
              const q7_t     *pSamplesTmp = pSamples;
  
              acc0 = 0;
              acc1 = 0;
              acc2 = 0;
              /*
               * Save 16 input samples in the history buffer
               */
              vst1q(pStateCur, vld1q(pTempSrc));
              pStateCur += 16;
              pTempSrc += 16;
  
              uint32_t       i = tapsBlkCnt;
              while (i > 0U)
              {
                  vecCoeffs = *(q7x16_t *) pCoeffsTmp;
  
                  vecIn0 = vld1q(pSamplesTmp);
                  acc0 = vmladavaq(acc0, vecIn0, vecCoeffs);
  
                  vecIn0 = vld1q(&pSamplesTmp[1]);
                  acc1 = vmladavaq(acc1, vecIn0, vecCoeffs);
  
                  vecIn0 = vld1q(&pSamplesTmp[2]);
                  acc2 = vmladavaq(acc2, vecIn0, vecCoeffs);
  
                  pSamplesTmp += 16;
                  pCoeffsTmp += 16;
                  i--;
              }
  
              *pOutput++ = (q7_t) __SSAT((acc0 >> 7U), 8);
              *pOutput++ = (q7_t) __SSAT((acc1 >> 7U), 8);
              *pOutput++ = (q7_t) __SSAT((acc2 >> 7U), 8);
          }
          break;
  
      case 2:
          {
              const q7_t     *pCoeffsTmp = pCoeffs;
              const q7_t     *pSamplesTmp = pSamples;
  
              acc0 = 0;
              acc1 = 0;
              /*
               * Save 16 input samples in the history buffer
               */
              vst1q(pStateCur, vld1q(pTempSrc));
              pStateCur += 16;
              pTempSrc += 16;
  
              uint32_t       i = tapsBlkCnt;
              while (i > 0U)
              {
                  vecCoeffs = *(q7x16_t *) pCoeffsTmp;
  
                  vecIn0 = vld1q(pSamplesTmp);
                  acc0 = vmladavaq(acc0, vecIn0, vecCoeffs);
  
                  vecIn0 = vld1q(&pSamplesTmp[1]);
                  acc1 = vmladavaq(acc1, vecIn0, vecCoeffs);
  
                  pSamplesTmp += 16;
                  pCoeffsTmp += 16;
                  i--;
              }
  
              *pOutput++ = (q7_t) __SSAT((acc0 >> 7U), 8);
              *pOutput++ = (q7_t) __SSAT((acc1 >> 7U), 8);
          }
          break;
  
      case 1:
          {
              const q7_t     *pCoeffsTmp = pCoeffs;
              const q7_t     *pSamplesTmp = pSamples;
  
              acc0 = 0;
              /*
               * Save 16 input samples in the history buffer
               */
              vst1q(pStateCur, vld1q(pTempSrc));
              pStateCur += 16;
              pTempSrc += 16;
  
              uint32_t       i = tapsBlkCnt;
              while (i > 0U)
              {
                  vecCoeffs = *(q7x16_t *) pCoeffsTmp;
  
                  vecIn0 = vld1q(pSamplesTmp);
                  acc0 = vmladavaq(acc0, vecIn0, vecCoeffs);
  
                  pSamplesTmp += 16;
                  pCoeffsTmp += 16;
                  i--;
              }
              *pOutput++ = (q7_t) __SSAT((acc0 >> 7U), 8);
          }
          break;
      }
    }
    else
    {
        q7_t *pStateCurnt;                            /* Points to the current sample of the state */
            q7_t *px;                                     /* Temporary pointer for state buffer */
      const q7_t *pb;                                     /* Temporary pointer for coefficient buffer */
            q31_t acc0;                                    /* Accumulator */
            uint32_t  i,blkCnt;                    /* Loop counters */
      pStateCurnt = &(S->pState[(numTaps - 1U)]);
      blkCnt = blockSize;

         while (blkCnt > 0U)
           {
             /* Copy one sample at a time into state buffer */
             *pStateCurnt++ = *pSrc++;
         
             /* Set the accumulator to zero */
             acc0 = 0;
         
             /* Initialize state pointer */
             px = pState;
         
             /* Initialize Coefficient pointer */
             pb = pCoeffs;
         
             i = numTaps;
         
             /* Perform the multiply-accumulates */
             while (i > 0U)
             {
               acc0 += (q15_t) * (px++) * (*(pb++));
               i--;
             } 
         
             /* The result is in 2.14 format. Convert to 1.7
                Then store the output in the destination buffer. */
             *pDst++ = __SSAT((acc0 >> 7U), 8);
         
             /* Advance state pointer by 1 for the next sample */
             pState = pState + 1U;
         
             /* Decrement loop counter */
             blkCnt--;
           }
    }
    /*
     * Copy the samples back into the history buffer start
     */
    pTempSrc = &S->pState[blockSize];
    pTempDest = S->pState;

    blkCnt = numTaps >> 4;
    while (blkCnt > 0U)
    {
        vst1q(pTempDest, vld1q(pTempSrc));
        pTempSrc += 16;
        pTempDest += 16;
        blkCnt--;
    }
    blkCnt = numTaps & 0xF;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp8q(blkCnt);
        vstrbq_p_s8(pTempDest, vld1q(pTempSrc), p0);
    }
}
#else
void arm_fir_q7(
  const arm_fir_instance_q7 * S,
  const q7_t * pSrc,
        q7_t * pDst,
        uint32_t blockSize)
{
        q7_t *pState = S->pState;                      /* State pointer */
  const q7_t *pCoeffs = S->pCoeffs;                    /* Coefficient pointer */
        q7_t *pStateCurnt;                             /* Points to the current sample of the state */
        q7_t *px;                                      /* Temporary pointer for state buffer */
  const q7_t *pb;                                      /* Temporary pointer for coefficient buffer */
        q31_t acc0;                                    /* Accumulators */
        uint32_t numTaps = S->numTaps;                 /* Number of filter coefficients in the filter */
        uint32_t i, tapCnt, blkCnt;                    /* Loop counters */

#if defined (ARM_MATH_LOOPUNROLL)
        q31_t acc1, acc2, acc3;                        /* Accumulators */
        q7_t x0, x1, x2, x3, c0;                       /* Temporary variables to hold state */
#endif

  /* S->pState points to state array which contains previous frame (numTaps - 1) samples */
  /* pStateCurnt points to the location where the new input data should be written */
  pStateCurnt = &(S->pState[(numTaps - 1U)]);

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 output values simultaneously.
   * The variables acc0 ... acc3 hold output values that are being computed:
   *
   *    acc0 =  b[numTaps-1] * x[n-numTaps-1] + b[numTaps-2] * x[n-numTaps-2] + b[numTaps-3] * x[n-numTaps-3] +...+ b[0] * x[0]
   *    acc1 =  b[numTaps-1] * x[n-numTaps]   + b[numTaps-2] * x[n-numTaps-1] + b[numTaps-3] * x[n-numTaps-2] +...+ b[0] * x[1]
   *    acc2 =  b[numTaps-1] * x[n-numTaps+1] + b[numTaps-2] * x[n-numTaps]   + b[numTaps-3] * x[n-numTaps-1] +...+ b[0] * x[2]
   *    acc3 =  b[numTaps-1] * x[n-numTaps+2] + b[numTaps-2] * x[n-numTaps+1] + b[numTaps-3] * x[n-numTaps]   +...+ b[0] * x[3]
   */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* Copy 4 new input samples into the state buffer. */
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;

    /* Set all accumulators to zero */
    acc0 = 0;
    acc1 = 0;
    acc2 = 0;
    acc3 = 0;

    /* Initialize state pointer */
    px = pState;

    /* Initialize coefficient pointer */
    pb = pCoeffs;

    /* Read the first 3 samples from the state buffer:
     *  x[n-numTaps], x[n-numTaps-1], x[n-numTaps-2] */
    x0 = *px++;
    x1 = *px++;
    x2 = *px++;

    /* Loop unrolling. Process 4 taps at a time. */
    tapCnt = numTaps >> 2U;

    /* Loop over the number of taps.  Unroll by a factor of 4.
       Repeat until we've computed numTaps-4 coefficients. */
    while (tapCnt > 0U)
    {
      /* Read the b[numTaps] coefficient */
      c0 = *pb;

      /* Read x[n-numTaps-3] sample */
      x3 = *px;

      /* acc0 +=  b[numTaps] * x[n-numTaps] */
      acc0 += ((q15_t) x0 * c0);

      /* acc1 +=  b[numTaps] * x[n-numTaps-1] */
      acc1 += ((q15_t) x1 * c0);

      /* acc2 +=  b[numTaps] * x[n-numTaps-2] */
      acc2 += ((q15_t) x2 * c0);

      /* acc3 +=  b[numTaps] * x[n-numTaps-3] */
      acc3 += ((q15_t) x3 * c0);

      /* Read the b[numTaps-1] coefficient */
      c0 = *(pb + 1U);

      /* Read x[n-numTaps-4] sample */
      x0 = *(px + 1U);

      /* Perform the multiply-accumulates */
      acc0 += ((q15_t) x1 * c0);
      acc1 += ((q15_t) x2 * c0);
      acc2 += ((q15_t) x3 * c0);
      acc3 += ((q15_t) x0 * c0);

      /* Read the b[numTaps-2] coefficient */
      c0 = *(pb + 2U);

      /* Read x[n-numTaps-5] sample */
      x1 = *(px + 2U);

      /* Perform the multiply-accumulates */
      acc0 += ((q15_t) x2 * c0);
      acc1 += ((q15_t) x3 * c0);
      acc2 += ((q15_t) x0 * c0);
      acc3 += ((q15_t) x1 * c0);

      /* Read the b[numTaps-3] coefficients */
      c0 = *(pb + 3U);

      /* Read x[n-numTaps-6] sample */
      x2 = *(px + 3U);

      /* Perform the multiply-accumulates */
      acc0 += ((q15_t) x3 * c0);
      acc1 += ((q15_t) x0 * c0);
      acc2 += ((q15_t) x1 * c0);
      acc3 += ((q15_t) x2 * c0);

      /* update coefficient pointer */
      pb += 4U;
      px += 4U;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* If the filter length is not a multiple of 4, compute the remaining filter taps */
    tapCnt = numTaps % 0x4U;

    while (tapCnt > 0U)
    {
      /* Read coefficients */
      c0 = *(pb++);

      /* Fetch 1 state variable */
      x3 = *(px++);

      /* Perform the multiply-accumulates */
      acc0 += ((q15_t) x0 * c0);
      acc1 += ((q15_t) x1 * c0);
      acc2 += ((q15_t) x2 * c0);
      acc3 += ((q15_t) x3 * c0);

      /* Reuse the present sample states for next sample */
      x0 = x1;
      x1 = x2;
      x2 = x3;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* The results in the 4 accumulators are in 2.62 format. Convert to 1.31
       Then store the 4 outputs in the destination buffer. */
    acc0 = __SSAT((acc0 >> 7U), 8);
    *pDst++ = acc0;
    acc1 = __SSAT((acc1 >> 7U), 8);
    *pDst++ = acc1;
    acc2 = __SSAT((acc2 >> 7U), 8);
    *pDst++ = acc2;
    acc3 = __SSAT((acc3 >> 7U), 8);
    *pDst++ = acc3;

    /* Advance the state pointer by 4 to process the next group of 4 samples */
    pState = pState + 4U;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining output samples */
  blkCnt = blockSize % 0x4U;

#else

  /* Initialize blkCnt with number of taps */
  blkCnt = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* Copy one sample at a time into state buffer */
    *pStateCurnt++ = *pSrc++;

    /* Set the accumulator to zero */
    acc0 = 0;

    /* Initialize state pointer */
    px = pState;

    /* Initialize Coefficient pointer */
    pb = pCoeffs;

    i = numTaps;

    /* Perform the multiply-accumulates */
    while (i > 0U)
    {
      acc0 += (q15_t) * (px++) * (*(pb++));
      i--;
    } 

    /* The result is in 2.14 format. Convert to 1.7
       Then store the output in the destination buffer. */
    *pDst++ = __SSAT((acc0 >> 7U), 8);

    /* Advance state pointer by 1 for the next sample */
    pState = pState + 1U;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Processing is complete.
     Now copy the last numTaps - 1 samples to the start of the state buffer.
     This prepares the state buffer for the next function call. */

  /* Points to the start of the state buffer */
  pStateCurnt = S->pState;

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 taps at a time */
  tapCnt = (numTaps - 1U) >> 2U;

  /* Copy data */
  while (tapCnt > 0U)
  {
    *pStateCurnt++ = *pState++;
    *pStateCurnt++ = *pState++;
    *pStateCurnt++ = *pState++;
    *pStateCurnt++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

  /* Calculate remaining number of copies */
  tapCnt = (numTaps - 1U) % 0x4U;

#else

  /* Initialize tapCnt with number of taps */
  tapCnt = (numTaps - 1U);

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  /* Copy remaining data */
  while (tapCnt > 0U)
  {
    *pStateCurnt++ = *pState++;

    /* Decrement the loop counter */
    tapCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of FIR group
 */
