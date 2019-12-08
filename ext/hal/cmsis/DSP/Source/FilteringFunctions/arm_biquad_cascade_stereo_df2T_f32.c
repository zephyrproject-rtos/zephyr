/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_biquad_cascade_stereo_df2T_f32.c
 * Description:  Processing function for floating-point transposed direct form II Biquad cascade filter. 2 channels
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
  @addtogroup BiquadCascadeDF2T
  @{
 */

/**
  @brief         Processing function for the floating-point transposed direct form II Biquad cascade filter.
  @param[in]     S         points to an instance of the filter data structure
  @param[in]     pSrc      points to the block of input data
  @param[out]    pDst      points to the block of output data
  @param[in]     blockSize number of samples to process
  @return        none
 */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
#include "arm_helium_utils.h"

void arm_biquad_cascade_stereo_df2T_f32(
  const arm_biquad_cascade_stereo_df2T_instance_f32 * S,
  const float32_t * pSrc,
        float32_t * pDst,
        uint32_t blockSize)
{
    const float32_t *pIn = pSrc;              /*  source pointer            */
    float32_t *pOut = pDst;             /*  destination pointer       */
    float32_t *pState = S->pState;      /*  State pointer             */
    const float32_t *pCoeffs = S->pCoeffs;    /*  coefficient pointer       */
    float32_t b0, b1, b2, a1, a2;       /*  Filter coefficients       */
    uint32_t  sample, stage = S->numStages; /*  loop counters           */
    float32_t scratch[6];
    uint32x4_t loadIdxVec;
    f32x4_t aCoeffs, bCoeffs;
    f32x4_t stateVec0, stateVec1;
    f32x4_t inVec;
    uint32_t  startIdx = 0;

    /*
     * {0, 1, 0, 1} generator
     */
    loadIdxVec = viwdupq_u32(&startIdx, 2, 1);

    /*
     * scratch top clearing
     * layout : [d1a d1b d2a d2b 0 0]
     */
    scratch[4] = 0.0f;
    scratch[5] = 0.0f;

    do
    {
        /*
         * Reading the coefficients
         */
        b0 = *pCoeffs++;
        b1 = *pCoeffs++;
        b2 = *pCoeffs++;
        a1 = *pCoeffs++;
        a2 = *pCoeffs++;

        /*
         * aCoeffs = {a1 a1 a2 a2}
         */
        aCoeffs = vdupq_n_f32(a1);
        aCoeffs = vsetq_lane(a2, aCoeffs, 2);
        aCoeffs = vsetq_lane(a2, aCoeffs, 3);

        /*
         * bCoeffs = {b1 b1 b2 b2}
         */
        bCoeffs = vdupq_n_f32(b1);
        bCoeffs = vsetq_lane(b2, bCoeffs, 2);
        bCoeffs = vsetq_lane(b2, bCoeffs, 3);

        /*
         * Reading the state values
         * Save into scratch
         */
        *(f32x4_t *) scratch = *(f32x4_t *) pState;

        sample = blockSize;

        while (sample > 0U)
        {
            /*
             * step 1
             *
             * 0   | acc1a = xn1a * b0 + d1a
             * 1   | acc1b = xn1b * b0 + d1b
             * 2   | acc1a = xn1a * b0 + d1a
             * 3   | acc1b = xn1b * b0 + d1b
             */
            /*
             * load {d1a, d1b, d1a, d1b}
             */
            stateVec0 = vldrwq_gather_shifted_offset((uint32_t const *) scratch, loadIdxVec);
            /*
             * load {in0 in1 in0 in1}
             */
            inVec = vldrwq_gather_shifted_offset((uint32_t const *) pIn, loadIdxVec);

            stateVec0 = vfmaq(stateVec0, inVec, b0);
            *pOut++ = vgetq_lane(stateVec0, 0);
            *pOut++ = vgetq_lane(stateVec0, 1);

            /*
             * step 2
             *
             * 0  | d1a = b1 * xn1a  +  a1 * acc1a  +  d2a
             * 1  | d1b = b1 * xn1b  +  a1 * acc1b  +  d2b
             * 2  | d2a = b2 * xn1a  +  a2 * acc1a  +  0
             * 3  | d2b = b2 * xn1b  +  a2 * acc1b  +  0
             */

            /*
             * load {d2a, d2b, 0, 0}
             */
            stateVec1 = *(f32x4_t *) & scratch[2];
            stateVec1 = vfmaq(stateVec1, stateVec0, aCoeffs);
            stateVec1 = vfmaq(stateVec1, inVec, bCoeffs);
            *(f32x4_t *) scratch = stateVec1;

            pIn = pIn + 2;
            sample--;
        }

        /*
         * Store the updated state variables back into the state array
         */
        vst1q(pState, stateVec1);
        pState += 4;

        /*
         * The current stage input is given as the output to the next stage
         */
        pIn = pDst;
        /*
         * Reset the output working pointer
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
LOW_OPTIMIZATION_ENTER
void arm_biquad_cascade_stereo_df2T_f32(
  const arm_biquad_cascade_stereo_df2T_instance_f32 * S,
  const float32_t * pSrc,
        float32_t * pDst,
        uint32_t blockSize)
{
  const float32_t *pIn = pSrc;                         /* Source pointer */
        float32_t *pOut = pDst;                        /* Destination pointer */
        float32_t *pState = S->pState;                 /* State pointer */
  const float32_t *pCoeffs = S->pCoeffs;               /* Coefficient pointer */
        float32_t acc1a, acc1b;                        /* Accumulator */
        float32_t b0, b1, b2, a1, a2;                  /* Filter coefficients */
        float32_t Xn1a, Xn1b;                          /* Temporary input */
        float32_t d1a, d2a, d1b, d2b;                  /* State variables */
        uint32_t sample, stage = S->numStages;         /* Loop counters */

    do
    {
        /* Reading the coefficients */
        b0 = pCoeffs[0];
        b1 = pCoeffs[1];
        b2 = pCoeffs[2];
        a1 = pCoeffs[3];
        a2 = pCoeffs[4];

        /* Reading the state values */
        d1a = pState[0];
        d2a = pState[1];
        d1b = pState[2];
        d2b = pState[3];

        pCoeffs += 5U;

#if defined (ARM_MATH_LOOPUNROLL)

    /* Loop unrolling: Compute 8 outputs at a time */
        sample = blockSize >> 3U;

        while (sample > 0U) {
          /* y[n] = b0 * x[n] + d1 */
          /* d1 = b1 * x[n] + a1 * y[n] + d2 */
          /* d2 = b2 * x[n] + a2 * y[n] */

/*  1 */
          Xn1a = *pIn++; /* Channel a */
          Xn1b = *pIn++; /* Channel b */

          acc1a = (b0 * Xn1a) + d1a;
          acc1b = (b0 * Xn1b) + d1b;

          *pOut++ = acc1a;
          *pOut++ = acc1b;

          d1a = ((b1 * Xn1a) + (a1 * acc1a)) + d2a;
          d1b = ((b1 * Xn1b) + (a1 * acc1b)) + d2b;

          d2a = (b2 * Xn1a) + (a2 * acc1a);
          d2b = (b2 * Xn1b) + (a2 * acc1b);

/*  2 */
          Xn1a = *pIn++; /* Channel a */
          Xn1b = *pIn++; /* Channel b */

          acc1a = (b0 * Xn1a) + d1a;
          acc1b = (b0 * Xn1b) + d1b;

          *pOut++ = acc1a;
          *pOut++ = acc1b;

          d1a = ((b1 * Xn1a) + (a1 * acc1a)) + d2a;
          d1b = ((b1 * Xn1b) + (a1 * acc1b)) + d2b;

          d2a = (b2 * Xn1a) + (a2 * acc1a);
          d2b = (b2 * Xn1b) + (a2 * acc1b);

/*  3 */
          Xn1a = *pIn++; /* Channel a */
          Xn1b = *pIn++; /* Channel b */

          acc1a = (b0 * Xn1a) + d1a;
          acc1b = (b0 * Xn1b) + d1b;

          *pOut++ = acc1a;
          *pOut++ = acc1b;

          d1a = ((b1 * Xn1a) + (a1 * acc1a)) + d2a;
          d1b = ((b1 * Xn1b) + (a1 * acc1b)) + d2b;

          d2a = (b2 * Xn1a) + (a2 * acc1a);
          d2b = (b2 * Xn1b) + (a2 * acc1b);

/*  4 */
          Xn1a = *pIn++; /* Channel a */
          Xn1b = *pIn++; /* Channel b */

          acc1a = (b0 * Xn1a) + d1a;
          acc1b = (b0 * Xn1b) + d1b;

          *pOut++ = acc1a;
          *pOut++ = acc1b;

          d1a = ((b1 * Xn1a) + (a1 * acc1a)) + d2a;
          d1b = ((b1 * Xn1b) + (a1 * acc1b)) + d2b;

          d2a = (b2 * Xn1a) + (a2 * acc1a);
          d2b = (b2 * Xn1b) + (a2 * acc1b);

/*  5 */
          Xn1a = *pIn++; /* Channel a */
          Xn1b = *pIn++; /* Channel b */

          acc1a = (b0 * Xn1a) + d1a;
          acc1b = (b0 * Xn1b) + d1b;

          *pOut++ = acc1a;
          *pOut++ = acc1b;

          d1a = ((b1 * Xn1a) + (a1 * acc1a)) + d2a;
          d1b = ((b1 * Xn1b) + (a1 * acc1b)) + d2b;

          d2a = (b2 * Xn1a) + (a2 * acc1a);
          d2b = (b2 * Xn1b) + (a2 * acc1b);

/*  6 */
          Xn1a = *pIn++; /* Channel a */
          Xn1b = *pIn++; /* Channel b */

          acc1a = (b0 * Xn1a) + d1a;
          acc1b = (b0 * Xn1b) + d1b;

          *pOut++ = acc1a;
          *pOut++ = acc1b;

          d1a = ((b1 * Xn1a) + (a1 * acc1a)) + d2a;
          d1b = ((b1 * Xn1b) + (a1 * acc1b)) + d2b;

          d2a = (b2 * Xn1a) + (a2 * acc1a);
          d2b = (b2 * Xn1b) + (a2 * acc1b);

/*  7 */
          Xn1a = *pIn++; /* Channel a */
          Xn1b = *pIn++; /* Channel b */

          acc1a = (b0 * Xn1a) + d1a;
          acc1b = (b0 * Xn1b) + d1b;

          *pOut++ = acc1a;
          *pOut++ = acc1b;

          d1a = ((b1 * Xn1a) + (a1 * acc1a)) + d2a;
          d1b = ((b1 * Xn1b) + (a1 * acc1b)) + d2b;

          d2a = (b2 * Xn1a) + (a2 * acc1a);
          d2b = (b2 * Xn1b) + (a2 * acc1b);

/*  8 */
          Xn1a = *pIn++; /* Channel a */
          Xn1b = *pIn++; /* Channel b */

          acc1a = (b0 * Xn1a) + d1a;
          acc1b = (b0 * Xn1b) + d1b;

          *pOut++ = acc1a;
          *pOut++ = acc1b;

          d1a = ((b1 * Xn1a) + (a1 * acc1a)) + d2a;
          d1b = ((b1 * Xn1b) + (a1 * acc1b)) + d2b;

          d2a = (b2 * Xn1a) + (a2 * acc1a);
          d2b = (b2 * Xn1b) + (a2 * acc1b);

          /* decrement loop counter */
          sample--;
        }

        /* Loop unrolling: Compute remaining outputs */
        sample = blockSize & 0x7U;

#else

        /* Initialize blkCnt with number of samples */
        sample = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

        while (sample > 0U) {
          /* Read the input */
          Xn1a = *pIn++; /* Channel a */
          Xn1b = *pIn++; /* Channel b */

          /* y[n] = b0 * x[n] + d1 */
          acc1a = (b0 * Xn1a) + d1a;
          acc1b = (b0 * Xn1b) + d1b;

          /* Store the result in the accumulator in the destination buffer. */
          *pOut++ = acc1a;
          *pOut++ = acc1b;

          /* Every time after the output is computed state should be updated. */
          /* d1 = b1 * x[n] + a1 * y[n] + d2 */
          d1a = ((b1 * Xn1a) + (a1 * acc1a)) + d2a;
          d1b = ((b1 * Xn1b) + (a1 * acc1b)) + d2b;

          /* d2 = b2 * x[n] + a2 * y[n] */
          d2a = (b2 * Xn1a) + (a2 * acc1a);
          d2b = (b2 * Xn1b) + (a2 * acc1b);

          /* decrement loop counter */
          sample--;
        }

        /* Store the updated state variables back into the state array */
        pState[0] = d1a;
        pState[1] = d2a;

        pState[2] = d1b;
        pState[3] = d2b;

        pState += 4U;

        /* The current stage input is given as the output to the next stage */
        pIn = pDst;

        /* Reset the output working pointer */
        pOut = pDst;

        /* Decrement the loop counter */
        stage--;

    } while (stage > 0U);

}
LOW_OPTIMIZATION_EXIT
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of BiquadCascadeDF2T group
 */
