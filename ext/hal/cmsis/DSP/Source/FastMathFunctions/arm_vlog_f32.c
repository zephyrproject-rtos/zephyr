/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_vlog_f32.c
 * Description:  Fast vectorized log
 *
 * $Date:        15. Octoboer 2019
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
#include "arm_common_tables.h"

#if (defined(ARM_MATH_MVEF) || defined(ARM_MATH_HELIUM) || defined(ARM_MATH_NEON) || defined(ARM_MATH_NEON_EXPERIMENTAL)) && !defined(ARM_MATH_AUTOVECTORIZE)
#include "arm_vec_math.h"
#endif

void arm_vlog_f32(
  const float32_t * pSrc,
        float32_t * pDst,
        uint32_t blockSize)
{
   uint32_t blkCnt; 

#if (defined(ARM_MATH_MVEF) || defined(ARM_MATH_HELIUM)) && !defined(ARM_MATH_AUTOVECTORIZE)
   f32x4_t src;
   f32x4_t dst;

   blkCnt = blockSize >> 2;

   while (blkCnt > 0U)
   {
      src = vld1q(pSrc);
      dst = vlogq_f32(src);
      vst1q(pDst, dst);

      pSrc += 4;
      pDst += 4;
      /* Decrement loop counter */
      blkCnt--;
   }

   blkCnt = blockSize & 3;
#else
#if (defined(ARM_MATH_NEON) || defined(ARM_MATH_NEON_EXPERIMENTAL)) && !defined(ARM_MATH_AUTOVECTORIZE)
   f32x4_t src;
   f32x4_t dst;

   blkCnt = blockSize >> 2;

   while (blkCnt > 0U)
   {
      src = vld1q_f32(pSrc);
      dst = vlogq_f32(src);
      vst1q_f32(pDst, dst);

      pSrc += 4;
      pDst += 4;
      /* Decrement loop counter */
      blkCnt--;
   }

   blkCnt = blockSize & 3;
#else
   blkCnt = blockSize;
#endif
#endif

   while (blkCnt > 0U)
   {
      /* C = log(A) */
  
      /* Calculate log and store result in destination buffer. */
      *pDst++ = logf(*pSrc++);
  
      /* Decrement loop counter */
      blkCnt--;
   }
}
