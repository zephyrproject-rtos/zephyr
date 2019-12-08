/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_iir_lattice_f32.c
 * Description:  Floating-point IIR Lattice filter processing function
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
  @defgroup IIR_Lattice Infinite Impulse Response (IIR) Lattice Filters

  This set of functions implements lattice filters
  for Q15, Q31 and floating-point data types.  Lattice filters are used in a
  variety of adaptive filter applications. The filter structure has feedforward and
  feedback components and the net impulse response is infinite length.
  The functions operate on blocks
  of input and output data and each call to the function processes
  <code>blockSize</code> samples through the filter.  <code>pSrc</code> and
  <code>pDst</code> point to input and output arrays containing <code>blockSize</code> values.

  @par           Algorithm
                   \image html IIRLattice.gif "Infinite Impulse Response Lattice filter"
  @par
  <pre>
      fN(n)   = x(n)
      fm-1(n) = fm(n) - km * gm-1(n-1)   for m = N, N-1, ..., 1
      gm(n)   = km * fm-1(n) + gm-1(n-1) for m = N, N-1, ..., 1
      y(n)    = vN * gN(n) + vN-1 * gN-1(n) + ...+ v0 * g0(n)
  </pre>
  @par
                   <code>pkCoeffs</code> points to array of reflection coefficients of size <code>numStages</code>.
                   Reflection Coefficients are stored in time-reversed order.
  @par
  <pre>
     {kN, kN-1, ..., k1}
  </pre>
  @par
                  <code>pvCoeffs</code> points to the array of ladder coefficients of size <code>(numStages+1)</code>.
                  Ladder coefficients are stored in time-reversed order.
  <pre>
      {vN, vN-1, ..., v0}
  </pre>
  @par
                   <code>pState</code> points to a state array of size <code>numStages + blockSize</code>.
                   The state variables shown in the figure above (the g values) are stored in the <code>pState</code> array.
                   The state variables are updated after each block of data is processed; the coefficients are untouched.

  @par           Instance Structure
                   The coefficients and state variables for a filter are stored together in an instance data structure.
                   A separate instance structure must be defined for each filter.
                   Coefficient arrays may be shared among several instances while state variable arrays cannot be shared.
                   There are separate instance structure declarations for each of the 3 supported data types.

  @par           Initialization Functions
                   There is also an associated initialization function for each data type.
                   The initialization function performs the following operations:
                   - Sets the values of the internal structure fields.
                   - Zeros out the values in the state buffer.
                   To do this manually without calling the init function, assign the follow subfields of the instance structure:
                   numStages, pkCoeffs, pvCoeffs, pState. Also set all of the values in pState to zero.
  @par
                   Use of the initialization function is optional.
                   However, if the initialization function is used, then the instance structure cannot be placed into a const data section.
                   To place an instance structure into a const data section, the instance structure must be manually initialized.
                   Set the values in the state buffer to zeros and then manually initialize the instance structure as follows:
  <pre>
      arm_iir_lattice_instance_f32 S = {numStages, pState, pkCoeffs, pvCoeffs};
      arm_iir_lattice_instance_q31 S = {numStages, pState, pkCoeffs, pvCoeffs};
      arm_iir_lattice_instance_q15 S = {numStages, pState, pkCoeffs, pvCoeffs};
  </pre>
  @par
                   where <code>numStages</code> is the number of stages in the filter; <code>pState</code> points to the state buffer array;
                   <code>pkCoeffs</code> points to array of the reflection coefficients; <code>pvCoeffs</code> points to the array of ladder coefficients.

  @par           Fixed-Point Behavior
                   Care must be taken when using the fixed-point versions of the IIR lattice filter functions.
                   In particular, the overflow and saturation behavior of the accumulator used in each function must be considered.
                   Refer to the function specific documentation below for usage guidelines.
 */

/**
  @addtogroup IIR_Lattice
  @{
 */

/**
  @brief         Processing function for the floating-point IIR lattice filter.
  @param[in]     S          points to an instance of the floating-point IIR lattice structure
  @param[in]     pSrc       points to the block of input data
  @param[out]    pDst       points to the block of output data
  @param[in]     blockSize  number of samples to process
  @return        none
 */

