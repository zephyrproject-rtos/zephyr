/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_fir_f32.c
 * Description:  Floating-point FIR filter processing function
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
  @defgroup FIR Finite Impulse Response (FIR) Filters

  This set of functions implements Finite Impulse Response (FIR) filters
  for Q7, Q15, Q31, and floating-point data types.  Fast versions of Q15 and Q31 are also provided.
  The functions operate on blocks of input and output data and each call to the function processes
  <code>blockSize</code> samples through the filter.  <code>pSrc</code> and
  <code>pDst</code> points to input and output arrays containing <code>blockSize</code> values.

  @par           Algorithm
                   The FIR filter algorithm is based upon a sequence of multiply-accumulate (MAC) operations.
                   Each filter coefficient <code>b[n]</code> is multiplied by a state variable which equals a previous input sample <code>x[n]</code>.
  <pre>
      y[n] = b[0] * x[n] + b[1] * x[n-1] + b[2] * x[n-2] + ...+ b[numTaps-1] * x[n-numTaps+1]
  </pre>
  @par
                   \image html FIR.GIF "Finite Impulse Response filter"
  @par
                   <code>pCoeffs</code> points to a coefficient array of size <code>numTaps</code>.
                   Coefficients are stored in time reversed order.
  @par
  <pre>
      {b[numTaps-1], b[numTaps-2], b[N-2], ..., b[1], b[0]}
  </pre>
  @par
                   <code>pState</code> points to a state array of size <code>numTaps + blockSize - 1</code>.
                   Samples in the state buffer are stored in the following order.
  @par
  <pre>
      {x[n-numTaps+1], x[n-numTaps], x[n-numTaps-1], x[n-numTaps-2]....x[0], x[1], ..., x[blockSize-1]}
  </pre>
  @par
                   Note that the length of the state buffer exceeds the length of the coefficient array by <code>blockSize-1</code>.
                   The increased state buffer length allows circular addressing, which is traditionally used in the FIR filters,
                   to be avoided and yields a significant speed improvement.
                   The state variables are updated after each block of data is processed; the coefficients are untouched.

  @par           Instance Structure
                   The coefficients and state variables for a filter are stored together in an instance data structure.
                   A separate instance structure must be defined for each filter.
                   Coefficient arrays may be shared among several instances while state variable arrays cannot be shared.
                   There are separate instance structure declarations for each of the 4 supported data types.

  @par           Initialization Functions
                   There is also an associated initialization function for each data type.
                   The initialization function performs the following operations:
                   - Sets the values of the internal structure fields.
                   - Zeros out the values in the state buffer.
                   To do this manually without calling the init function, assign the follow subfields of the instance structure:
                   numTaps, pCoeffs, pState. Also set all of the values in pState to zero.
  @par
                   Use of the initialization function is optional.
                   However, if the initialization function is used, then the instance structure cannot be placed into a const data section.
                   To place an instance structure into a const data section, the instance structure must be manually initialized.
                   Set the values in the state buffer to zeros before static initialization.
                   The code below statically initializes each of the 4 different data type filter instance structures
  <pre>
      arm_fir_instance_f32 S = {numTaps, pState, pCoeffs};
      arm_fir_instance_q31 S = {numTaps, pState, pCoeffs};
      arm_fir_instance_q15 S = {numTaps, pState, pCoeffs};
      arm_fir_instance_q7 S =  {numTaps, pState, pCoeffs};
  </pre>
                   where <code>numTaps</code> is the number of filter coefficients in the filter; <code>pState</code> is the address of the state buffer;
                   <code>pCoeffs</code> is the address of the coefficient buffer.
  @par          Initialization of Helium version
                 For Helium version the array of coefficients must be a multiple of 16 even if less
                 then 16 coefficients are used. The additional coefficients must be set to 0.
                 It does not mean that all the coefficients will be used in the filter (numTaps
                 is still set to its right value in the init function.) It just means that
                 the implementation may require to read more coefficients due to the vectorization and
                 to avoid having to manage too many different cases in the code.

  @par           Fixed-Point Behavior
                   Care must be taken when using the fixed-point versions of the FIR filter functions.
                   In particular, the overflow and saturation behavior of the accumulator used in each function must be considered.
                   Refer to the function specific documentation below for usage guidelines.
 */

/**
  @addtogroup FIR
  @{
 */

