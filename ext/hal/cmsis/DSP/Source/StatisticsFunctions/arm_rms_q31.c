/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_rms_q31.c
 * Description:  Root Mean Square of the elements of a Q31 vector
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
  @ingroup groupStats
 */

/**
  @addtogroup RMS
  @{
 */

/**
  @brief         Root Mean Square of the elements of a Q31 vector.
  @param[in]     pSrc       points to the input vector
  @param[in]     blockSize  number of samples in input vector
  @param[out]    pResult    root mean square value returned here
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using an internal 64-bit accumulator.
                   The input is represented in 1.31 format, and intermediate multiplication
                   yields a 2.62 format.
                   The accumulator maintains full precision of the intermediate multiplication results,
                   but provides only a single guard bit.
                   There is no saturation on intermediate additions.
                   If the accumulator overflows, it wraps around and distorts the result.
                   In order to avoid overflows completely, the input signal must be scaled down by
                   log2(blockSize) bits, as a total of blockSize additions are performed internally.
                   Finally, the 2.62 accumulator is right shifted by 31 bits to yield a 1.31 format value.
 */
#if defined(ARM_MATH_MVEI)

void arm_rms_q31(
  const q31_t * pSrc,
        uint32_t blockSize,
        q31_t * pResult)
{
    q63_t pow = 0.0f;
    q31_t normalizedPower;
    arm_power_q31(pSrc, blockSize, &pow);

    normalizedPower=clip_q63_to_q31((pow / (q63_t) blockSize) >> 17);
    arm_sqrt_q31(normalizedPower, pResult);

}

#else
void arm_rms_q31(
  const q31_t * pSrc,
        uint32_t blockSize,
        q31_t * pResult)
{
        uint32_t blkCnt;                               /* Loop counter */
        uint64_t sum = 0;                              /* Temporary result storage (can get never negative. changed type from q63 to uint64 */
        q31_t in;                                      /* Temporary variable to store input value */

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A[0] * A[0] + A[1] * A[1] + ... + A[blockSize-1] * A[blockSize-1] */

    in = *pSrc++;
    /* Compute sum of squares and store result in a temporary variable, sum. */
    sum += ((q63_t) in * in);

    in = *pSrc++;
    sum += ((q63_t) in * in);

    in = *pSrc++;
    sum += ((q63_t) in * in);

    in = *pSrc++;
    sum += ((q63_t) in * in);

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
    /* C = A[0] * A[0] + A[1] * A[1] + ... + A[blockSize-1] * A[blockSize-1] */

    in = *pSrc++;
    /* Compute sum of squares and store result in a temporary variable. */
    sum += ((q63_t) in * in);

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Convert data in 2.62 to 1.31 by 31 right shifts and saturate */
  /* Compute Rms and store result in destination vector */
  arm_sqrt_q31(clip_q63_to_q31((sum / (q63_t) blockSize) >> 31), pResult);
}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of RMS group
 */
