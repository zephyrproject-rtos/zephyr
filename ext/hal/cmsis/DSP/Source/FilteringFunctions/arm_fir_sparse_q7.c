/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_fir_sparse_q7.c
 * Description:  Q7 sparse FIR filter processing function
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
  @addtogroup FIR_Sparse
  @{
 */

/**
  @brief         Processing function for the Q7 sparse FIR filter.
  @param[in]     S           points to an instance of the Q7 sparse FIR structure
  @param[in]     pSrc        points to the block of input data
  @param[out]    pDst        points to the block of output data
  @param[in]     pScratchIn  points to a temporary buffer of size blockSize
  @param[in]     pScratchOut points to a temporary buffer of size blockSize
  @param[in]     blockSize   number of input samples to process
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using a 32-bit internal accumulator.
                   Both coefficients and state variables are represented in 1.7 format and multiplications yield a 2.14 result.
                   The 2.14 intermediate results are accumulated in a 32-bit accumulator in 18.14 format.
                   There is no risk of internal overflow with this approach and the full precision of intermediate multiplications is preserved.
                   The accumulator is then converted to 18.7 format by discarding the low 7 bits.
                   Finally, the result is truncated to 1.7 format.
 */

void arm_fir_sparse_q7(
        arm_fir_sparse_instance_q7 * S,
  const q7_t * pSrc,
        q7_t * pDst,
        q7_t * pScratchIn,
        q31_t * pScratchOut,
        uint32_t blockSize)
{
        q7_t *pState = S->pState;                      /* State pointer */
  const q7_t *pCoeffs = S->pCoeffs;                    /* Coefficient pointer */
        q7_t *px;                                      /* Scratch buffer pointer */
        q7_t *py = pState;                             /* Temporary pointers for state buffer */
        q7_t *pb = pScratchIn;                         /* Temporary pointers for scratch buffer */
        q7_t *pOut = pDst;                             /* Destination pointer */
        int32_t *pTapDelay = S->pTapDelay;             /* Pointer to the array containing offset of the non-zero tap values. */
        uint32_t delaySize = S->maxDelay + blockSize;  /* state length */
        uint16_t numTaps = S->numTaps;                 /* Number of filter coefficients in the filter  */
        int32_t readIndex;                             /* Read index of the state buffer */
        uint32_t tapCnt, blkCnt;                       /* loop counters */
        q31_t *pScr2 = pScratchOut;                    /* Working pointer for scratch buffer of output values */
        q31_t in;
        q7_t coeff = *pCoeffs++;                       /* Read the coefficient value */

#if defined (ARM_MATH_LOOPUNROLL)
        q7_t in1, in2, in3, in4;
#endif

  /* BlockSize of Input samples are copied into the state buffer */
  /* StateIndex points to the starting position to write in the state buffer */
  arm_circularWrite_q7(py, (int32_t) delaySize, &S->stateIndex, 1, pSrc, 1, blockSize);

  /* Loop over the number of taps. */
  tapCnt = numTaps;

  /* Read Index, from where the state buffer should be read, is calculated. */
  readIndex = (int32_t) (S->stateIndex - blockSize) - *pTapDelay++;

  /* Wraparound of readIndex */
  if (readIndex < 0)
  {
    readIndex += (int32_t) delaySize;
  }

  /* Working pointer for state buffer is updated */
  py = pState;

  /* blockSize samples are read from the state buffer */
  arm_circularRead_q7(py, (int32_t) delaySize, &readIndex, 1,
                   pb, pb, (int32_t) blockSize, 1, blockSize);

  /* Working pointer for the scratch buffer of state values */
  px = pb;

  /* Working pointer for scratch buffer of output values */
  pScratchOut = pScr2;


#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time. */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* Perform multiplication and store in the scratch buffer */
    *pScratchOut++ = ((q31_t) *px++ * coeff);
    *pScratchOut++ = ((q31_t) *px++ * coeff);
    *pScratchOut++ = ((q31_t) *px++ * coeff);
    *pScratchOut++ = ((q31_t) *px++ * coeff);

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
    /* Perform Multiplication and store in the scratch buffer */
    *pScratchOut++ = ((q31_t) *px++ * coeff);

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Load the coefficient value and
   * increment the coefficient buffer for the next set of state values */
  coeff = *pCoeffs++;

  /* Read Index, from where the state buffer should be read, is calculated. */
  readIndex = (int32_t) (S->stateIndex - blockSize) - *pTapDelay++;

  /* Wraparound of readIndex */
  if (readIndex < 0)
  {
    readIndex += (int32_t) delaySize;
  }

  /* Loop over the number of taps. */
  tapCnt = (uint32_t) numTaps - 2U;

  while (tapCnt > 0U)
  {
    /* Working pointer for state buffer is updated */
    py = pState;

    /* blockSize samples are read from the state buffer */
    arm_circularRead_q7(py, (int32_t) delaySize, &readIndex, 1,
                        pb, pb, (int32_t) blockSize, 1, blockSize);

    /* Working pointer for the scratch buffer of state values */
    px = pb;

    /* Working pointer for scratch buffer of output values */
    pScratchOut = pScr2;


#if defined (ARM_MATH_LOOPUNROLL)

    /* Loop unrolling: Compute 4 outputs at a time. */
    blkCnt = blockSize >> 2U;

    while (blkCnt > 0U)
    {
      /* Perform Multiply-Accumulate */
      in = *pScratchOut + ((q31_t) * px++ * coeff);
      *pScratchOut++ = in;
      in = *pScratchOut + ((q31_t) * px++ * coeff);
      *pScratchOut++ = in;
      in = *pScratchOut + ((q31_t) * px++ * coeff);
      *pScratchOut++ = in;
      in = *pScratchOut + ((q31_t) * px++ * coeff);
      *pScratchOut++ = in;

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
      /* Perform Multiply-Accumulate */
      in = *pScratchOut + ((q31_t) *px++ * coeff);
      *pScratchOut++ = in;

      /* Decrement loop counter */
      blkCnt--;
    }

    /* Load the coefficient value and
     * increment the coefficient buffer for the next set of state values */
    coeff = *pCoeffs++;

    /* Read Index, from where the state buffer should be read, is calculated. */
    readIndex = (int32_t) (S->stateIndex - blockSize) - *pTapDelay++;

    /* Wraparound of readIndex */
    if (readIndex < 0)
    {
      readIndex += (int32_t) delaySize;
    }

    /* Decrement loop counter */
    tapCnt--;
  }

  /* Compute last tap without the final read of pTapDelay */

  /* Working pointer for state buffer is updated */
  py = pState;

  /* blockSize samples are read from the state buffer */
  arm_circularRead_q7(py, (int32_t) delaySize, &readIndex, 1,
                      pb, pb, (int32_t) blockSize, 1, blockSize);

  /* Working pointer for the scratch buffer of state values */
  px = pb;

  /* Working pointer for scratch buffer of output values */
  pScratchOut = pScr2;


#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time. */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* Perform Multiply-Accumulate */
    in = *pScratchOut + ((q31_t) *px++ * coeff);
    *pScratchOut++ = in;
    in = *pScratchOut + ((q31_t) *px++ * coeff);
    *pScratchOut++ = in;
    in = *pScratchOut + ((q31_t) *px++ * coeff);
    *pScratchOut++ = in;
    in = *pScratchOut + ((q31_t) *px++ * coeff);
    *pScratchOut++ = in;

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
    /* Perform Multiply-Accumulate */
    in = *pScratchOut + ((q31_t) *px++ * coeff);
    *pScratchOut++ = in;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* All the output values are in pScratchOut buffer.
     Convert them into 1.15 format, saturate and store in the destination buffer. */
#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time. */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    in1 = (q7_t) __SSAT(*pScr2++ >> 7, 8);
    in2 = (q7_t) __SSAT(*pScr2++ >> 7, 8);
    in3 = (q7_t) __SSAT(*pScr2++ >> 7, 8);
    in4 = (q7_t) __SSAT(*pScr2++ >> 7, 8);

    write_q7x4_ia (&pOut, __PACKq7(in1, in2, in3, in4));

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
    *pOut++ = (q7_t) __SSAT(*pScr2++ >> 7, 8);

    /* Decrement loop counter */
    blkCnt--;
  }

}

/**
  @} end of FIR_Sparse group
 */