/**
  @brief         Processing function for floating-point FIR filter.
  @param[in]     S          points to an instance of the floating-point FIR filter structure
  @param[in]     pSrc       points to the block of input data
  @param[out]    pDst       points to the block of output data
  @param[in]     blockSize  number of samples to process
  @return        none
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

static void arm_fir_f32_1_4_mve(const arm_fir_instance_f32 * S, const float32_t * pSrc, float32_t * pDst, uint32_t blockSize)
{
    float32_t *pState = S->pState;      /* State pointer */
    const float32_t *pCoeffs = S->pCoeffs;    /* Coefficient pointer */
    float32_t *pStateCur;               /* Points to the current sample of the state */
    const float32_t *pSamples;          /* Temporary pointer to the sample buffer */
    float32_t *pOutput;                 /* Temporary pointer to the output buffer */
    const float32_t *pTempSrc;          /* Temporary pointer to the source data */
    float32_t *pTempDest;               /* Temporary pointer to the destination buffer */
    uint32_t  numTaps = S->numTaps;     /* Number of filter coefficients in the filter */
    uint32_t  blkCnt;
    f32x4_t vecIn0;
    f32x4_t vecAcc0;
    float32_t c0, c1, c2, c3;

    /*
     * pState points to state array which contains previous frame (numTaps - 1) samples
     * pStateCur points to the location where the new input data should be written
     */
    pStateCur = &(pState[(numTaps - 1u)]);
    pSamples  = pState;
    pTempSrc  = pSrc;
    pOutput   = pDst;

    if (((numTaps - 1) / 4) == 0)
    {
        const float32_t *pCoeffsCur = pCoeffs;

        c0 = *pCoeffsCur++;
        c1 = *pCoeffsCur++;
        c2 = *pCoeffsCur++;
        c3 = *pCoeffsCur++;

        blkCnt = blockSize >> 2;
        while (blkCnt > 0U)
        {
            /*
             * Save 4 input samples in the history buffer
             */
            vst1q(pStateCur, vld1q(pTempSrc));
            pStateCur += 4;
            pTempSrc += 4;

            vecIn0 = vld1q(pSamples);
            vecAcc0 = vmulq(vecIn0, c0);

            vecIn0 = vld1q(&pSamples[1]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c1);

            vecIn0 = vld1q(&pSamples[2]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c2);

            vecIn0 = vld1q(&pSamples[3]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c3);

            vst1q(pOutput, vecAcc0);

            pOutput += 4;
            pSamples += 4;

            blkCnt--;
        }
        blkCnt = blockSize & 3;
        {
            mve_pred16_t p0 = vctp32q(blkCnt);

            vstrwq_p_f32(pStateCur, vld1q(pTempSrc),p0);
            pStateCur += blkCnt;
            pTempSrc += blkCnt;

            vecIn0 = vld1q(pSamples);
            vecAcc0 = vmulq(vecIn0, c0);

            vecIn0 = vld1q(&pSamples[1]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c1);

            vecIn0 = vld1q(&pSamples[2]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c2);

            vecIn0 = vld1q(&pSamples[3]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c3);

            vstrwq_p_f32(pOutput, vecAcc0, p0);
        }
    }

    /*
     * Copy the samples back into the history buffer start
     */
    pTempSrc = &S->pState[blockSize];
    pTempDest = S->pState;
    blkCnt = numTaps >> 2;
    while (blkCnt > 0U)
    {
        vst1q(pTempDest, vld1q(pTempSrc));
        pTempSrc += 4;
        pTempDest += 4;
        blkCnt--;
    }
    blkCnt = numTaps & 3;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp32q(blkCnt);
        vstrwq_p_f32(pTempDest, vld1q(pTempSrc), p0);
    }
}



