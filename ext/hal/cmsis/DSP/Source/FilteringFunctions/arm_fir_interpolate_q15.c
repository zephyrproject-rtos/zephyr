/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_fir_interpolate_q15.c
 * Description:  Q15 FIR interpolation
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
  @addtogroup FIR_Interpolate
  @{
 */

/**
  @brief         Processing function for the Q15 FIR interpolator.
  @param[in]     S          points to an instance of the Q15 FIR interpolator structure
  @param[in]     pSrc       points to the block of input data
  @param[out]    pDst       points to the block of output data
  @param[in]     blockSize  number of samples to process
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using a 64-bit internal accumulator.
                   Both coefficients and state variables are represented in 1.15 format and multiplications yield a 2.30 result.
                   The 2.30 intermediate results are accumulated in a 64-bit accumulator in 34.30 format.
                   There is no risk of internal overflow with this approach and the full precision of intermediate multiplications is preserved.
                   After all additions have been performed, the accumulator is truncated to 34.15 format by discarding low 15 bits.
                   Lastly, the accumulator is saturated to yield a result in 1.15 format.
 */

void arm_fir_interpolate_q15(
  const arm_fir_interpolate_instance_q15 * S,
  const q15_t * pSrc,
        q15_t * pDst,
        uint32_t blockSize)
{
#if (1)
//#if !defined(ARM_MATH_CM0_FAMILY)

        q15_t *pState = S->pState;                     /* State pointer */
  const q15_t *pCoeffs = S->pCoeffs;                   /* Coefficient pointer */
        q15_t *pStateCur;                              /* Points to the current sample of the state */
        q15_t *ptr1;                                   /* Temporary pointer for state buffer */
  const q15_t *ptr2;                                   /* Temporary pointer for coefficient buffer */
        q63_t sum0;                                    /* Accumulators */
        uint32_t i, blkCnt, tapCnt;                    /* Loop counters */
        uint32_t phaseLen = S->phaseLength;            /* Length of each polyphase filter component */
        uint32_t j;

#if defined (ARM_MATH_LOOPUNROLL)
        q63_t acc0, acc1, acc2, acc3;
        q15_t x0, x1, x2, x3;
        q15_t c0, c1, c2, c3;
#endif

  /* S->pState buffer contains previous frame (phaseLen - 1) samples */
  /* pStateCur points to the location where the new input data should be written */
  pStateCur = S->pState + (phaseLen - 1U);

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* Copy new input sample into the state buffer */
    *pStateCur++ = *pSrc++;
    *pStateCur++ = *pSrc++;
    *pStateCur++ = *pSrc++;
    *pStateCur++ = *pSrc++;

    /* Address modifier index of coefficient buffer */
    j = 1U;

    /* Loop over the Interpolation factor. */
    i = (S->L);

    while (i > 0U)
    {
      /* Set accumulator to zero */
      acc0 = 0;
      acc1 = 0;
      acc2 = 0;
      acc3 = 0;

      /* Initialize state pointer */
      ptr1 = pState;

      /* Initialize coefficient pointer */
      ptr2 = pCoeffs + (S->L - j);

      /* Loop over the polyPhase length. Unroll by a factor of 4.
         Repeat until we've computed numTaps-(4*S->L) coefficients. */
      tapCnt = phaseLen >> 2U;

      x0 = *(ptr1++);
      x1 = *(ptr1++);
      x2 = *(ptr1++);

      while (tapCnt > 0U)
      {
        /* Read the input sample */
        x3 = *(ptr1++);

        /* Read the coefficient */
        c0 = *(ptr2);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x0 * c0;
        acc1 += (q63_t) x1 * c0;
        acc2 += (q63_t) x2 * c0;
        acc3 += (q63_t) x3 * c0;

        /* Read the coefficient */
        c1 = *(ptr2 + S->L);

        /* Read the input sample */
        x0 = *(ptr1++);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x1 * c1;
        acc1 += (q63_t) x2 * c1;
        acc2 += (q63_t) x3 * c1;
        acc3 += (q63_t) x0 * c1;

        /* Read the coefficient */
        c2 = *(ptr2 + S->L * 2);

        /* Read the input sample */
        x1 = *(ptr1++);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x2 * c2;
        acc1 += (q63_t) x3 * c2;
        acc2 += (q63_t) x0 * c2;
        acc3 += (q63_t) x1 * c2;

        /* Read the coefficient */
        c3 = *(ptr2 + S->L * 3);

        /* Read the input sample */
        x2 = *(ptr1++);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x3 * c3;
        acc1 += (q63_t) x0 * c3;
        acc2 += (q63_t) x1 * c3;
        acc3 += (q63_t) x2 * c3;


        /* Upsampling is done by stuffing L-1 zeros between each sample.
         * So instead of multiplying zeros with coefficients,
         * Increment the coefficient pointer by interpolation factor times. */
        ptr2 += 4 * S->L;

        /* Decrement loop counter */
        tapCnt--;
      }

      /* If the polyPhase length is not a multiple of 4, compute the remaining filter taps */
      tapCnt = phaseLen % 0x4U;

      while (tapCnt > 0U)
      {
        /* Read the input sample */
        x3 = *(ptr1++);

        /* Read the coefficient */
        c0 = *(ptr2);

        /* Perform the multiply-accumulate */
        acc0 += (q63_t) x0 * c0;
        acc1 += (q63_t) x1 * c0;
        acc2 += (q63_t) x2 * c0;
        acc3 += (q63_t) x3 * c0;

        /* Increment the coefficient pointer by interpolation factor times. */
        ptr2 += S->L;

        /* update states for next sample processing */
        x0 = x1;
        x1 = x2;
        x2 = x3;

        /* Decrement loop counter */
        tapCnt--;
      }

      /* The result is in the accumulator, store in the destination buffer. */
      *(pDst           ) = (q15_t) (__SSAT((acc0 >> 15), 16));
      *(pDst +     S->L) = (q15_t) (__SSAT((acc1 >> 15), 16));
      *(pDst + 2 * S->L) = (q15_t) (__SSAT((acc2 >> 15), 16));
      *(pDst + 3 * S->L) = (q15_t) (__SSAT((acc3 >> 15), 16));

      pDst++;

      /* Increment the address modifier index of coefficient buffer */
      j++;

      /* Decrement loop counter */
      i--;
    }

    /* Advance the state pointer by 1
     * to process the next group of interpolation factor number samples */
    pState = pState + 4;

    pDst += S->L * 3;

    /* Decrement loop counter */
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
    /* Copy new input sample into the state buffer */
    *pStateCur++ = *pSrc++;

    /* Address modifier index of coefficient buffer */
    j = 1U;

    /* Loop over the Interpolation factor. */
    i = S->L;
    while (i > 0U)
    {
      /* Set accumulator to zero */
      sum0 = 0;

      /* Initialize state pointer */
      ptr1 = pState;

      /* Initialize coefficient pointer */
      ptr2 = pCoeffs + (S->L - j);

      /* Loop over the polyPhase length.
         Repeat until we've computed numTaps-(4*S->L) coefficients. */

#if defined (ARM_MATH_LOOPUNROLL)

     /* Loop unrolling: Compute 4 outputs at a time */
      tapCnt = phaseLen >> 2U;

      while (tapCnt > 0U)
      {
        /* Perform the multiply-accumulate */
        sum0 += (q63_t) *ptr1++ * *ptr2;

        /* Upsampling is done by stuffing L-1 zeros between each sample.
         * So instead of multiplying zeros with coefficients,
         * Increment the coefficient pointer by interpolation factor times. */
        ptr2 += S->L;

        sum0 += (q63_t) *ptr1++ * *ptr2;
        ptr2 += S->L;

        sum0 += (q63_t) *ptr1++ * *ptr2;
        ptr2 += S->L;

        sum0 += (q63_t) *ptr1++ * *ptr2;
        ptr2 += S->L;

        /* Decrement loop counter */
        tapCnt--;
      }

      /* Loop unrolling: Compute remaining outputs */
      tapCnt = phaseLen % 0x4U;

#else

      /* Initialize tapCnt with number of samples */
      tapCnt = phaseLen;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

      while (tapCnt > 0U)
      {
        /* Perform the multiply-accumulate */
        sum0 += (q63_t) *ptr1++ * *ptr2;

        /* Upsampling is done by stuffing L-1 zeros between each sample.
         * So instead of multiplying zeros with coefficients,
         * Increment the coefficient pointer by interpolation factor times. */
        ptr2 += S->L;

        /* Decrement loop counter */
        tapCnt--;
      }

      /* The result is in the accumulator, store in the destination buffer. */
      *pDst++ = (q15_t) (__SSAT((sum0 >> 15), 16));

      /* Increment the address modifier index of coefficient buffer */
      j++;

      /* Decrement the loop counter */
      i--;
    }

    /* Advance the state pointer by 1
     * to process the next group of interpolation factor number samples */
    pState = pState + 1;

    /* Decrement the loop counter */
    blkCnt--;
  }

  /* Processing is complete.
     Now copy the last phaseLen - 1 samples to the satrt of the state buffer.
     This prepares the state buffer for the next function call. */

  /* Points to the start of the state buffer */
  pStateCur = S->pState;

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  tapCnt = (phaseLen - 1U) >> 2U;

  /* copy data */
  while (tapCnt > 0U)
  {
    write_q15x2_ia (&pStateCur, read_q15x2_ia (&pState));
    write_q15x2_ia (&pStateCur, read_q15x2_ia (&pState));

    /* Decrement loop counter */
    tapCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  tapCnt = (phaseLen - 1U) % 0x04U;

#else

    /* Initialize tapCnt with number of samples */
    tapCnt = (phaseLen - 1U);

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  /* Copy data */
  while (tapCnt > 0U)
  {
    *pStateCur++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

#else
/* alternate version for CM0_FAMILY */

        q15_t *pState = S->pState;                     /* State pointer */
  const q15_t *pCoeffs = S->pCoeffs;                   /* Coefficient pointer */
        q15_t *pStateCur;                              /* Points to the current sample of the state */
        q15_t *ptr1;                                   /* Temporary pointer for state buffer */
  const q15_t *ptr2;                                   /* Temporary pointer for coefficient buffer */
        q63_t sum0;                                    /* Accumulators */
        uint32_t i, blkCnt, tapCnt;                    /* Loop counters */
        uint32_t phaseLen = S->phaseLength;            /* Length of each polyphase filter component */

  /* S->pState buffer contains previous frame (phaseLen - 1) samples */
  /* pStateCur points to the location where the new input data should be written */
  pStateCur = S->pState + (phaseLen - 1U);

  /* Total number of intput samples */
  blkCnt = blockSize;

  /* Loop over the blockSize. */
  while (blkCnt > 0U)
  {
    /* Copy new input sample into the state buffer */
    *pStateCur++ = *pSrc++;

    /* Loop over the Interpolation factor. */
    i = S->L;

    while (i > 0U)
    {
      /* Set accumulator to zero */
      sum0 = 0;

      /* Initialize state pointer */
      ptr1 = pState;

      /* Initialize coefficient pointer */
      ptr2 = pCoeffs + (i - 1U);

      /* Loop over the polyPhase length */
      tapCnt = phaseLen;

      while (tapCnt > 0U)
      {
        /* Perform the multiply-accumulate */
        sum0 += ((q63_t) *ptr1++ * *ptr2);

        /* Increment the coefficient pointer by interpolation factor times. */
        ptr2 += S->L;

        /* Decrement the loop counter */
        tapCnt--;
      }

      /* Store the result after converting to 1.15 format in the destination buffer. */
      *pDst++ = (q15_t) (__SSAT((sum0 >> 15), 16));

      /* Decrement loop counter */
      i--;
    }

    /* Advance the state pointer by 1
     * to process the next group of interpolation factor number samples */
    pState = pState + 1;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Processing is complete.
   ** Now copy the last phaseLen - 1 samples to the start of the state buffer.
   ** This prepares the state buffer for the next function call. */

  /* Points to the start of the state buffer */
  pStateCur = S->pState;

  tapCnt = phaseLen - 1U;

  /* Copy data */
  while (tapCnt > 0U)
  {
    *pStateCur++ = *pState++;

    /* Decrement loop counter */
    tapCnt--;
  }

#endif /* #if !defined(ARM_MATH_CM0_FAMILY) */

}

/**
  @} end of FIR_Interpolate group
 */
