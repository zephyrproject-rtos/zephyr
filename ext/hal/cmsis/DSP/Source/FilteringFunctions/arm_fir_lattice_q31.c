/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_fir_lattice_q31.c
 * Description:  Q31 FIR lattice filter processing function
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
  @addtogroup FIR_Lattice
  @{
 */

/**
  @brief         Processing function for the Q31 FIR lattice filter.
  @param[in]     S          points to an instance of the Q31 FIR lattice structure
  @param[in]     pSrc       points to the block of input data
  @param[out]    pDst       points to the block of output data
  @param[in]     blockSize  number of samples to process
  @return        none

  @par           Scaling and Overflow Behavior
                   In order to avoid overflows the input signal must be scaled down by 2*log2(numStages) bits.
 */

void arm_fir_lattice_q31(
  const arm_fir_lattice_instance_q31 * S,
  const q31_t * pSrc,
        q31_t * pDst,
        uint32_t blockSize)
{
        q31_t *pState = S->pState;                     /* State pointer */
  const q31_t *pCoeffs = S->pCoeffs;                   /* Coefficient pointer */
        q31_t *px;                                     /* Temporary state pointer */
  const q31_t *pk;                                     /* Temporary coefficient pointer */
        uint32_t numStages = S->numStages;             /* Number of stages in the filter */
        uint32_t blkCnt, stageCnt;                     /* Loop counters */
        q31_t fcurr0, fnext0, gnext0, gcurr0;          /* Temporary variables */

#if (1)
//#if !defined(ARM_MATH_CM0_FAMILY)

#if defined (ARM_MATH_LOOPUNROLL)
        q31_t fcurr1, fnext1, gnext1;                  /* Temporary variables for second sample in loop unrolling */
        q31_t fcurr2, fnext2, gnext2;                  /* Temporary variables for third sample in loop unrolling */
        q31_t fcurr3, fnext3, gnext3;                  /* Temporary variables for fourth sample in loop unrolling */
#endif

  gcurr0 = 0;

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* Read two samples from input buffer */
    /* f0(n) = x(n) */
    fcurr0 = *pSrc++;
    fcurr1 = *pSrc++;

    /* Initialize state pointer */
    px = pState;

    /* Initialize coeff pointer */
    pk = pCoeffs;

    /* Read g0(n-1) from state buffer */
    gcurr0 = *px;

    /* Process first sample for first tap */
    /* f1(n) = f0(n) +  K1 * g0(n-1) */
    fnext0 = (q31_t) (((q63_t) gcurr0 * (*pk)) >> 32U);
    fnext0 = (fnext0 << 1U) + fcurr0;

    /* g1(n) = f0(n) * K1  +  g0(n-1) */
    gnext0 = (q31_t) (((q63_t) fcurr0 * (*pk)) >> 32U);
    gnext0 = (gnext0 << 1U) + gcurr0;

    /* Process second sample for first tap */
    fnext1 = (q31_t) (((q63_t) fcurr0 * (*pk)) >> 32U);
    fnext1 = (fnext1 << 1U) + fcurr1;
    gnext1 = (q31_t) (((q63_t) fcurr1 * (*pk)) >> 32U);
    gnext1 = (gnext1 << 1U) + fcurr0;

    /* Read next two samples from input buffer */
    /* f0(n+2) = x(n+2) */
    fcurr2 = *pSrc++;
    fcurr3 = *pSrc++;

    /* Process third sample for first tap */
    fnext2 = (q31_t) (((q63_t) fcurr1 * (*pk)) >> 32U);
    fnext2 = (fnext2 << 1U) + fcurr2;
    gnext2 = (q31_t) (((q63_t) fcurr2 * (*pk)) >> 32U);
    gnext2 = (gnext2 << 1U) + fcurr1;

    /* Process fourth sample for first tap */
    fnext3 = (q31_t) (((q63_t) fcurr2 * (*pk  )) >> 32U);
    fnext3 = (fnext3 << 1U) + fcurr3;
    gnext3 = (q31_t) (((q63_t) fcurr3 * (*pk++)) >> 32U);
    gnext3 = (gnext3 << 1U) + fcurr2;

    /* Copy only last input sample into the state buffer
       which will be used for next samples processing */
    *px++ = fcurr3;

    /* Update of f values for next coefficient set processing */
    fcurr0 = fnext0;
    fcurr1 = fnext1;
    fcurr2 = fnext2;
    fcurr3 = fnext3;

    /* Loop unrolling.  Process 4 taps at a time . */
    stageCnt = (numStages - 1U) >> 2U;

    /* Loop over the number of taps.  Unroll by a factor of 4.
       Repeat until we've computed numStages-3 coefficients. */

    /* Process 2nd, 3rd, 4th and 5th taps ... here */
    while (stageCnt > 0U)
    {
      /* Read g1(n-1), g3(n-1) .... from state */
      gcurr0 = *px;

      /* save g1(n) in state buffer */
      *px++ = gnext3;

      /* Process first sample for 2nd, 6th .. tap */
      /* Sample processing for K2, K6.... */
      /* f1(n) = f0(n) +  K1 * g0(n-1) */
      fnext0 = (q31_t) (((q63_t) gcurr0 * (*pk)) >> 32U);
      fnext0 = (fnext0 << 1U) + fcurr0;

      /* Process second sample for 2nd, 6th .. tap */
      /* for sample 2 processing */
      fnext1 = (q31_t) (((q63_t) gnext0 * (*pk)) >> 32U);
      fnext1 = (fnext1 << 1U) + fcurr1;

      /* Process third sample for 2nd, 6th .. tap */
      fnext2 = (q31_t) (((q63_t) gnext1 * (*pk)) >> 32U);
      fnext2 = (fnext2 << 1U) + fcurr2;

      /* Process fourth sample for 2nd, 6th .. tap */
      fnext3 = (q31_t) (((q63_t) gnext2 * (*pk)) >> 32U);
      fnext3 = (fnext3 << 1U) + fcurr3;

      /* g1(n) = f0(n) * K1  +  g0(n-1) */
      /* Calculation of state values for next stage */
      gnext3 = (q31_t) (((q63_t) fcurr3 * (*pk)) >> 32U);
      gnext3 = (gnext3 << 1U) + gnext2;

      gnext2 = (q31_t) (((q63_t) fcurr2 * (*pk)) >> 32U);
      gnext2 = (gnext2 << 1U) + gnext1;

      gnext1 = (q31_t) (((q63_t) fcurr1 * (*pk)) >> 32U);
      gnext1 = (gnext1 << 1U) + gnext0;

      gnext0 = (q31_t) (((q63_t) fcurr0 * (*pk++)) >> 32U);
      gnext0 = (gnext0 << 1U) + gcurr0;


      /* Read g2(n-1), g4(n-1) .... from state */
      gcurr0 = *px;

      /* save g1(n) in state buffer */
      *px++ = gnext3;

      /* Sample processing for K3, K7.... */
      /* Process first sample for 3rd, 7th .. tap */
      /* f3(n) = f2(n) +  K3 * g2(n-1) */
      fcurr0 = (q31_t) (((q63_t) gcurr0 * (*pk)) >> 32U);
      fcurr0 = (fcurr0 << 1U) + fnext0;

      /* Process second sample for 3rd, 7th .. tap */
      fcurr1 = (q31_t) (((q63_t) gnext0 * (*pk)) >> 32U);
      fcurr1 = (fcurr1 << 1U) + fnext1;

      /* Process third sample for 3rd, 7th .. tap */
      fcurr2 = (q31_t) (((q63_t) gnext1 * (*pk)) >> 32U);
      fcurr2 = (fcurr2 << 1U) + fnext2;

      /* Process fourth sample for 3rd, 7th .. tap */
      fcurr3 = (q31_t) (((q63_t) gnext2 * (*pk)) >> 32U);
      fcurr3 = (fcurr3 << 1U) + fnext3;

      /* Calculation of state values for next stage */
      /* g3(n) = f2(n) * K3  +  g2(n-1) */
      gnext3 = (q31_t) (((q63_t) fnext3 * (*pk)) >> 32U);
      gnext3 = (gnext3 << 1U) + gnext2;

      gnext2 = (q31_t) (((q63_t) fnext2 * (*pk)) >> 32U);
      gnext2 = (gnext2 << 1U) + gnext1;

      gnext1 = (q31_t) (((q63_t) fnext1 * (*pk)) >> 32U);
      gnext1 = (gnext1 << 1U) + gnext0;

      gnext0 = (q31_t) (((q63_t) fnext0 * (*pk++)) >> 32U);
      gnext0 = (gnext0 << 1U) + gcurr0;

      /* Read g1(n-1), g3(n-1) .... from state */
      gcurr0 = *px;

      /* save g1(n) in state buffer */
      *px++ = gnext3;

      /* Sample processing for K4, K8.... */
      /* Process first sample for 4th, 8th .. tap */
      /* f4(n) = f3(n) +  K4 * g3(n-1) */
      fnext0 = (q31_t) (((q63_t) gcurr0 * (*pk)) >> 32U);
      fnext0 = (fnext0 << 1U) + fcurr0;

      /* Process second sample for 4th, 8th .. tap */
      /* for sample 2 processing */
      fnext1 = (q31_t) (((q63_t) gnext0 * (*pk)) >> 32U);
      fnext1 = (fnext1 << 1U) + fcurr1;

      /* Process third sample for 4th, 8th .. tap */
      fnext2 = (q31_t) (((q63_t) gnext1 * (*pk)) >> 32U);
      fnext2 = (fnext2 << 1U) + fcurr2;

      /* Process fourth sample for 4th, 8th .. tap */
      fnext3 = (q31_t) (((q63_t) gnext2 * (*pk)) >> 32U);
      fnext3 = (fnext3 << 1U) + fcurr3;

      /* g4(n) = f3(n) * K4  +  g3(n-1) */
      /* Calculation of state values for next stage */
      gnext3 = (q31_t) (((q63_t) fcurr3 * (*pk)) >> 32U);
      gnext3 = (gnext3 << 1U) + gnext2;

      gnext2 = (q31_t) (((q63_t) fcurr2 * (*pk)) >> 32U);
      gnext2 = (gnext2 << 1U) + gnext1;

      gnext1 = (q31_t) (((q63_t) fcurr1 * (*pk)) >> 32U);
      gnext1 = (gnext1 << 1U) + gnext0;

      gnext0 = (q31_t) (((q63_t) fcurr0 * (*pk++)) >> 32U);
      gnext0 = (gnext0 << 1U) + gcurr0;

      /* Read g2(n-1), g4(n-1) .... from state */
      gcurr0 = *px;

      /* save g4(n) in state buffer */
      *px++ = gnext3;

      /* Sample processing for K5, K9.... */
      /* Process first sample for 5th, 9th .. tap */
      /* f5(n) = f4(n) +  K5 * g4(n-1) */
      fcurr0 = (q31_t) (((q63_t) gcurr0 * (*pk)) >> 32U);
      fcurr0 = (fcurr0 << 1U) + fnext0;

      /* Process second sample for 5th, 9th .. tap */
      fcurr1 = (q31_t) (((q63_t) gnext0 * (*pk)) >> 32U);
      fcurr1 = (fcurr1 << 1U) + fnext1;

      /* Process third sample for 5th, 9th .. tap */
      fcurr2 = (q31_t) (((q63_t) gnext1 * (*pk)) >> 32U);
      fcurr2 = (fcurr2 << 1U) + fnext2;

      /* Process fourth sample for 5th, 9th .. tap */
      fcurr3 = (q31_t) (((q63_t) gnext2 * (*pk)) >> 32U);
      fcurr3 = (fcurr3 << 1U) + fnext3;

      /* Calculation of state values for next stage */
      /* g5(n) = f4(n) * K5  +  g4(n-1) */
      gnext3 = (q31_t) (((q63_t) fnext3 * (*pk)) >> 32U);
      gnext3 = (gnext3 << 1U) + gnext2;

      gnext2 = (q31_t) (((q63_t) fnext2 * (*pk)) >> 32U);
      gnext2 = (gnext2 << 1U) + gnext1;

      gnext1 = (q31_t) (((q63_t) fnext1 * (*pk)) >> 32U);
      gnext1 = (gnext1 << 1U) + gnext0;

      gnext0 = (q31_t) (((q63_t) fnext0 * (*pk++)) >> 32U);
      gnext0 = (gnext0 << 1U) + gcurr0;

      stageCnt--;
    }

    /* If the (filter length -1) is not a multiple of 4, compute the remaining filter taps */
    stageCnt = (numStages - 1U) % 0x4U;

    while (stageCnt > 0U)
    {
      gcurr0 = *px;

      /* save g value in state buffer */
      *px++ = gnext3;

      /* Process four samples for last three taps here */
      fnext0 = (q31_t) (((q63_t) gcurr0 * (*pk)) >> 32U);
      fnext0 = (fnext0 << 1U) + fcurr0;

      fnext1 = (q31_t) (((q63_t) gnext0 * (*pk)) >> 32U);
      fnext1 = (fnext1 << 1U) + fcurr1;

      fnext2 = (q31_t) (((q63_t) gnext1 * (*pk)) >> 32U);
      fnext2 = (fnext2 << 1U) + fcurr2;

      fnext3 = (q31_t) (((q63_t) gnext2 * (*pk)) >> 32U);
      fnext3 = (fnext3 << 1U) + fcurr3;

      /* g1(n) = f0(n) * K1  +  g0(n-1) */
      gnext3 = (q31_t) (((q63_t) fcurr3 * (*pk)) >> 32U);
      gnext3 = (gnext3 << 1U) + gnext2;

      gnext2 = (q31_t) (((q63_t) fcurr2 * (*pk)) >> 32U);
      gnext2 = (gnext2 << 1U) + gnext1;

      gnext1 = (q31_t) (((q63_t) fcurr1 * (*pk)) >> 32U);
      gnext1 = (gnext1 << 1U) + gnext0;

      gnext0 = (q31_t) (((q63_t) fcurr0 * (*pk++)) >> 32U);
      gnext0 = (gnext0 << 1U) + gcurr0;

      /* Update of f values for next coefficient set processing */
      fcurr0 = fnext0;
      fcurr1 = fnext1;
      fcurr2 = fnext2;
      fcurr3 = fnext3;

      stageCnt--;
    }

    /* The results in the 4 accumulators, store in the destination buffer. */
    /* y(n) = fN(n) */
    *pDst++ = fcurr0;
    *pDst++ = fcurr1;
    *pDst++ = fcurr2;
    *pDst++ = fcurr3;

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
    /* f0(n) = x(n) */
    fcurr0 = *pSrc++;

    /* Initialize state pointer */
    px = pState;

    /* Initialize coeff pointer */
    pk = pCoeffs;

    /* read g2(n) from state buffer */
    gcurr0 = *px;

    /* for sample 1 processing */
    /* f1(n) = f0(n) +  K1 * g0(n-1) */
    fnext0 = (q31_t) (((q63_t) gcurr0 * (*pk)) >> 32U);
    fnext0 = (fnext0 << 1U) + fcurr0;

    /* g1(n) = f0(n) * K1  +  g0(n-1) */
    gnext0 = (q31_t) (((q63_t) fcurr0 * (*pk++)) >> 32U);
    gnext0 = (gnext0 << 1U) + gcurr0;

    /* save g1(n) in state buffer */
    *px++ = fcurr0;

    /* f1(n) is saved in fcurr0 for next stage processing */
    fcurr0 = fnext0;

    stageCnt = (numStages - 1U);

    /* stage loop */
    while (stageCnt > 0U)
    {
      /* read g2(n) from state buffer */
      gcurr0 = *px;

      /* save g1(n) in state buffer */
      *px++ = gnext0;

      /* Sample processing for K2, K3.... */
      /* f2(n) = f1(n) +  K2 * g1(n-1) */
      fnext0 = (q31_t) (((q63_t) gcurr0 * (*pk)) >> 32U);
      fnext0 = (fnext0 << 1U) + fcurr0;

      /* g2(n) = f1(n) * K2  +  g1(n-1) */
      gnext0 = (q31_t) (((q63_t) fcurr0 * (*pk++)) >> 32U);
      gnext0 = (gnext0 << 1U) + gcurr0;

      /* f1(n) is saved in fcurr0 for next stage processing */
      fcurr0 = fnext0;

      stageCnt--;
    }

    /* y(n) = fN(n) */
    *pDst++ = fcurr0;

    blkCnt--;
  }

#else
/* alternate version for CM0_FAMILY */

  blkCnt = blockSize;

  while (blkCnt > 0U)
  {
    /* f0(n) = x(n) */
    fcurr0 = *pSrc++;

    /* Initialize state pointer */
    px = pState;

    /* Initialize coeff pointer */
    pk = pCoeffs;

    /* read g0(n-1) from state buffer */
    gcurr0 = *px;

    /* for sample 1 processing */
    /* f1(n) = f0(n) +  K1 * g0(n-1) */
    fnext0 = (q31_t) (((q63_t) gcurr0 * (*pk)) >> 32U);
    fnext0 = (fnext << 1U) + fcurr0;

    /* g1(n) = f0(n) * K1  +  g0(n-1) */
    gnext0 = (q31_t) (((q63_t) fcurr0 * (*pk++)) >> 32U);
    gnext0 = (gnext0 << 1U) + gcurr0;

    /* save f0(n) in state buffer */
    *px++ = fcurr0;

    /* f1(n) is saved in fcurr for next stage processing */
    fcurr0 = fnext0;

    stageCnt = (numStages - 1U);

    /* stage loop */
    while (stageCnt > 0U)
    {
      /* read g1(n-1) from state buffer */
      gcurr0 = *px;

      /* save g0(n-1) in state buffer */
      *px++ = gnext0;

      /* Sample processing for K2, K3.... */
      /* f2(n) = f1(n) +  K2 * g1(n-1) */
      fnext0 = (q31_t) (((q63_t) gcurr0 * (*pk)) >> 32U);
      fnext0 = (fnext0 << 1U) + fcurr0;

      /* g2(n) = f1(n) * K2  +  g1(n-1) */
      gnext0 = (q31_t) (((q63_t) fcurr0 * (*pk++)) >> 32U);
      gnext0 = (gnext0 << 1U) + gcurr0;

      /* f1(n) is saved in fcurr0 for next stage processing */
      fcurr0 = fnext0;

      stageCnt--;
    }

    /* y(n) = fN(n) */
    *pDst++ = fcurr0;

    blkCnt--;
  }

#endif /* #if !defined(ARM_MATH_CM0_FAMILY) */

}

/**
  @} end of FIR_Lattice group
 */