static void arm_fir_f32_5_8_mve(const arm_fir_instance_f32 * S, const float32_t * pSrc, float32_t * pDst, uint32_t blockSize)
{
    float32_t *pState = S->pState;      /* State pointer */
    const float32_t *pCoeffs = S->pCoeffs;    /* Coefficient pointer */
    float32_t *pStateCur;               /* Points to the current sample of the state */
    const float32_t *pSamples;          /* Temporary pointer to the sample buffer */
    float32_t *pOutput;                 /* Temporary pointer to the output buffer */
    const float32_t *pTempSrc;          /* Temporary pointer to the source data */
    float32_t *pTempDest;               /* Temporary pointer to the destination buffer */
    uint32_t  numTaps = S->numTaps;     /* Number of filter coefficients in the filter */
    uint32_t  blkCnt;
    f32x4_t vecIn0;
    f32x4_t vecAcc0;
    float32_t c0, c1, c2, c3;
    float32_t c4, c5, c6, c7;
    const float32_t *pCoeffsCur = pCoeffs;

    /*
     * pState points to state array which contains previous frame (numTaps - 1) samples
     * pStateCur points to the location where the new input data should be written
     */
    pStateCur = &(pState[(numTaps - 1u)]);
    pTempSrc = pSrc;

    pSamples = pState;
    pOutput = pDst;

    c0 = *pCoeffsCur++;
    c1 = *pCoeffsCur++;
    c2 = *pCoeffsCur++;
    c3 = *pCoeffsCur++;
    c4 = *pCoeffsCur++;
    c5 = *pCoeffsCur++;
    c6 = *pCoeffsCur++;
    c7 = *pCoeffsCur++;

    blkCnt = blockSize >> 2;
    while (blkCnt > 0U)
    {
        /*
         * Save 4 input samples in the history buffer
         */
        vst1q(pStateCur, vld1q(pTempSrc));
        pStateCur += 4;
        pTempSrc += 4;

        vecIn0 = vld1q(pSamples);
        vecAcc0 = vmulq(vecIn0, c0);

        vecIn0 = vld1q(&pSamples[1]);
        vecAcc0 = vfmaq(vecAcc0, vecIn0, c1);

        vecIn0 = vld1q(&pSamples[2]);
        vecAcc0 = vfmaq(vecAcc0, vecIn0, c2);

        vecIn0 = vld1q(&pSamples[3]);
        vecAcc0 = vfmaq(vecAcc0, vecIn0, c3);

        vecIn0 = vld1q(&pSamples[4]);
        vecAcc0 = vfmaq(vecAcc0, vecIn0, c4);

        vecIn0 = vld1q(&pSamples[5]);
        vecAcc0 = vfmaq(vecAcc0, vecIn0, c5);

        vecIn0 = vld1q(&pSamples[6]);
        vecAcc0 = vfmaq(vecAcc0, vecIn0, c6);

        vecIn0 = vld1q(&pSamples[7]);
        vecAcc0 = vfmaq(vecAcc0, vecIn0, c7);

        vst1q(pOutput, vecAcc0);

        pOutput += 4;
        pSamples += 4;

        blkCnt--;
    }

    blkCnt = blockSize & 3;
    {

        mve_pred16_t p0 = vctp32q(blkCnt);

        vstrwq_p_f32(pStateCur, vld1q(pTempSrc),p0);
        pStateCur += blkCnt;
        pTempSrc += blkCnt;

        vecIn0 = vld1q(pSamples);
        vecAcc0 = vmulq(vecIn0, c0);

        vecIn0 = vld1q(&pSamples[1]);
        vecAcc0 = vfmaq(vecAcc0, vecIn0, c1);

        vecIn0 = vld1q(&pSamples[2]);
        vecAcc0 = vfmaq(vecAcc0, vecIn0, c2);

        vecIn0 = vld1q(&pSamples[3]);
        vecAcc0 = vfmaq(vecAcc0, vecIn0, c3);

        vecIn0 = vld1q(&pSamples[4]);
        vecAcc0 = vfmaq(vecAcc0, vecIn0, c4);

        vecIn0 = vld1q(&pSamples[5]);
        vecAcc0 = vfmaq(vecAcc0, vecIn0, c5);

        vecIn0 = vld1q(&pSamples[6]);
        vecAcc0 = vfmaq(vecAcc0, vecIn0, c6);

        vecIn0 = vld1q(&pSamples[7]);
        vecAcc0 = vfmaq(vecAcc0, vecIn0, c7);

        vstrwq_p_f32(pOutput, vecAcc0, p0);
    }

    /*
     * Copy the samples back into the history buffer start
     */
    pTempSrc = &S->pState[blockSize];
    pTempDest = S->pState;

    blkCnt = numTaps >> 2;
    while (blkCnt > 0U)
    {
        vst1q(pTempDest, vld1q(pTempSrc));
        pTempSrc += 4;
        pTempDest += 4;
        blkCnt--;
    }
    blkCnt = numTaps & 3;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp32q(blkCnt);
        vstrwq_p_f32(pTempDest, vld1q(pTempSrc), p0);
    }
}