void arm_iir_lattice_f32(
  const arm_iir_lattice_instance_f32 * S,
  const float32_t * pSrc,
        float32_t * pDst,
        uint32_t blockSize)
{       
        float32_t *pState = S->pState;                   /* State pointer */
        float32_t *pStateCur;                            /* State current pointer */
        float32_t acc;                                   /* Accumlator */
        float32_t fnext1, fnext2, gcurr1, gnext;         /* Temporary variables for lattice stages */
        float32_t *px1, *px2, *pk, *pv;                  /* Temporary pointers for state and coef */
        uint32_t numStages = S->numStages;               /* Number of stages */
        uint32_t blkCnt, tapCnt;                         /* Temporary variables for counts */

#if defined (ARM_MATH_LOOPUNROLL)
        float32_t gcurr2;                                /* Temporary variables for lattice stages */
        float32_t k1, k2;
        float32_t v1, v2, v3, v4;
#endif

  /* initialise loop count */
  blkCnt = blockSize;

  /* Sample processing */
  while (blkCnt > 0U)
  {
    /* Read Sample from input buffer */
    /* fN(n) = x(n) */
    fnext2 = *pSrc++;

    /* Initialize Ladder coeff pointer */
    pv = &S->pvCoeffs[0];

    /* Initialize Reflection coeff pointer */
    pk = &S->pkCoeffs[0];

    /* Initialize state read pointer */
    px1 = pState;

    /* Initialize state write pointer */
    px2 = pState;

    /* Set accumulator to zero */
    acc = 0.0;

#if defined (ARM_MATH_LOOPUNROLL)

    /* Loop unrolling: Compute 4 taps at a time. */
    tapCnt = (numStages) >> 2U;

    while (tapCnt > 0U)
    {
      /* Read gN-1(n-1) from state buffer */
      gcurr1 = *px1;

      /* read reflection coefficient kN */
      k1 = *pk;

      /* fN-1(n) = fN(n) - kN * gN-1(n-1) */
      fnext1 = fnext2 - (k1 * gcurr1);

      /* read ladder coefficient vN */
      v1 = *pv;

      /* read next reflection coefficient kN-1 */
      k2 = *(pk + 1U);

      /* Read gN-2(n-1) from state buffer */
      gcurr2 = *(px1 + 1U);

      /* read next ladder coefficient vN-1 */
      v2 = *(pv + 1U);

      /* fN-2(n) = fN-1(n) - kN-1 * gN-2(n-1) */
      fnext2 = fnext1 - (k2 * gcurr2);

      /* gN(n)   = kN * fN-1(n) + gN-1(n-1) */
      gnext = gcurr1 + (k1 * fnext1);

      /* read reflection coefficient kN-2 */
      k1 = *(pk + 2U);

      /* write gN(n) into state for next sample processing */
      *px2++ = gnext;

      /* Read gN-3(n-1) from state buffer */
      gcurr1 = *(px1 + 2U);

      /* y(n) += gN(n) * vN  */
      acc += (gnext * v1);

      /* fN-3(n) = fN-2(n) - kN-2 * gN-3(n-1) */
      fnext1 = fnext2 - (k1 * gcurr1);

      /* gN-1(n)   = kN-1 * fN-2(n) + gN-2(n-1) */
      gnext = gcurr2 + (k2 * fnext2);

      /* Read gN-4(n-1) from state buffer */
      gcurr2 = *(px1 + 3U);

      /* y(n) += gN-1(n) * vN-1  */
      acc += (gnext * v2);

      /* read reflection coefficient kN-3 */
      k2 = *(pk + 3U);

      /* write gN-1(n) into state for next sample processing */
      *px2++ = gnext;

      /* fN-4(n) = fN-3(n) - kN-3 * gN-4(n-1) */
      fnext2 = fnext1 - (k2 * gcurr2);

      /* gN-2(n) = kN-2 * fN-3(n) + gN-3(n-1) */
      gnext = gcurr1 + (k1 * fnext1);

      /* read ladder coefficient vN-2 */
      v3 = *(pv + 2U);

      /* y(n) += gN-2(n) * vN-2  */
      acc += (gnext * v3);

      /* write gN-2(n) into state for next sample processing */
      *px2++ = gnext;

      /* update pointer */
      pk += 4U;

      /* gN-3(n) = kN-3 * fN-4(n) + gN-4(n-1) */
      gnext = (fnext2 * k2) + gcurr2;

      /* read next ladder coefficient vN-3 */
      v4 = *(pv + 3U);

      /* y(n) += gN-4(n) * vN-4  */
      acc += (gnext * v4);

      /* write gN-3(n) into state for next sample processing */
      *px2++ = gnext;

      /* update pointers */
      px1 += 4U;
      pv += 4U;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* Loop unrolling: Compute remaining taps */
    tapCnt = numStages % 0x4U;

#else

    /* Initialize tapCnt with number of samples */
    tapCnt = numStages;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

    while (tapCnt > 0U)
    {
      gcurr1 = *px1++;
      /* Process sample for last taps */
      fnext1 = fnext2 - ((*pk) * gcurr1);
      gnext = (fnext1 * (*pk++)) + gcurr1;
      /* Output samples for last taps */
      acc += (gnext * (*pv++));
      *px2++ = gnext;
      fnext2 = fnext1;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* y(n) += g0(n) * v0 */
    acc += (fnext2 * (*pv));

    *px2++ = fnext2;

    /* write out into pDst */
    *pDst++ = acc;

    /* Advance the state pointer by 4 to process the next group of 4 samples */
    pState = pState + 1U;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Processing is complete. Now copy last S->numStages samples to start of the buffer
     for the preperation of next frame process */

  /* Points to the start of the state buffer */
  pStateCur = &S->pState[0];
  pState = &S->pState[blockSize];

  /* Copy data */
#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 taps at a time. */
  tapCnt = numStages >> 2U;

  while (tapCnt > 0U)
  {
    *pStateCur++ = *pState++;
    *pStateCur++ = *pState++;
    *pStateCur++ = *pState++;
    *pStateCur++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

  /* Loop unrolling: Compute remaining taps */
  tapCnt = numStages % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  tapCnt = numStages;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (tapCnt > 0U)
  {
    *pStateCur++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

}

/**
  @} end of IIR_Lattice group
 */
