/******************************************************************************
 * @file     arm_sorting.h
 * @brief    Private header file for CMSIS DSP Library
 * @version  V1.7.0
 * @date     2019
 ******************************************************************************/
/*
 * Copyright (c) 2010-2019 Arm Limited or its affiliates. All rights reserved.
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

#ifndef _ARM_SORTING_H_
#define _ARM_SORTING_H_

#include "arm_math.h"

#ifdef   __cplusplus
extern "C"
{
#endif

  /**
   * @param[in]  S          points to an instance of the sorting structure.
   * @param[in]  pSrc       points to the block of input data.
   * @param[out] pDst       points to the block of output data.
   * @param[in]  blockSize  number of samples to process.
   */
  void arm_bubble_sort_f32(
    const arm_sort_instance_f32 * S, 
          float32_t * pSrc, 
          float32_t * pDst, 
    uint32_t blockSize);

   /**
   * @param[in]  S          points to an instance of the sorting structure.
   * @param[in]  pSrc       points to the block of input data.
   * @param[out] pDst       points to the block of output data.
   * @param[in]  blockSize  number of samples to process.
   */
  void arm_heap_sort_f32(
    const arm_sort_instance_f32 * S, 
          float32_t * pSrc, 
          float32_t * pDst, 
    uint32_t blockSize);

  /**
   * @param[in]  S          points to an instance of the sorting structure.
   * @param[in]  pSrc       points to the block of input data.
   * @param[out] pDst       points to the block of output data.
   * @param[in]  blockSize  number of samples to process.
   */
  void arm_insertion_sort_f32(
    const arm_sort_instance_f32 * S, 
          float32_t *pSrc, 
          float32_t* pDst, 
    uint32_t blockSize);

  /**
   * @param[in]  S          points to an instance of the sorting structure.
   * @param[in]  pSrc       points to the block of input data.
   * @param[out] pDst       points to the block of output data
   * @param[in]  blockSize  number of samples to process.
   */
  void arm_merge_sort_f32(
    const arm_sort_instance_f32 * S, 
          float32_t *pSrc, 
          float32_t *pDst, 
    uint32_t blockSize);

  /**
   * @param[in]  S          points to an instance of the sorting structure.
   * @param[in]  pSrc       points to the block of input data.
   * @param[out] pDst       points to the block of output data
   * @param[in]  blockSize  number of samples to process.
   */
  void arm_quick_sort_f32(
    const arm_sort_instance_f32 * S, 
          float32_t * pSrc, 
          float32_t * pDst, 
    uint32_t blockSize);

  /**
   * @param[in]  S          points to an instance of the sorting structure.
   * @param[in]  pSrc       points to the block of input data.
   * @param[out] pDst       points to the block of output data
   * @param[in]  blockSize  number of samples to process.
   */
  void arm_selection_sort_f32(
    const arm_sort_instance_f32 * S, 
          float32_t * pSrc, 
          float32_t * pDst, 
    uint32_t blockSize);
 
  /**
   * @param[in]  S          points to an instance of the sorting structure.
   * @param[in]  pSrc       points to the block of input data.
   * @param[out] pDst       points to the block of output data
   * @param[in]  blockSize  number of samples to process.
   */
  void arm_bitonic_sort_f32(
    const arm_sort_instance_f32 * S,
          float32_t * pSrc,
          float32_t * pDst,
          uint32_t blockSize);

#if defined(ARM_MATH_NEON)

#define vtrn256_128q(a, b)                   \
do {                                         \
	float32x4_t vtrn128_temp = a.val[1]; \
	a.val[1] = b.val[0];                 \
	b.val[0] = vtrn128_temp ;            \
} while (0)

#define vtrn128_64q(a, b)           \
do {                                \
	float32x2_t ab, cd, ef, gh; \
	ab = vget_low_f32(a);	    \
	ef = vget_low_f32(b);	    \
	cd = vget_high_f32(a);	    \
	gh = vget_high_f32(b);      \
	a = vcombine_f32(ab, ef);   \
	b = vcombine_f32(cd, gh);   \
} while (0)

#define vtrn256_64q(a, b)                  \
do {                                       \
	float32x2_t a_0, a_1, a_2, a_3;    \
	float32x2_t b_0, b_1, b_2, b_3;    \
	a_0 = vget_low_f32(a.val[0]);      \
	a_1 = vget_high_f32(a.val[0]);     \
	a_2 = vget_low_f32(a.val[1]);      \
	a_3 = vget_high_f32(a.val[1]);     \
	b_0 = vget_low_f32(b.val[0]);      \
	b_1 = vget_high_f32(b.val[0]);     \
	b_2 = vget_low_f32(b.val[1]);      \
	b_3 = vget_high_f32(b.val[1]);     \
	a.val[0] = vcombine_f32(a_0, b_0); \
	a.val[1] = vcombine_f32(a_2, b_2); \
	b.val[0] = vcombine_f32(a_1, b_1); \
	b.val[1] = vcombine_f32(a_3, b_3); \
} while (0) 

#define vtrn128_32q(a, b)                               \
do {                                                    \
	float32x4x2_t vtrn32_tmp = vtrnq_f32((a), (b)); \
	(a) = vtrn32_tmp.val[0];                        \
	(b) = vtrn32_tmp.val[1];                        \
} while (0)

#define vtrn256_32q(a, b)               \
do {                                    \
	float32x4x2_t vtrn32_tmp_1 = vtrnq_f32((a.val[0]), (b.val[0])); \
	float32x4x2_t vtrn32_tmp_2 = vtrnq_f32((a.val[1]), (b.val[1])); \
	a.val[0] = vtrn32_tmp_1.val[0]; \
	a.val[1] = vtrn32_tmp_2.val[0]; \
	b.val[0] = vtrn32_tmp_1.val[1]; \
	b.val[1] = vtrn32_tmp_2.val[1]; \
} while (0) 

#define vminmaxq(a, b)                    \
	do {                              \
	float32x4_t minmax_tmp = (a);     \
	(a) = vminq_f32((a), (b));        \
	(b) = vmaxq_f32(minmax_tmp, (b)); \
} while (0)

#define vminmax256q(a, b)                         \
	do {                                      \
	float32x4x2_t minmax256_tmp = (a);        \
	a.val[0] = vminq_f32(a.val[0], b.val[0]); \
	a.val[1] = vminq_f32(a.val[1], b.val[1]); \
	b.val[0] = vmaxq_f32(minmax256_tmp.val[0], b.val[0]); \
	b.val[1] = vmaxq_f32(minmax256_tmp.val[1], b.val[1]); \
} while (0)

#define vrev128q_f32(a) \
        vcombine_f32(vrev64_f32(vget_high_f32(a)), vrev64_f32(vget_low_f32(a)))

#define vrev256q_f32(a)     \
	do {                \
        float32x4_t rev_tmp = vcombine_f32(vrev64_f32(vget_high_f32(a.val[0])), vrev64_f32(vget_low_f32(a.val[0]))); \
	a.val[0] = vcombine_f32(vrev64_f32(vget_high_f32(a.val[1])), vrev64_f32(vget_low_f32(a.val[1])));  \
	a.val[1] = rev_tmp; \
} while (0)

#define vldrev128q_f32(a, p) \
	do {                 \
	a = vld1q_f32(p);    \
	a = vrev128q_f32(a); \
} while (0) 

#endif /* ARM_MATH_NEON */

#ifdef   __cplusplus
}
#endif

#endif /* _ARM_SORTING_H */