void arm_fir_f32(
const arm_fir_instance_f32 * S,
const float32_t * pSrc,
float32_t * pDst,
uint32_t blockSize)
{
    float32_t *pState = S->pState;      /* State pointer */
    const float32_t *pCoeffs = S->pCoeffs;    /* Coefficient pointer */
    float32_t *pStateCur;               /* Points to the current sample of the state */
    const float32_t *pSamples;          /* Temporary pointer to the sample buffer */
    float32_t *pOutput;                 /* Temporary pointer to the output buffer */
    const float32_t *pTempSrc;          /* Temporary pointer to the source data */
    float32_t *pTempDest;               /* Temporary pointer to the destination buffer */
    uint32_t  numTaps = S->numTaps;     /* Number of filter coefficients in the filter */
    uint32_t  blkCnt;
    int32_t numCnt;
    f32x4_t vecIn0;
    f32x4_t vecAcc0;
    float32_t c0, c1, c2, c3;
    float32_t c4, c5, c6, c7;

    /*
     * [1 to 8 taps] specialized routines
     */
    if (blockSize >= 8)
    {
       if (numTaps <= 4)
       {
           arm_fir_f32_1_4_mve(S, pSrc, pDst, blockSize);
           return;
       }
    }
    if (blockSize >= 8)
    {
       if (numTaps <= 8)
       {
           arm_fir_f32_5_8_mve(S, pSrc, pDst, blockSize);
           return;
       }
    }


   

    if (blockSize >= 8)
    {
        /*
         * pState points to state array which contains previous frame (numTaps - 1) samples
         * pStateCur points to the location where the new input data should be written
         */
        pStateCur = &(pState[(numTaps - 1u)]);
        pTempSrc = pSrc;
        pSamples = pState;
        pOutput = pDst;

        blkCnt = blockSize >> 2;
        while (blkCnt > 0U)
        {
            int32_t       i;
            const float32_t *pCoeffsCur = pCoeffs;
    
            c0 = *pCoeffsCur++;
            c1 = *pCoeffsCur++;
            c2 = *pCoeffsCur++;
            c3 = *pCoeffsCur++;
            c4 = *pCoeffsCur++;
            c5 = *pCoeffsCur++;
            c6 = *pCoeffsCur++;
            c7 = *pCoeffsCur++;
    
            vst1q(pStateCur, vld1q(pTempSrc));
            pStateCur += 4;
            pTempSrc += 4;
    
            vecIn0 = vld1q(pSamples);
            vecAcc0 = vmulq(vecIn0, c0);
    
            vecIn0 = vld1q(&pSamples[1]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c1);
    
            vecIn0 = vld1q(&pSamples[2]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c2);
    
            vecIn0 = vld1q(&pSamples[3]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c3);
    
            vecIn0 = vld1q(&pSamples[4]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c4);
    
            vecIn0 = vld1q(&pSamples[5]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c5);
    
            vecIn0 = vld1q(&pSamples[6]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c6);
    
            vecIn0 = vld1q(&pSamples[7]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c7);
    
            pSamples += 8;
    
            numCnt = ((int32_t)numTaps - 8) / 8;

            for (i = 0; i < numCnt; i++)
            {
                c0 = *pCoeffsCur++;
                c1 = *pCoeffsCur++;
                c2 = *pCoeffsCur++;
                c3 = *pCoeffsCur++;
                c4 = *pCoeffsCur++;
                c5 = *pCoeffsCur++;
                c6 = *pCoeffsCur++;
                c7 = *pCoeffsCur++;
    
                vecIn0 = vld1q(pSamples);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c0);
    
                vecIn0 = vld1q(&pSamples[1]);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c1);
    
                vecIn0 = vld1q(&pSamples[2]);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c2);
    
                vecIn0 = vld1q(&pSamples[3]);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c3);
    
                vecIn0 = vld1q(&pSamples[4]);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c4);
    
                vecIn0 = vld1q(&pSamples[5]);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c5);
    
                vecIn0 = vld1q(&pSamples[6]);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c6);
    
                vecIn0 = vld1q(&pSamples[7]);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c7);
    
                pSamples += 8;
            }

            numCnt = ((int32_t)numTaps - 8) & 7;

            while (numCnt > 0)
            {
                c0 = *pCoeffsCur++;
                vecIn0 = vld1q(pSamples);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c0);
                pSamples ++;

                numCnt --;
            }
    
            vst1q(pOutput, vecAcc0);
            pOutput += 4;
            pSamples = pSamples - numTaps + 4;
    
            blkCnt--;
        }
    
        blkCnt = blockSize & 3;
        {
            mve_pred16_t p0 = vctp32q(blkCnt);
            int32_t       i;
            const float32_t *pCoeffsCur = pCoeffs;
    
            vst1q(pStateCur, vld1q(pTempSrc));
            pStateCur += 4;
            pTempSrc += 4;
    
            c0 = *pCoeffsCur++;
            c1 = *pCoeffsCur++;
            c2 = *pCoeffsCur++;
            c3 = *pCoeffsCur++;
            c4 = *pCoeffsCur++;
            c5 = *pCoeffsCur++;
            c6 = *pCoeffsCur++;
            c7 = *pCoeffsCur++;
    
            vecIn0 = vld1q(pSamples);
            vecAcc0 = vmulq(vecIn0, c0);
    
            vecIn0 = vld1q(&pSamples[1]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c1);
    
            vecIn0 = vld1q(&pSamples[2]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c2);
    
            vecIn0 = vld1q(&pSamples[3]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c3);
    
            vecIn0 = vld1q(&pSamples[4]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c4);
    
            vecIn0 = vld1q(&pSamples[5]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c5);
    
            vecIn0 = vld1q(&pSamples[6]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c6);
    
            vecIn0 = vld1q(&pSamples[7]);
            vecAcc0 = vfmaq(vecAcc0, vecIn0, c7);
    
            pSamples += 8;
    
            numCnt = ((int32_t)numTaps - 8) / 8;

            for (i = 0; i < numCnt; i++)
            {
                c0 = *pCoeffsCur++;
                c1 = *pCoeffsCur++;
                c2 = *pCoeffsCur++;
                c3 = *pCoeffsCur++;
                c4 = *pCoeffsCur++;
                c5 = *pCoeffsCur++;
                c6 = *pCoeffsCur++;
                c7 = *pCoeffsCur++;
    
                vecIn0 = vld1q(pSamples);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c0);
    
                vecIn0 = vld1q(&pSamples[1]);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c1);
    
                vecIn0 = vld1q(&pSamples[2]);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c2);
    
                vecIn0 = vld1q(&pSamples[3]);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c3);
    
                vecIn0 = vld1q(&pSamples[4]);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c4);
    
                vecIn0 = vld1q(&pSamples[5]);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c5);
    
                vecIn0 = vld1q(&pSamples[6]);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c6);
    
                vecIn0 = vld1q(&pSamples[7]);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c7);
    
                pSamples += 8;
            }

            numCnt = ((int32_t)numTaps - 8) & 7;

            while (numCnt > 0)
            {
                c0 = *pCoeffsCur++;
                vecIn0 = vld1q(pSamples);
                vecAcc0 = vfmaq(vecAcc0, vecIn0, c0);
                pSamples ++;

                numCnt --;
            }
    
    
            vstrwq_p_f32(pOutput, vecAcc0, p0);
        }
    }
    else
    {
        float32_t *pStateCurnt;                        /* Points to the current sample of the state */
        float32_t *px;                                 /* Temporary pointer for state buffer */
  const float32_t *pb;                                 /* Temporary pointer for coefficient buffer */
        float32_t acc0;                                /* Accumulator */
        uint32_t numTaps = S->numTaps;                 /* Number of filter coefficients in the filter */
        uint32_t i, blkCnt;                    /* Loop counters */
        pStateCurnt = &(S->pState[(numTaps - 1U)]);
        blkCnt = blockSize;
        while (blkCnt > 0U)
        {
          /* Copy one sample at a time into state buffer */
          *pStateCurnt++ = *pSrc++;
      
          /* Set the accumulator to zero */
          acc0 = 0.0f;
      
          /* Initialize state pointer */
          px = pState;
      
          /* Initialize Coefficient pointer */
          pb = pCoeffs;
      
          i = numTaps;
      
          /* Perform the multiply-accumulates */
          while (i > 0U)
          {
            /* acc =  b[numTaps-1] * x[n-numTaps-1] + b[numTaps-2] * x[n-numTaps-2] + b[numTaps-3] * x[n-numTaps-3] +...+ b[0] * x[0] */
            acc0 += *px++ * *pb++;
      
            i--;
          }


      
          /* Store result in destination buffer. */
          *pDst++ = acc0;
      
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

    blkCnt = numTaps >> 2;
    while (blkCnt > 0U)
    {
        vst1q(pTempDest, vld1q(pTempSrc));
        pTempSrc += 4;
        pTempDest += 4;
        blkCnt--;
    }
    blkCnt = numTaps & 3;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp32q(blkCnt);
        vstrwq_p_f32(pTempDest, vld1q(pTempSrc), p0);
    }
}

