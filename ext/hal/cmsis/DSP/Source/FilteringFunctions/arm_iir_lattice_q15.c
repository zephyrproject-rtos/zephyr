/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_iir_lattice_q15.c
 * Description:  Q15 IIR Lattice filter processing function
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
  @addtogroup IIR_Lattice
  @{
 */

/**
  @brief         Processing function for the Q15 IIR lattice filter.
  @param[in]     S          points to an instance of the Q15 IIR lattice structure
  @param[in]     pSrc       points to the block of input data
  @param[out]    pDst       points to the block of output data
  @param[in]     blockSize  number of samples to process
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using an internal 64-bit accumulator.
                   Both coefficients and state variables are represented in 1.15 format and multiplications yield a 2.30 result.
                   The 2.30 intermediate results are accumulated in a 64-bit accumulator in 34.30 format.
                   There is no risk of internal overflow with this approach and the full precision of intermediate multiplications is preserved.
                   After all additions have been performed, the accumulator is truncated to 34.15 format by discarding low 15 bits.
                   Lastly, the accumulator is saturated to yield a result in 1.15 format.
 */

void arm_iir_lattice_q15(
  const arm_iir_lattice_instance_q15 * S,
  const q15_t * pSrc,
        q15_t * pDst,
        uint32_t blockSize)
{
        q15_t *pState = S->pState;                     /* State pointer */
        q15_t *pStateCur;                              /* State current pointer */
        q31_t fcurr, fnext = 0, gcurr = 0, gnext;      /* Temporary variables for lattice stages */
        q63_t acc;                                     /* Accumlator */
        q15_t *px1, *px2, *pk, *pv;                    /* Temporary pointers for state and coef */
        uint32_t numStages = S->numStages;             /* Number of stages */
        uint32_t blkCnt, tapCnt;                       /* Temporary variables for counts */
        q15_t out;                                     /* Temporary variable for output */

#if defined (ARM_MATH_DSP) && defined (ARM_MATH_LOOPUNROLL)
        q15_t gnext1, gnext2;                          /* Temporary variables for lattice stages */
        q31_t v;                                       /* Temporary variable for ladder coefficient */
#endif

  /* initialise loop count */
  blkCnt = blockSize;

#if defined (ARM_MATH_DSP)

  /* Sample processing */
  while (blkCnt > 0U)
  {
    /* Read Sample from input buffer */
    /* fN(n) = x(n) */
    fcurr = *pSrc++;

    /* Initialize Ladder coeff pointer */
    pv = &S->pvCoeffs[0];

    /* Initialize Reflection coeff pointer */
    pk = &S->pkCoeffs[0];

    /* Initialize state read pointer */
    px1 = pState;

    /* Initialize state write pointer */
    px2 = pState;

    /* Set accumulator to zero */
    acc = 0;

    /* Process sample for first tap */
    gcurr = *px1++;
    /* fN-1(n) = fN(n) - kN * gN-1(n-1) */
    fnext = fcurr - (((q31_t) gcurr * (*pk)) >> 15);
    fnext = __SSAT(fnext, 16);

    /* gN(n) = kN * fN-1(n) + gN-1(n-1) */
    gnext = (((q31_t) fnext * (*pk++)) >> 15) + gcurr;
    gnext = __SSAT(gnext, 16);

    /* write gN(n) into state for next sample processing */
    *px2++ = (q15_t) gnext;

    /* y(n) += gN(n) * vN */
    acc += (q31_t) ((gnext * (*pv++)));

    /* Update f values for next coefficient processing */
    fcurr = fnext;


#if defined (ARM_MATH_LOOPUNROLL)

    /* Loop unrolling: Compute 4 taps at a time. */
    tapCnt = (numStages - 1U) >> 2U;

    while (tapCnt > 0U)
    {
      /* Process sample for 2nd, 6th ...taps */
      /* Read gN-2(n-1) from state buffer */
      gcurr = *px1++;
      /* fN-2(n) = fN-1(n) - kN-1 * gN-2(n-1) */
      fnext = fcurr - (((q31_t) gcurr * (*pk)) >> 15);
      fnext = __SSAT(fnext, 16);
      /* gN-1(n) = kN-1 * fN-2(n) + gN-2(n-1) */
      gnext = (((q31_t) fnext * (*pk++)) >> 15) + gcurr;
      gnext1 = (q15_t) __SSAT(gnext, 16);
      /* write gN-1(n) into state for next sample processing */
      *px2++ = (q15_t) gnext1;

      /* Process sample for 3nd, 7th ...taps */
      /* Read gN-3(n-1) from state buffer */
      gcurr = *px1++;
      /* Process sample for 3rd, 7th .. taps */
      /* fN-3(n) = fN-2(n) - kN-2 * gN-3(n-1) */
      fcurr = fnext - (((q31_t) gcurr * (*pk)) >> 15);
      fcurr = __SSAT(fcurr, 16);
      /* gN-2(n) = kN-2 * fN-3(n) + gN-3(n-1) */
      gnext = (((q31_t) fcurr * (*pk++)) >> 15) + gcurr;
      gnext2 = (q15_t) __SSAT(gnext, 16);
      /* write gN-2(n) into state */
      *px2++ = (q15_t) gnext2;

      /* Read vN-1 and vN-2 at a time */
      v = read_q15x2_ia (&pv);

      /* Pack gN-1(n) and gN-2(n) */

#ifndef  ARM_MATH_BIG_ENDIAN
      gnext = __PKHBT(gnext1, gnext2, 16);
#else
      gnext = __PKHBT(gnext2, gnext1, 16);
#endif /* #ifndef  ARM_MATH_BIG_ENDIAN */

      /* y(n) += gN-1(n) * vN-1  */
      /* process for gN-5(n) * vN-5, gN-9(n) * vN-9 ... */
      /* y(n) += gN-2(n) * vN-2  */
      /* process for gN-6(n) * vN-6, gN-10(n) * vN-10 ... */
      acc = __SMLALD(gnext, v, acc);

      /* Process sample for 4th, 8th ...taps */
      /* Read gN-4(n-1) from state buffer */
      gcurr = *px1++;
      /* Process sample for 4th, 8th .. taps */
      /* fN-4(n) = fN-3(n) - kN-3 * gN-4(n-1) */
      fnext = fcurr - (((q31_t) gcurr * (*pk)) >> 15);
      fnext = __SSAT(fnext, 16);
      /* gN-3(n) = kN-3 * fN-1(n) + gN-1(n-1) */
      gnext = (((q31_t) fnext * (*pk++)) >> 15) + gcurr;
      gnext1 = (q15_t) __SSAT(gnext, 16);
      /* write  gN-3(n) for the next sample process */
      *px2++ = (q15_t) gnext1;

      /* Process sample for 5th, 9th ...taps */
      /* Read gN-5(n-1) from state buffer */
      gcurr = *px1++;
      /* Process sample for 5th, 9th .. taps */
      /* fN-5(n) = fN-4(n) - kN-4 * gN-5(n-1) */
      fcurr = fnext - (((q31_t) gcurr * (*pk)) >> 15);
      fcurr = __SSAT(fcurr, 16);
      /* gN-4(n) = kN-4 * fN-5(n) + gN-5(n-1) */
      gnext = (((q31_t) fcurr * (*pk++)) >> 15) + gcurr;
      gnext2 = (q15_t) __SSAT(gnext, 16);
      /* write      gN-4(n) for the next sample process */
      *px2++ = (q15_t) gnext2;

      /* Read vN-3 and vN-4 at a time */
      v = read_q15x2_ia (&pv);

      /* Pack gN-3(n) and gN-4(n) */
#ifndef ARM_MATH_BIG_ENDIAN
      gnext = __PKHBT(gnext1, gnext2, 16);
#else
      gnext = __PKHBT(gnext2, gnext1, 16);
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

      /* y(n) += gN-4(n) * vN-4  */
      /* process for gN-8(n) * vN-8, gN-12(n) * vN-12 ... */
      /* y(n) += gN-3(n) * vN-3  */
      /* process for gN-7(n) * vN-7, gN-11(n) * vN-11 ... */
      acc = __SMLALD(gnext, v, acc);

      /* Decrement loop counter */
      tapCnt--;
    }

    fnext = fcurr;

    /* Loop unrolling: Compute remaining taps */
    tapCnt = (numStages - 1U) % 0x4U;

#else

    /* Initialize blkCnt with number of samples */
    tapCnt = (numStages - 1U);

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

    while (tapCnt > 0U)
    {
      gcurr = *px1++;
      /* Process sample for last taps */
      fnext = fcurr - (((q31_t) gcurr * (*pk)) >> 15);
      fnext = __SSAT(fnext, 16);
      gnext = (((q31_t) fnext * (*pk++)) >> 15) + gcurr;
      gnext = __SSAT(gnext, 16);

      /* Output samples for last taps */
      acc += (q31_t) (((q31_t) gnext * (*pv++)));
      *px2++ = (q15_t) gnext;
      fcurr = fnext;

      /* Decrement loop counter */
      tapCnt--;
    }

    /* y(n) += g0(n) * v0 */
    acc += (q31_t) (((q31_t) fnext * (*pv++)));

    out = (q15_t) __SSAT(acc >> 15, 16);
    *px2++ = (q15_t) fnext;

    /* write out into pDst */
    *pDst++ = out;

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

  /* copy data */
#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 taps at a time. */
  tapCnt = numStages >> 2U;

  while (tapCnt > 0U)
  {
    write_q15x2_ia (&pStateCur, read_q15x2_ia (&pState));
    write_q15x2_ia (&pStateCur, read_q15x2_ia (&pState));

    /* Decrement loop counter */
    tapCnt--;
  }

  /* Loop unrolling: Compute remaining taps */
  tapCnt = numStages % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  tapCnt = (numStages - 1U);

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (tapCnt > 0U)
  {
    *pStateCur++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

#else /* #if defined (ARM_MATH_DSP) */

  /* Sample processing */
  while (blkCnt > 0U)
  {
    /* Read Sample from input buffer */
    /* fN(n) = x(n) */
    fcurr = *pSrc++;

    /* Initialize Ladder coeff pointer */
    pv = &S->pvCoeffs[0];

    /* Initialize Reflection coeff pointer */
    pk = &S->pkCoeffs[0];

    /* Initialize state read pointer */
    px1 = pState;

    /* Initialize state write pointer */
    px2 = pState;

    /* Set accumulator to zero */
    acc = 0;

    tapCnt = numStages;

    while (tapCnt > 0U)
    {
      gcurr = *px1++;
      /* Process sample */
      /* fN-1(n) = fN(n) - kN * gN-1(n-1) */
      fnext = fcurr - ((gcurr * (*pk)) >> 15);
      fnext = __SSAT(fnext, 16);

      /* gN(n) = kN * fN-1(n) + gN-1(n-1) */
      gnext = ((fnext * (*pk++)) >> 15) + gcurr;
      gnext = __SSAT(gnext, 16);

      /* Output samples */
      /* y(n) += gN(n) * vN */
      acc += (q31_t) ((gnext * (*pv++)));

      /* write gN(n) into state for next sample processing */
      *px2++ = (q15_t) gnext;

      /* Update f values for next coefficient processing */
      fcurr = fnext;

      tapCnt--;
    }

    /* y(n) += g0(n) * v0 */
    acc += (q31_t) ((fnext * (*pv++)));

    out = (q15_t) __SSAT(acc >> 15, 16);
    *px2++ = (q15_t) fnext;

    /* write out into pDst */
    *pDst++ = out;

    /* Advance the state pointer by 1 to process the next group of samples */
    pState = pState + 1U;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Processing is complete. Now copy last S->numStages samples to start of the buffer
     for the preperation of next frame process */

  /* Points to the start of the state buffer */
  pStateCur = &S->pState[0];
  pState = &S->pState[blockSize];

  tapCnt = numStages;

  /* Copy data */
  while (tapCnt > 0U)
  {
    *pStateCur++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

#endif /* #if defined (ARM_MATH_DSP) */

}

/**
  @} end of IIR_Lattice group
 */
