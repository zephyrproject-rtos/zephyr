/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_merge_sort_f32.c
 * Description:  Floating point merge sort
 *
 * $Date:        2019
 * $Revision:    V1.6.0
 *
 * Target Processor: Cortex-M and Cortex-A cores
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
#include "arm_sorting.h"


static void topDownMerge(float32_t * pA, uint32_t begin, uint32_t middle, uint32_t end, float32_t * pB, uint8_t dir)
{
    /* Left  array is pA[begin:middle-1]
     * Right Array is pA[middle:end-1] 
     * They are merged in pB
     */

    uint32_t i = begin;
    uint32_t j = middle;
    uint32_t k;
 
    // Read all the elements in the sublist
    for (k = begin; k < end; k++)
    {
	// Merge 
        if (i < middle && (j >= end || dir==(pA[i] <= pA[j])) )
        {
            pB[k] = pA[i];
            i++;
        }
        else
        {
            pB[k] = pA[j];
            j++;
        }
    }
}

static void arm_merge_sort_core_f32(float32_t * pB, uint32_t begin, uint32_t end, float32_t * pA, uint8_t dir)
{
    if((int32_t)end - (int32_t)begin >= 2 )           // If run size != 1 divide
    {                                 
        int32_t middle = (end + begin) / 2;           // Take the middle point

        arm_merge_sort_core_f32(pA, begin,  middle, pB, dir);  // Sort the left part
        arm_merge_sort_core_f32(pA, middle,    end, pB, dir);  // Sort the right part

        topDownMerge(pB, begin, middle, end, pA, dir);
    }
}


/**
  @ingroup groupSupport
 */

/**
  @addtogroup Sorting
  @{
 */

/**
   * @param[in]  S          points to an instance of the sorting structure.
   * @param[in]  pSrc       points to the block of input data.
   * @param[out] pDst       points to the block of output data
   * @param[in]  blockSize  number of samples to process.
   *
   * @par        Algorithm
   *               The merge sort algorithm is a comparison algorithm that
   *               divide the input array in sublists and merge them to produce
   *               longer sorted sublists until there is only one list remaining.
   *
   * @par          A work array is always needed, hence pSrc and pDst cannot be
   *               equal and the results will be stored in pDst.
   */


void arm_merge_sort_f32(
  const arm_sort_instance_f32 * S, 
        float32_t *pSrc, 
        float32_t *pDst, 
        uint32_t blockSize)
{
    // A work array is needed: pDst
    if(pSrc != pDst) // out-of-place only
    {
	memcpy(pDst, pSrc, blockSize*sizeof(float32_t));

	arm_merge_sort_core_f32(pSrc, 0, blockSize, pDst, S->dir);
    }
}
/**
  @} end of Sorting group
 */