#else
#if defined(ARM_MATH_NEON)

void arm_fir_f32(
const arm_fir_instance_f32 * S,
const float32_t * pSrc,
float32_t * pDst,
uint32_t blockSize)
{
   float32_t *pState = S->pState;                 /* State pointer */
   const float32_t *pCoeffs = S->pCoeffs;         /* Coefficient pointer */
   float32_t *pStateCurnt;                        /* Points to the current sample of the state */
   float32_t *px;                                 /* Temporary pointers for state buffer */
   const float32_t *pb;                           /* Temporary pointers for coefficient buffer */
   uint32_t numTaps = S->numTaps;                 /* Number of filter coefficients in the filter */
   uint32_t i, tapCnt, blkCnt;                    /* Loop counters */

   float32x4_t accv0,accv1,samples0,samples1,x0,x1,x2,xa,xb,b;
   float32_t acc;

   /* S->pState points to state array which contains previous frame (numTaps - 1) samples */
   /* pStateCurnt points to the location where the new input data should be written */
   pStateCurnt = &(S->pState[(numTaps - 1U)]);

   /* Loop unrolling */
   blkCnt = blockSize >> 3;

   while (blkCnt > 0U)
   {
      /* Copy 8 samples at a time into state buffers */
      samples0 = vld1q_f32(pSrc);
      vst1q_f32(pStateCurnt,samples0);

      pStateCurnt += 4;
      pSrc += 4 ;

      samples1 = vld1q_f32(pSrc);
      vst1q_f32(pStateCurnt,samples1);

      pStateCurnt += 4;
      pSrc += 4 ;

      /* Set the accumulators to zero */
      accv0 = vdupq_n_f32(0);
      accv1 = vdupq_n_f32(0);

      /* Initialize state pointer */
      px = pState;

      /* Initialize coefficient pointer */
      pb = pCoeffs;

      /* Loop unroling */
      i = numTaps >> 2;

      /* Perform the multiply-accumulates */
      x0 = vld1q_f32(px);
      x1 = vld1q_f32(px + 4);

      while(i > 0)
      {
         /* acc =  b[numTaps-1] * x[n-numTaps-1] + b[numTaps-2] * x[n-numTaps-2] + b[numTaps-3] * x[n-numTaps-3] +...+ b[0] * x[0] */
         x2 = vld1q_f32(px + 8);
         b = vld1q_f32(pb);
         xa = x0;
         xb = x1;
         accv0 = vmlaq_n_f32(accv0,xa,vgetq_lane_f32(b, 0));
         accv1 = vmlaq_n_f32(accv1,xb,vgetq_lane_f32(b, 0));

         xa = vextq_f32(x0,x1,1);
         xb = vextq_f32(x1,x2,1);
        
         accv0 = vmlaq_n_f32(accv0,xa,vgetq_lane_f32(b, 1));
         accv1 = vmlaq_n_f32(accv1,xb,vgetq_lane_f32(b, 1));

         xa = vextq_f32(x0,x1,2);
         xb = vextq_f32(x1,x2,2);

         accv0 = vmlaq_n_f32(accv0,xa,vgetq_lane_f32(b, 2));
         accv1 = vmlaq_n_f32(accv1,xb,vgetq_lane_f32(b, 2));

         xa = vextq_f32(x0,x1,3);
         xb = vextq_f32(x1,x2,3);
         
         accv0 = vmlaq_n_f32(accv0,xa,vgetq_lane_f32(b, 3));
         accv1 = vmlaq_n_f32(accv1,xb,vgetq_lane_f32(b, 3));

         pb += 4;
         x0 = x1;
         x1 = x2;
         px += 4;
         i--;

      }

      /* Tail */
      i = numTaps & 3;
      x2 = vld1q_f32(px + 8);

      /* Perform the multiply-accumulates */
      switch(i)
      {
         case 3:
         {
           accv0 = vmlaq_n_f32(accv0,x0,*pb);
           accv1 = vmlaq_n_f32(accv1,x1,*pb);

           pb++;

           xa = vextq_f32(x0,x1,1);
           xb = vextq_f32(x1,x2,1);

           accv0 = vmlaq_n_f32(accv0,xa,*pb);
           accv1 = vmlaq_n_f32(accv1,xb,*pb);

           pb++;

           xa = vextq_f32(x0,x1,2);
           xb = vextq_f32(x1,x2,2);
           
           accv0 = vmlaq_n_f32(accv0,xa,*pb);
           accv1 = vmlaq_n_f32(accv1,xb,*pb);

         }
         break;
         case 2:
         {
           accv0 = vmlaq_n_f32(accv0,x0,*pb);
           accv1 = vmlaq_n_f32(accv1,x1,*pb);

           pb++;

           xa = vextq_f32(x0,x1,1);
           xb = vextq_f32(x1,x2,1);
           
           accv0 = vmlaq_n_f32(accv0,xa,*pb);
           accv1 = vmlaq_n_f32(accv1,xb,*pb);

         }
         break;
         case 1:
         {
           
           accv0 = vmlaq_n_f32(accv0,x0,*pb);
           accv1 = vmlaq_n_f32(accv1,x1,*pb);

         }
         break;
         default:
         break;
      }

      /* The result is stored in the destination buffer. */
      vst1q_f32(pDst,accv0);
      pDst += 4;
      vst1q_f32(pDst,accv1);
      pDst += 4;

      /* Advance state pointer by 8 for the next 8 samples */
      pState = pState + 8;

      blkCnt--;
   }

   /* Tail */
   blkCnt = blockSize & 0x7;

   while (blkCnt > 0U)
   {
      /* Copy one sample at a time into state buffer */
      *pStateCurnt++ = *pSrc++;

      /* Set the accumulator to zero */
      acc = 0.0f;

      /* Initialize state pointer */
      px = pState;

      /* Initialize Coefficient pointer */
      pb = pCoeffs;

      i = numTaps;

      /* Perform the multiply-accumulates */
      do
      {
         /* acc =  b[numTaps-1] * x[n-numTaps-1] + b[numTaps-2] * x[n-numTaps-2] + b[numTaps-3] * x[n-numTaps-3] +...+ b[0] * x[0] */
         acc += *px++ * *pb++;
         i--;

      } while (i > 0U);

      /* The result is stored in the destination buffer. */
      *pDst++ = acc;

      /* Advance state pointer by 1 for the next sample */
      pState = pState + 1;

      blkCnt--;
   }

   /* Processing is complete.
   ** Now copy the last numTaps - 1 samples to the starting of the state buffer.
   ** This prepares the state buffer for the next function call. */

   /* Points to the start of the state buffer */
   pStateCurnt = S->pState;

   /* Copy numTaps number of values */
   tapCnt = numTaps - 1U;

   /* Copy data */
   while (tapCnt > 0U)
   {
      *pStateCurnt++ = *pState++;

      /* Decrement the loop counter */
      tapCnt--;
   }

}
#else
void arm_fir_f32(
  const arm_fir_instance_f32 * S,
  const float32_t * pSrc,
        float32_t * pDst,
        uint32_t blockSize)
{
        float32_t *pState = S->pState;                 /* State pointer */
  const float32_t *pCoeffs = S->pCoeffs;               /* Coefficient pointer */
        float32_t *pStateCurnt;                        /* Points to the current sample of the state */
        float32_t *px;                                 /* Temporary pointer for state buffer */
  const float32_t *pb;                                 /* Temporary pointer for coefficient buffer */
        float32_t acc0;                                /* Accumulator */
        uint32_t numTaps = S->numTaps;                 /* Number of filter coefficients in the filter */
        uint32_t i, tapCnt, blkCnt;                    /* Loop counters */

#if defined (ARM_MATH_LOOPUNROLL)
        float32_t acc1, acc2, acc3, acc4, acc5, acc6, acc7;     /* Accumulators */
        float32_t x0, x1, x2, x3, x4, x5, x6, x7;               /* Temporary variables to hold state values */
        float32_t c0;                                           /* Temporary variable to hold coefficient value */
#endif

  /* S->pState points to state array which contains previous frame (numTaps - 1) samples */
  /* pStateCurnt points to the location where the new input data should be written */
  pStateCurnt = &(S->pState[(numTaps - 1U)]);

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 8 output values simultaneously.
   * The variables acc0 ... acc7 hold output values that are being computed:
   *
   *    acc0 =  b[numTaps-1] * x[n-numTaps-1] + b[numTaps-2] * x[n-numTaps-2] + b[numTaps-3] * x[n-numTaps-3] +...+ b[0] * x[0]
   *    acc1 =  b[numTaps-1] * x[n-numTaps]   + b[numTaps-2] * x[n-numTaps-1] + b[numTaps-3] * x[n-numTaps-2] +...+ b[0] * x[1]
   *    acc2 =  b[numTaps-1] * x[n-numTaps+1] + b[numTaps-2] * x[n-numTaps]   + b[numTaps-3] * x[n-numTaps-1] +...+ b[0] * x[2]
   *    acc3 =  b[numTaps-1] * x[n-numTaps+2] + b[numTaps-2] * x[n-numTaps+1] + b[numTaps-3] * x[n-numTaps]   +...+ b[0] * x[3]
   */

  blkCnt = blockSize >> 3U;

  while (blkCnt > 0U)
  {
    /* Copy 4 new input samples into the state buffer. */
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;

    /* Set all accumulators to zero */
    acc0 = 0.0f;
    acc1 = 0.0f;
    acc2 = 0.0f;
    acc3 = 0.0f;
    acc4 = 0.0f;
    acc5 = 0.0f;
    acc6 = 0.0f;
    acc7 = 0.0f;

    /* Initialize state pointer */
    px = pState;

    /* Initialize coefficient pointer */
    pb = pCoeffs;

    /* This is separated from the others to avoid
     * a call to __aeabi_memmove which would be slower
     */
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;
    *pStateCurnt++ = *pSrc++;

    /* Read the first 7 samples from the state buffer:  x[n-numTaps], x[n-numTaps-1], x[n-numTaps-2] */
    x0 = *px++;
    x1 = *px++;
    x2 = *px++;
    x3 = *px++;
    x4 = *px++;
    x5 = *px++;
    x6 = *px++;

    /* Loop unrolling: process 8 taps at a time. */
    tapCnt = numTaps >> 3U;

    while (tapCnt > 0U)
    {
      /* Read the b[numTaps-1] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-3] sample */
      x7 = *(px++);

      /* acc0 +=  b[numTaps-1] * x[n-numTaps] */
      acc0 += x0 * c0;

      /* acc1 +=  b[numTaps-1] * x[n-numTaps-1] */
      acc1 += x1 * c0;

      /* acc2 +=  b[numTaps-1] * x[n-numTaps-2] */
      acc2 += x2 * c0;

      /* acc3 +=  b[numTaps-1] * x[n-numTaps-3] */
      acc3 += x3 * c0;

      /* acc4 +=  b[numTaps-1] * x[n-numTaps-4] */
      acc4 += x4 * c0;

      /* acc1 +=  b[numTaps-1] * x[n-numTaps-5] */
      acc5 += x5 * c0;

      /* acc2 +=  b[numTaps-1] * x[n-numTaps-6] */
      acc6 += x6 * c0;

      /* acc3 +=  b[numTaps-1] * x[n-numTaps-7] */
      acc7 += x7 * c0;

      /* Read the b[numTaps-2] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-4] sample */
      x0 = *(px++);

      /* Perform the multiply-accumulate */
      acc0 += x1 * c0;
      acc1 += x2 * c0;
      acc2 += x3 * c0;
      acc3 += x4 * c0;
      acc4 += x5 * c0;
      acc5 += x6 * c0;
      acc6 += x7 * c0;
      acc7 += x0 * c0;

      /* Read the b[numTaps-3] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-5] sample */
      x1 = *(px++);

      /* Perform the multiply-accumulates */
      acc0 += x2 * c0;
      acc1 += x3 * c0;
      acc2 += x4 * c0;
      acc3 += x5 * c0;
      acc4 += x6 * c0;
      acc5 += x7 * c0;
      acc6 += x0 * c0;
      acc7 += x1 * c0;

      /* Read the b[numTaps-4] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-6] sample */
      x2 = *(px++);

      /* Perform the multiply-accumulates */
      acc0 += x3 * c0;
      acc1 += x4 * c0;
      acc2 += x5 * c0;
      acc3 += x6 * c0;
      acc4 += x7 * c0;
      acc5 += x0 * c0;
      acc6 += x1 * c0;
      acc7 += x2 * c0;

      /* Read the b[numTaps-4] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-6] sample */
      x3 = *(px++);
      /* Perform the multiply-accumulates */
      acc0 += x4 * c0;
      acc1 += x5 * c0;
      acc2 += x6 * c0;
      acc3 += x7 * c0;
      acc4 += x0 * c0;
      acc5 += x1 * c0;
      acc6 += x2 * c0;
      acc7 += x3 * c0;

      /* Read the b[numTaps-4] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-6] sample */
      x4 = *(px++);

      /* Perform the multiply-accumulates */
      acc0 += x5 * c0;
      acc1 += x6 * c0;
      acc2 += x7 * c0;
      acc3 += x0 * c0;
      acc4 += x1 * c0;
      acc5 += x2 * c0;
      acc6 += x3 * c0;
      acc7 += x4 * c0;

      /* Read the b[numTaps-4] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-6] sample */
      x5 = *(px++);

      /* Perform the multiply-accumulates */
      acc0 += x6 * c0;
      acc1 += x7 * c0;
      acc2 += x0 * c0;
      acc3 += x1 * c0;
      acc4 += x2 * c0;
      acc5 += x3 * c0;
      acc6 += x4 * c0;
      acc7 += x5 * c0;

      /* Read the b[numTaps-4] coefficient */
      c0 = *(pb++);

      /* Read x[n-numTaps-6] sample */
      x6 = *(px++);

      /* Perform the multiply-accumulates */
      acc0 += x7 * c0;
      acc1 += x0 * c0;
      acc2 += x1 * c0;
      acc3 += x2 * c0;
      acc4 += x3 * c0;
      acc5 += x4 * c0;
      acc6 += x5 * c0;
      acc7 += x6 * c0;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* Loop unrolling: Compute remaining outputs */
    tapCnt = numTaps % 0x8U;

    while (tapCnt > 0U)
    {
      /* Read coefficients */
      c0 = *(pb++);

      /* Fetch 1 state variable */
      x7 = *(px++);

      /* Perform the multiply-accumulates */
      acc0 += x0 * c0;
      acc1 += x1 * c0;
      acc2 += x2 * c0;
      acc3 += x3 * c0;
      acc4 += x4 * c0;
      acc5 += x5 * c0;
      acc6 += x6 * c0;
      acc7 += x7 * c0;

      /* Reuse the present sample states for next sample */
      x0 = x1;
      x1 = x2;
      x2 = x3;
      x3 = x4;
      x4 = x5;
      x5 = x6;
      x6 = x7;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* Advance the state pointer by 8 to process the next group of 8 samples */
    pState = pState + 8;

    /* The results in the 8 accumulators, store in the destination buffer. */
    *pDst++ = acc0;
    *pDst++ = acc1;
    *pDst++ = acc2;
    *pDst++ = acc3;
    *pDst++ = acc4;
    *pDst++ = acc5;
    *pDst++ = acc6;
    *pDst++ = acc7;


    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining output samples */
  blkCnt = blockSize % 0x8U;

#else

  /* Initialize blkCnt with number of taps */
  blkCnt = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* Copy one sample at a time into state buffer */
    *pStateCurnt++ = *pSrc++;

    /* Set the accumulator to zero */
    acc0 = 0.0f;

    /* Initialize state pointer */
    px = pState;

    /* Initialize Coefficient pointer */
    pb = pCoeffs;

    i = numTaps;

    /* Perform the multiply-accumulates */
    while (i > 0U)
    {
      /* acc =  b[numTaps-1] * x[n-numTaps-1] + b[numTaps-2] * x[n-numTaps-2] + b[numTaps-3] * x[n-numTaps-3] +...+ b[0] * x[0] */
      acc0 += *px++ * *pb++;

      i--;
    }

    /* Store result in destination buffer. */
    *pDst++ = acc0;

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

    /* Decrement loop counter */
    tapCnt--;
  }

}

#endif /* #if defined(ARM_MATH_NEON) */
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
* @} end of FIR group
*/
