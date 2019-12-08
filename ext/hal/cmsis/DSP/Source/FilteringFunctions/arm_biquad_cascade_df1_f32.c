/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_biquad_cascade_df1_f32.c
 * Description:  Processing function for the floating-point Biquad cascade DirectFormI(DF1) filter
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
  @defgroup BiquadCascadeDF1 Biquad Cascade IIR Filters Using Direct Form I Structure

  This set of functions implements arbitrary order recursive (IIR) filters.
  The filters are implemented as a cascade of second order Biquad sections.
  The functions support Q15, Q31 and floating-point data types.
  Fast version of Q15 and Q31 also available.

  The functions operate on blocks of input and output data and each call to the function
  processes <code>blockSize</code> samples through the filter.
  <code>pSrc</code> points to the array of input data and
  <code>pDst</code> points to the array of output data.
  Both arrays contain <code>blockSize</code> values.

  @par           Algorithm
                   Each Biquad stage implements a second order filter using the difference equation:
  <pre>
      y[n] = b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2]
  </pre>
                  A Direct Form I algorithm is used with 5 coefficients and 4 state variables per stage.
                  \image html Biquad.gif "Single Biquad filter stage"
                  Coefficients <code>b0, b1 and b2 </code> multiply the input signal <code>x[n]</code> and are referred to as the feedforward coefficients.
                  Coefficients <code>a1</code> and <code>a2</code> multiply the output signal <code>y[n]</code> and are referred to as the feedback coefficients.
                  Pay careful attention to the sign of the feedback coefficients.
                  Some design tools use the difference equation
  <pre>
      y[n] = b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] - a1 * y[n-1] - a2 * y[n-2]
  </pre>
                  In this case the feedback coefficients <code>a1</code> and <code>a2</code>
                  must be negated when used with the CMSIS DSP Library.

  @par
                   Higher order filters are realized as a cascade of second order sections.
                   <code>numStages</code> refers to the number of second order stages used.
                   For example, an 8th order filter would be realized with <code>numStages=4</code> second order stages.
                   \image html BiquadCascade.gif "8th order filter using a cascade of Biquad stages"
                   A 9th order filter would be realized with <code>numStages=5</code> second order stages with the coefficients for one of the stages configured as a first order filter (<code>b2=0</code> and <code>a2=0</code>).

  @par
                   The <code>pState</code> points to state variables array.
                   Each Biquad stage has 4 state variables <code>x[n-1], x[n-2], y[n-1],</code> and <code>y[n-2]</code>.
                   The state variables are arranged in the <code>pState</code> array as:
  <pre>
      {x[n-1], x[n-2], y[n-1], y[n-2]}
  </pre>

  @par
                   The 4 state variables for stage 1 are first, then the 4 state variables for stage 2, and so on.
                   The state array has a total length of <code>4*numStages</code> values.
                   The state variables are updated after each block of data is processed, the coefficients are untouched.

  @par           Instance Structure
                   The coefficients and state variables for a filter are stored together in an instance data structure.
                   A separate instance structure must be defined for each filter.
                   Coefficient arrays may be shared among several instances while state variable arrays cannot be shared.
                   There are separate instance structure declarations for each of the 3 supported data types.

  @par           Init Function
                   There is also an associated initialization function for each data type.
                   The initialization function performs following operations:
                   - Sets the values of the internal structure fields.
                   - Zeros out the values in the state buffer.
                   To do this manually without calling the init function, assign the follow subfields of the instance structure:
                   numStages, pCoeffs, pState. Also set all of the values in pState to zero.

  @par
                   Use of the initialization function is optional.
                   However, if the initialization function is used, then the instance structure cannot be placed into a const data section.
                   To place an instance structure into a const data section, the instance structure must be manually initialized.
                   Set the values in the state buffer to zeros before static initialization.
                   The code below statically initializes each of the 3 different data type filter instance structures
  <pre>
      arm_biquad_casd_df1_inst_f32 S1 = {numStages, pState, pCoeffs};
      arm_biquad_casd_df1_inst_q15 S2 = {numStages, pState, pCoeffs, postShift};
      arm_biquad_casd_df1_inst_q31 S3 = {numStages, pState, pCoeffs, postShift};
  </pre>
                   where <code>numStages</code> is the number of Biquad stages in the filter;
                   <code>pState</code> is the address of the state buffer;
                   <code>pCoeffs</code> is the address of the coefficient buffer;
                   <code>postShift</code> shift to be applied.

  @par           Fixed-Point Behavior
                   Care must be taken when using the Q15 and Q31 versions of the Biquad Cascade filter functions.
                   Following issues must be considered:
                   - Scaling of coefficients
                   - Filter gain
                   - Overflow and saturation

  @par           Scaling of coefficients
                   Filter coefficients are represented as fractional values and
                   coefficients are restricted to lie in the range <code>[-1 +1)</code>.
                   The fixed-point functions have an additional scaling parameter <code>postShift</code>
                   which allow the filter coefficients to exceed the range <code>[+1 -1)</code>.
                   At the output of the filter's accumulator is a shift register which shifts the result by <code>postShift</code> bits.
                   \image html BiquadPostshift.gif "Fixed-point Biquad with shift by postShift bits after accumulator"
                   This essentially scales the filter coefficients by <code>2^postShift</code>.
                   For example, to realize the coefficients
  <pre>
     {1.5, -0.8, 1.2, 1.6, -0.9}
  </pre>
                   set the pCoeffs array to:
  <pre>
     {0.75, -0.4, 0.6, 0.8, -0.45}
  </pre>
                   and set <code>postShift=1</code>

  @par           Filter gain
                   The frequency response of a Biquad filter is a function of its coefficients.
                   It is possible for the gain through the filter to exceed 1.0 meaning that the filter increases the amplitude of certain frequencies.
                   This means that an input signal with amplitude < 1.0 may result in an output > 1.0 and these are saturated or overflowed based on the implementation of the filter.
                   To avoid this behavior the filter needs to be scaled down such that its peak gain < 1.0 or the input signal must be scaled down so that the combination of input and filter are never overflowed.

  @par           Overflow and saturation
                   For Q15 and Q31 versions, it is described separately as part of the function specific documentation below.
 */

