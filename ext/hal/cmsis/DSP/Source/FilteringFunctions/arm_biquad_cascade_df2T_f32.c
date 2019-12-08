/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_biquad_cascade_df2T_f32.c
 * Description:  Processing function for floating-point transposed direct form II Biquad cascade filter
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

void arm_biquad_cascade_df2T_f32(
  const arm_biquad_cascade_df2T_instance_f32 * S,
  const float32_t * pSrc,
        float32_t * pDst,
        uint32_t blockSize)
{
    const float32_t *pIn = pSrc;                  /*  source pointer            */
    float32_t Xn0, Xn1;
    float32_t acc0, acc1;
    float32_t *pOut = pDst;                 /*  destination pointer       */
    float32_t *pState = S->pState;          /*  State pointer             */
    uint32_t  sample, stage = S->numStages; /*  loop counters             */
    float32_t const *pCurCoeffs =          /*  coefficient pointer       */
                (float32_t const *) S->pCoeffs;
    f32x4_t b0Coeffs, a0Coeffs;           /*  Coefficients vector       */
    f32x4_t b1Coeffs, a1Coeffs;           /*  Modified coef. vector     */
    f32x4_t state;                        /*  State vector              */

    do
    {
        /*
         * temporary carry variable for feeding the 128-bit vector shifter
         */
        uint32_t  tmp = 0;
        /*
         * Reading the coefficients
         * b0Coeffs = {b0, b1, b2, x}
         * a0Coeffs = { x, a1, a2, x}
         */
        b0Coeffs = vld1q(pCurCoeffs);   pCurCoeffs+= 2;
        a0Coeffs = vld1q(pCurCoeffs);   pCurCoeffs+= 3;
        /*
         * Reading the state values
         * state = {d1, d2,  0, 0}
         */
        state = *(f32x4_t *) pState;
        state = vsetq_lane(0.0f, state, 2);
        state = vsetq_lane(0.0f, state, 3);

        /* b1Coeffs = {b0, b1, b2, x} */
        /* b1Coeffs = { x, x, a1, a2} */
        b1Coeffs = vshlcq_s32(b0Coeffs, &tmp, 32);
        a1Coeffs = vshlcq_s32(a0Coeffs, &tmp, 32);

        sample = blockSize / 2;

        /* unrolled 2 x */
        while (sample > 0U)
        {
            /*
             * Read 2 inputs
             */
            Xn0 = *pIn++;
            Xn1 = *pIn++;

            /*
             * 1st half:
             * / acc0 \   / b0 \         / d1 \   / 0  \
             * |  d1  | = | b1 | * Xn0 + | d2 | + | a1 | x acc0
             * |  d2  |   | b2 |         | 0  |   | a2 |
             * \  x   /   \ x  /         \ x  /   \ x  /
             */

            state = vfmaq(state, b0Coeffs, Xn0);
            acc0 = vgetq_lane(state, 0);
            state = vfmaq(state, a0Coeffs, acc0);
            state = vsetq_lane(0.0f, state, 3);

            /*
             * 2nd half:
             * same as 1st half, but all vector elements shifted down.
             * /  x   \   / x  \         / x  \   / x  \
             * | acc1 | = | b0 | * Xn1 + | d1 | + | 0  | x acc1
             * |  d1  |   | b1 |         | d2 |   | a1 |
             * \  d2  /   \ b2 /         \ 0  /   \ a2 /
             */

            state = vfmaq(state, b1Coeffs, Xn1);
            acc1 = vgetq_lane(state, 1);
            state = vfmaq(state, a1Coeffs, acc1);

            /* move d1, d2 up + clearing */
            /* expect dual move or long move */
            state = vsetq_lane(vgetq_lane(state, 2), state, 0);
            state = vsetq_lane(vgetq_lane(state, 3), state, 1);
            state = vsetq_lane(0.0f, state, 2);
            /*
             * Store the results in the destination buffer.
             */
            *pOut++ = acc0;
            *pOut++ = acc1;
            /*
             * decrement the loop counter
             */
            sample--;
        }

        /*
         * tail handling
         */
        if (blockSize & 1)
        {
            Xn0 = *pIn++;
            state = vfmaq(state, b0Coeffs, Xn0);
            acc0 = vgetq_lane(state, 0);

            state = vfmaq(state, a0Coeffs, acc0);
            *pOut++ = acc0;
            *pState++ = vgetq_lane(state, 1);
            *pState++ = vgetq_lane(state, 2);
        }
        else
        {
            *pState++ = vgetq_lane(state, 0);
            *pState++ = vgetq_lane(state, 1);
        }
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
#if defined(ARM_MATH_NEON) 

void arm_biquad_cascade_df2T_f32(
  const arm_biquad_cascade_df2T_instance_f32 * S,
  const float32_t * pSrc,
        float32_t * pDst,
        uint32_t blockSize)
{
   const float32_t *pIn = pSrc;                   /*  source pointer            */
   float32_t *pOut = pDst;                        /*  destination pointer       */
   float32_t *pState = S->pState;                 /*  State pointer             */
   const float32_t *pCoeffs = S->pCoeffs;         /*  coefficient pointer       */
   float32_t acc1;                                /*  accumulator               */
   float32_t b0, b1, b2, a1, a2;                  /*  Filter coefficients       */
   float32_t Xn1;                                 /*  temporary input           */
   float32_t d1, d2;                              /*  state variables           */
   uint32_t sample, stageCnt,stage = S->numStages;         /*   loop counters   */


   float32x4_t XnV, YnV;
   float32x4x2_t dV;
   float32x4_t zeroV = vdupq_n_f32(0.0);
   float32x4_t t1,t2,t3,t4,b1V,b2V,a1V,a2V,s;

   /* Loop unrolling. Compute 4 outputs at a time */
   stageCnt = stage >> 2;

   while (stageCnt > 0U)
   {
      /* Reading the coefficients */
      t1 = vld1q_f32(pCoeffs);
      pCoeffs += 4;

      t2 = vld1q_f32(pCoeffs);
      pCoeffs += 4;

      t3 = vld1q_f32(pCoeffs);
      pCoeffs += 4;

      t4 = vld1q_f32(pCoeffs);
      pCoeffs += 4;

      b1V = vld1q_f32(pCoeffs);
      pCoeffs += 4;

      b2V = vld1q_f32(pCoeffs);
      pCoeffs += 4;

      a1V = vld1q_f32(pCoeffs);
      pCoeffs += 4;

      a2V = vld1q_f32(pCoeffs);
      pCoeffs += 4;

      /* Reading the state values */
      dV = vld2q_f32(pState);

      sample = blockSize;
      
      while (sample > 0U) {
         /* y[n] = b0 * x[n] + d1 */
         /* d1 = b1 * x[n] + a1 * y[n] + d2 */
         /* d2 = b2 * x[n] + a2 * y[n] */

         XnV = vdupq_n_f32(*pIn++);

         s = dV.val[0];
         YnV = s;

         s = vextq_f32(zeroV,dV.val[0],3);
         YnV = vmlaq_f32(YnV, t1, s);

         s = vextq_f32(zeroV,dV.val[0],2);
         YnV = vmlaq_f32(YnV, t2, s);

         s = vextq_f32(zeroV,dV.val[0],1);
         YnV = vmlaq_f32(YnV, t3, s);

         YnV = vmlaq_f32(YnV, t4, XnV);

         s = vextq_f32(XnV,YnV,3);

         dV.val[0] = vmlaq_f32(dV.val[1], s, b1V);
         dV.val[0] = vmlaq_f32(dV.val[0], YnV, a1V);

         dV.val[1] = vmulq_f32(s, b2V);
         dV.val[1] = vmlaq_f32(dV.val[1], YnV, a2V);

         *pOut++ = vgetq_lane_f32(YnV, 3) ;

         sample--;
      }
     
      /* Store the updated state variables back into the state array */
      vst2q_f32(pState,dV);
      pState += 8;

      /* The current stage input is given as the output to the next stage */
      pIn = pDst;

      /*Reset the output working pointer */
      pOut = pDst;

      /* decrement the loop counter */
      stageCnt--;

   } 

   /* Tail */
   stageCnt = stage & 3;
   
   while (stageCnt > 0U)
   {
      /* Reading the coefficients */
      b0 = *pCoeffs++;
      b1 = *pCoeffs++;
      b2 = *pCoeffs++;
      a1 = *pCoeffs++;
      a2 = *pCoeffs++;

      /*Reading the state values */
      d1 = pState[0];
      d2 = pState[1];

      sample = blockSize;

      while (sample > 0U)
      {
         /* Read the input */
         Xn1 = *pIn++;

         /* y[n] = b0 * x[n] + d1 */
         acc1 = (b0 * Xn1) + d1;

         /* Store the result in the accumulator in the destination buffer. */
         *pOut++ = acc1;

         /* Every time after the output is computed state should be updated. */
         /* d1 = b1 * x[n] + a1 * y[n] + d2 */
         d1 = ((b1 * Xn1) + (a1 * acc1)) + d2;

         /* d2 = b2 * x[n] + a2 * y[n] */
         d2 = (b2 * Xn1) + (a2 * acc1);

         /* decrement the loop counter */
         sample--;
      }

      /* Store the updated state variables back into the state array */
      *pState++ = d1;
      *pState++ = d2;

      /* The current stage input is given as the output to the next stage */
      pIn = pDst;

      /*Reset the output working pointer */
      pOut = pDst;

      /* decrement the loop counter */
      stageCnt--;
   }
}
#else
LOW_OPTIMIZATION_ENTER
void arm_biquad_cascade_df2T_f32(
  const arm_biquad_cascade_df2T_instance_f32 * S,
  const float32_t * pSrc,
        float32_t * pDst,
        uint32_t blockSize)
{
  const float32_t *pIn = pSrc;                         /* Source pointer */
        float32_t *pOut = pDst;                        /* Destination pointer */
        float32_t *pState = S->pState;                 /* State pointer */
  const float32_t *pCoeffs = S->pCoeffs;               /* Coefficient pointer */
        float32_t acc1;                                /* Accumulator */
        float32_t b0, b1, b2, a1, a2;                  /* Filter coefficients */
        float32_t Xn1;                                 /* Temporary input */
        float32_t d1, d2;                              /* State variables */
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
     d1 = pState[0];
     d2 = pState[1];

     pCoeffs += 5U;

#if defined (ARM_MATH_LOOPUNROLL)

     /* Loop unrolling: Compute 16 outputs at a time */
     sample = blockSize >> 4U;

     while (sample > 0U) {

       /* y[n] = b0 * x[n] + d1 */
       /* d1 = b1 * x[n] + a1 * y[n] + d2 */
       /* d2 = b2 * x[n] + a2 * y[n] */

/*  1 */
       Xn1 = *pIn++;

       acc1 = b0 * Xn1 + d1;

       d1 = b1 * Xn1 + d2;
       d1 += a1 * acc1;

       d2 = b2 * Xn1;
       d2 += a2 * acc1;

       *pOut++ = acc1;

/*  2 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

/*  3 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

/*  4 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

/*  5 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

/*  6 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

/*  7 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

/*  8 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

/*  9 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

/* 10 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

/* 11 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

/* 12 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

/* 13 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

/* 14 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

/* 15 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

/* 16 */
         Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

        /* decrement loop counter */
        sample--;
      }

      /* Loop unrolling: Compute remaining outputs */
      sample = blockSize & 0xFU;

#else

      /* Initialize blkCnt with number of samples */
      sample = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

      while (sample > 0U) {
        Xn1 = *pIn++;

        acc1 = b0 * Xn1 + d1;

        d1 = b1 * Xn1 + d2;
        d1 += a1 * acc1;

        d2 = b2 * Xn1;
        d2 += a2 * acc1;

        *pOut++ = acc1;

        /* decrement loop counter */
        sample--;
      }

      /* Store the updated state variables back into the state array */
      pState[0] = d1;
      pState[1] = d2;

      pState += 2U;

      /* The current stage input is given as the output to the next stage */
      pIn = pDst;

      /* Reset the output working pointer */
      pOut = pDst;

      /* decrement loop counter */
      stage--;

   } while (stage > 0U);

}
LOW_OPTIMIZATION_EXIT
#endif /* #if defined(ARM_MATH_NEON) */
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of BiquadCascadeDF2T group
 */