/**
  @addtogroup BiquadCascadeDF1
  @{
 */

/**
  @brief         Processing function for the floating-point Biquad cascade filter.
  @param[in]     S         points to an instance of the floating-point Biquad cascade structure
  @param[in]     pSrc      points to the block of input data
  @param[out]    pDst      points to the block of output data
  @param[in]     blockSize  number of samples to process
  @return        none
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
#include "arm_helium_utils.h"
void arm_biquad_cascade_df1_f32(
  const arm_biquad_casd_df1_inst_f32 * S,
  const float32_t * pSrc,
  float32_t * pDst,
  uint32_t blockSize)
{
    const float32_t *pIn = pSrc;              /*  source pointer            */
    float32_t *pOut = pDst;             /*  destination pointer       */
    float32_t *pState = S->pState;      /*  pState pointer            */
    const float32_t *pCoeffs = S->pCoeffs;    /*  coefficient pointer       */
    float32_t Xn1, Xn2, Yn1, Yn2;       /*  Filter pState variables   */
    float32_t lastX, lastY;             /*  X,Y history for tail handling */
    float32_t X0, X1, X2, X3;           /*  temporary input           */
    f32x4_t coeffs;
    f32x4_t accVec;                   /* accumultor vector */
    uint32_t  sample, stage = S->numStages; /*  loop counters             */

    do
    {
        /*
         * Reading the pState values
         */
        Xn1 = pState[0];
        Xn2 = pState[1];
        Yn1 = pState[2];
        Yn2 = pState[3];

        sample = blockSize >> 2U;

        /*
         * First part of the processing with loop unrolling.  Compute 4 outputs at a time.
         * second loop below computes the remaining 1 to 3 samples.
         */
        while (sample > 0U)
        {
            X0 = *pIn++;
            X1 = *pIn++;
            X2 = *pIn++;
            X3 = *pIn++;

            coeffs = vld1q(pCoeffs);
            accVec = vmulq(coeffs, X3);

            coeffs = vld1q(&pCoeffs[4]);
            accVec = vfmaq(accVec, coeffs, X2);

            coeffs = vld1q(&pCoeffs[8]);
            accVec = vfmaq(accVec, coeffs, X1);

            coeffs = vld1q(&pCoeffs[12]);
            accVec = vfmaq(accVec, coeffs, X0);

            coeffs = vld1q(&pCoeffs[16]);
            accVec = vfmaq(accVec, coeffs, Xn1);

            coeffs = vld1q(&pCoeffs[20]);
            accVec = vfmaq(accVec, coeffs, Xn2);

            coeffs = vld1q(&pCoeffs[24]);
            accVec = vfmaq(accVec, coeffs, Yn1);

            coeffs = vld1q(&pCoeffs[28]);
            accVec = vfmaq(accVec, coeffs, Yn2);
            /*
             * Store the result in the accumulator in the destination buffer.
             */
            vst1q(pOut, accVec);
            pOut += 4;

            /*
             * update recurrence
             */
            Xn1 = X3;
            Xn2 = X2;
            Yn1 = vgetq_lane(accVec, 3);
            Yn2 = vgetq_lane(accVec, 2);
            /*
             * decrement the loop counter
             */
            sample--;
        }

        /*
         * If the blockSize is not a multiple of 4,
         * compute any remaining output samples here.
         */
        sample = blockSize & 0x3U;
        if (sample)
        {
            /* save previous X, Y for modulo 1 length case */
            lastX = X3;
            lastY = Yn1;

            X0 = *pIn++;
            X1 = *pIn++;
            X2 = *pIn++;
            X3 = *pIn++;

            coeffs = vld1q(pCoeffs);
            accVec = vmulq(coeffs, X3);

            coeffs = vld1q(&pCoeffs[4]);
            accVec = vfmaq(accVec, coeffs, X2);

            coeffs = vld1q(&pCoeffs[8]);
            accVec = vfmaq(accVec, coeffs, X1);

            coeffs = vld1q(&pCoeffs[12]);
            accVec = vfmaq(accVec, coeffs, X0);

            coeffs = vld1q(&pCoeffs[16]);
            accVec = vfmaq(accVec, coeffs, Xn1);

            coeffs = vld1q(&pCoeffs[20]);
            accVec = vfmaq(accVec, coeffs, Xn2);

            coeffs = vld1q(&pCoeffs[24]);
            accVec = vfmaq(accVec, coeffs, Yn1);

            coeffs = vld1q(&pCoeffs[28]);
            accVec = vfmaq(accVec, coeffs, Yn2);

            if (sample == 1)
            {
                *pOut++ = vgetq_lane(accVec, 0);
                Xn1 = X0;
                Xn2 = lastX;
                Yn1 = vgetq_lane(accVec, 0);
                Yn2 = lastY;
            }
            else if (sample == 2)
            {
                *pOut++ = vgetq_lane(accVec, 0);
                *pOut++ = vgetq_lane(accVec, 1);
                Xn1 = X1;
                Xn2 = X0;
                Yn1 = vgetq_lane(accVec, 1);
                Yn2 = vgetq_lane(accVec, 0);
            }
            else
            {
                *pOut++ = vgetq_lane(accVec, 0);
                *pOut++ = vgetq_lane(accVec, 1);
                *pOut++ = vgetq_lane(accVec, 2);
                Xn1 = X2;
                Xn2 = X1;
                Yn1 = vgetq_lane(accVec, 2);
                Yn2 = vgetq_lane(accVec, 1);
            }
        }
        /*
         * Store the updated state variables back into the pState array
         */
        *pState++ = Xn1;
        *pState++ = Xn2;
        *pState++ = Yn1;
        *pState++ = Yn2;

        pCoeffs += sizeof(arm_biquad_mod_coef_f32) / sizeof(float32_t);
        /*
         * The first stage goes from the input buffer to the output buffer.
         * Subsequent numStages  occur in-place in the output buffer
         */
        pIn = pDst;
        /*
         * Reset the output pointer
         */
        pOut = pDst;
        /*
         * decrement the loop counter
         */
        stage--;
    }
    while (stage > 0U);
}
#else
#if defined(ARM_MATH_NEON)  && !defined(ARM_MATH_AUTOVECTORIZE)
void arm_biquad_cascade_df1_f32(
  const arm_biquad_casd_df1_inst_f32 * S,
  const float32_t * pSrc,
  float32_t * pDst,
  uint32_t blockSize)
{

  const float32_t *pIn = pSrc;                         /*  source pointer            */
  float32_t *pOut = pDst;                        /*  destination pointer       */
  float32_t *pState = S->pState;                 /*  pState pointer            */
  const float32_t *pCoeffs = S->pCoeffs;               /*  coefficient pointer       */
  float32_t acc;                                 /*  Simulates the accumulator */
  
  uint32_t sample, stage = S->numStages;         /*  loop counters             */

  float32x4_t Xn;
  float32x2_t Yn;
  float32x2_t a;
  float32x4_t b;
  
  float32x4_t x,tmp;
  float32x2_t t;
  float32x2x2_t y;

  float32_t Xns;

  while (stage > 0U)
  {
    /* Reading the coefficients */
    Xn = vdupq_n_f32(0.0f);

    Xn = vsetq_lane_f32(pState[0],Xn,2);
    Xn = vsetq_lane_f32(pState[1],Xn,3);
    Yn = vset_lane_f32(pState[2],Yn,0);
    Yn = vset_lane_f32(pState[3],Yn,1);
  
    b = vld1q_f32(pCoeffs);
    b = vrev64q_f32(b);  
    b = vcombine_f32(vget_high_f32(b), vget_low_f32(b));

    a = vld1_f32(pCoeffs + 3);
    a = vrev64_f32(a);
    b = vsetq_lane_f32(0.0f,b,0);
    pCoeffs += 5;
    
    /* Reading the pState values */
   
    /* Apply loop unrolling and compute 4 output values simultaneously. */
    /*      The variable acc hold output values that are being computed:
     *
     *    acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2]
     *    acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2]
     *    acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2]
     *    acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2]
     */

    /* First part of the processing with loop unrolling.  Compute 4 outputs at a time.
     ** a second loop below computes the remaining 1 to 3 samples. */
    sample = blockSize >> 2U;

    while (sample > 0U)
    {
      /* Read the first 4 inputs */
      x = vld1q_f32(pIn);

      pIn += 4;

      tmp = vextq_f32(Xn, x, 1);
      t = vmul_f32(vget_high_f32(b), vget_high_f32(tmp));
      t = vmla_f32(t, vget_low_f32(b), vget_low_f32(tmp));
      t = vmla_f32(t, a, Yn);
      t = vpadd_f32(t, t);
      Yn = vext_f32(Yn, t, 1);

      tmp = vextq_f32(Xn, x, 2);
      t = vmul_f32(vget_high_f32(b), vget_high_f32(tmp));
      t = vmla_f32(t, vget_low_f32(b), vget_low_f32(tmp));
      t = vmla_f32(t, a, Yn);
      t = vpadd_f32(t, t);
      Yn = vext_f32(Yn, t, 1);

      y.val[0] = Yn;

      tmp = vextq_f32(Xn, x, 3);
      t = vmul_f32(vget_high_f32(b), vget_high_f32(tmp));
      t = vmla_f32(t, vget_low_f32(b), vget_low_f32(tmp));
      t = vmla_f32(t, a, Yn);
      t = vpadd_f32(t, t);
      Yn = vext_f32(Yn, t, 1);

      Xn = x;
      t = vmul_f32(vget_high_f32(b), vget_high_f32(Xn));
      t = vmla_f32(t, vget_low_f32(b), vget_low_f32(Xn));
      t = vmla_f32(t, a, Yn);
      t = vpadd_f32(t, t);
      Yn = vext_f32(Yn, t, 1);
      
      y.val[1] = Yn;

      tmp = vcombine_f32(y.val[0], y.val[1]);

      /* Store the 4 outputs and increment the pointer */
      vst1q_f32(pOut, tmp);
      pOut += 4;

      /* Decrement the loop counter */
      sample--;
    }

    
    /* If the block size is not a multiple of 4, compute any remaining output samples here.
     ** No loop unrolling is used. */
    sample = blockSize & 0x3U;

    while (sample > 0U)
    {
      /* Read the input */
      Xns = *pIn++;

      /* acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2] */
      acc =  (vgetq_lane_f32(b, 1) * vgetq_lane_f32(Xn, 2)) 
      + (vgetq_lane_f32(b, 2) * vgetq_lane_f32(Xn, 3)) 
      + (vgetq_lane_f32(b, 3) * Xns) 
      + (vget_lane_f32(a, 0) * vget_lane_f32(Yn, 0)) 
      + (vget_lane_f32(a, 1) * vget_lane_f32(Yn, 1));

      /* Store the result in the accumulator in the destination buffer. */
      *pOut++ = acc;

      /* Every time after the output is computed state should be updated. */
      /* The states should be updated as:    */
      /* Xn2 = Xn1   */
      /* Xn1 = Xn    */
      /* Yn2 = Yn1   */
      /* Yn1 = acc   */
      Xn = vsetq_lane_f32(vgetq_lane_f32(Xn, 3),Xn,2);
      Xn = vsetq_lane_f32(Xns,Xn,3);
      Yn = vset_lane_f32(vget_lane_f32(Yn, 1),Yn,0);
      Yn = vset_lane_f32(acc,Yn,1);

      /* Decrement the loop counter */
      sample--;

    }

    vst1q_f32(pState,vcombine_f32((vget_high_f32(Xn)),(Yn)));
    pState += 4;
    /*  Store the updated state variables back into the pState array */
   
    /*  The first stage goes from the input buffer to the output buffer. */
    /*  Subsequent numStages  occur in-place in the output buffer */
    pIn = pDst;

    /* Reset the output pointer */
    pOut = pDst;

    /* Decrement the loop counter */
    stage--;
  }
}

#else
void arm_biquad_cascade_df1_f32(
  const arm_biquad_casd_df1_inst_f32 * S,
  const float32_t * pSrc,
        float32_t * pDst,
        uint32_t blockSize)
{
  const float32_t *pIn = pSrc;                         /* Source pointer */
        float32_t *pOut = pDst;                        /* Destination pointer */
        float32_t *pState = S->pState;                 /* pState pointer */
  const float32_t *pCoeffs = S->pCoeffs;               /* Coefficient pointer */
        float32_t acc;                                 /* Accumulator */
        float32_t b0, b1, b2, a1, a2;                  /* Filter coefficients */
        float32_t Xn1, Xn2, Yn1, Yn2;                  /* Filter pState variables */
        float32_t Xn;                                  /* Temporary input */
        uint32_t sample, stage = S->numStages;         /* Loop counters */

  do
  {
    /* Reading the coefficients */
    b0 = *pCoeffs++;
    b1 = *pCoeffs++;
    b2 = *pCoeffs++;
    a1 = *pCoeffs++;
    a2 = *pCoeffs++;

    /* Reading the pState values */
    Xn1 = pState[0];
    Xn2 = pState[1];
    Yn1 = pState[2];
    Yn2 = pState[3];

#if defined (ARM_MATH_LOOPUNROLL) && !defined(ARM_MATH_AUTOVECTORIZE)

    /* Apply loop unrolling and compute 4 output values simultaneously. */
    /* Variable acc hold output values that are being computed:
     *
     * acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2]
     * acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2]
     * acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2]
     * acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2]
     */

    /* Loop unrolling: Compute 4 outputs at a time */
    sample = blockSize >> 2U;

    while (sample > 0U)
    {
      /* Read the first input */
      Xn = *pIn++;

      /* acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2] */
      Yn2 = (b0 * Xn) + (b1 * Xn1) + (b2 * Xn2) + (a1 * Yn1) + (a2 * Yn2);

      /* Store output in destination buffer. */
      *pOut++ = Yn2;

      /* Every time after the output is computed state should be updated. */
      /* The states should be updated as: */
      /* Xn2 = Xn1 */
      /* Xn1 = Xn  */
      /* Yn2 = Yn1 */
      /* Yn1 = acc */

      /* Read the second input */
      Xn2 = *pIn++;

      /* acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2] */
      Yn1 = (b0 * Xn2) + (b1 * Xn) + (b2 * Xn1) + (a1 * Yn2) + (a2 * Yn1);

      /* Store output in destination buffer. */
      *pOut++ = Yn1;

      /* Every time after the output is computed state should be updated. */
      /* The states should be updated as: */
      /* Xn2 = Xn1 */
      /* Xn1 = Xn  */
      /* Yn2 = Yn1 */
      /* Yn1 = acc */

      /* Read the third input */
      Xn1 = *pIn++;

      /* acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2] */
      Yn2 = (b0 * Xn1) + (b1 * Xn2) + (b2 * Xn) + (a1 * Yn1) + (a2 * Yn2);

      /* Store output in destination buffer. */
      *pOut++ = Yn2;

      /* Every time after the output is computed state should be updated. */
      /* The states should be updated as: */
      /* Xn2 = Xn1 */
      /* Xn1 = Xn  */
      /* Yn2 = Yn1 */
      /* Yn1 = acc */

      /* Read the forth input */
      Xn = *pIn++;

      /* acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2] */
      Yn1 = (b0 * Xn) + (b1 * Xn1) + (b2 * Xn2) + (a1 * Yn2) + (a2 * Yn1);

      /* Store output in destination buffer. */
      *pOut++ = Yn1;

      /* Every time after the output is computed state should be updated. */
      /* The states should be updated as: */
      /* Xn2 = Xn1 */
      /* Xn1 = Xn  */
      /* Yn2 = Yn1 */
      /* Yn1 = acc */
      Xn2 = Xn1;
      Xn1 = Xn;

      /* decrement loop counter */
      sample--;
    }

    /* Loop unrolling: Compute remaining outputs */
    sample = blockSize & 0x3U;

#else

    /* Initialize blkCnt with number of samples */
    sample = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

    while (sample > 0U)
    {
      /* Read the input */
      Xn = *pIn++;

      /* acc =  b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] + a1 * y[n-1] + a2 * y[n-2] */
      acc = (b0 * Xn) + (b1 * Xn1) + (b2 * Xn2) + (a1 * Yn1) + (a2 * Yn2);

      /* Store output in destination buffer. */
      *pOut++ = acc;

      /* Every time after the output is computed state should be updated. */
      /* The states should be updated as: */
      /* Xn2 = Xn1 */
      /* Xn1 = Xn  */
      /* Yn2 = Yn1 */
      /* Yn1 = acc */
      Xn2 = Xn1;
      Xn1 = Xn;
      Yn2 = Yn1;
      Yn1 = acc;

      /* decrement loop counter */
      sample--;
    }

    /* Store the updated state variables back into the pState array */
    *pState++ = Xn1;
    *pState++ = Xn2;
    *pState++ = Yn1;
    *pState++ = Yn2;

    /* The first stage goes from the input buffer to the output buffer. */
    /* Subsequent numStages occur in-place in the output buffer */
    pIn = pDst;

    /* Reset output pointer */
    pOut = pDst;

    /* decrement loop counter */
    stage--;

  } while (stage > 0U);

}

#endif /* #if defined(ARM_MATH_NEON) */
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of BiquadCascadeDF1 group
 */
