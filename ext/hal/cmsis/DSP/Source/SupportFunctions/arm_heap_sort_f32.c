/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_heap_sort_f32.c
 * Description:  Floating point heap sort
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



static void arm_heapify(float32_t * pSrc, uint32_t n, uint32_t i, uint8_t dir)
{
    /* Put all the elements of pSrc in heap order */
    uint32_t k = i; // Initialize largest/smallest as root 
    uint32_t l = 2*i + 1; // left = 2*i + 1 
    uint32_t r = 2*i + 2; // right = 2*i + 2 
    float32_t temp;

    if (l < n && dir==(pSrc[l] > pSrc[k]) )
        k = l; 

    if (r < n && dir==(pSrc[r] > pSrc[k]) )
        k = r; 

    if (k != i) 
    { 
	temp = pSrc[i];
	pSrc[i]=pSrc[k];
	pSrc[k]=temp;

        arm_heapify(pSrc, n, k, dir); 
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
   *               The heap sort algorithm is a comparison algorithm that
   *               divides the input array into a sorted and an unsorted region, 
   *               and shrinks the unsorted region by extracting the largest 
   *               element and moving it to the sorted region. A heap data 
   *               structure is used to find the maximum.
   *
   * @par          It's an in-place algorithm. In order to obtain an out-of-place
   *               function, a memcpy of the source vector is performed.
   */
void arm_heap_sort_f32(
  const arm_sort_instance_f32 * S, 
        float32_t * pSrc, 
        float32_t * pDst, 
        uint32_t blockSize)
{
    float32_t * pA;
    int32_t i;
    float32_t temp;

    if(pSrc != pDst) // out-of-place
    {   
        memcpy(pDst, pSrc, blockSize*sizeof(float32_t) );
        pA = pDst;
    }
    else
        pA = pSrc;

    // Build the heap array so that the largest value is the root
    for (i = blockSize/2 - 1; i >= 0; i--) 
        arm_heapify(pA, blockSize, i, S->dir); 

    for (i = blockSize - 1; i >= 0; i--)
    {
        // Swap
	temp = pA[i];
	pA[i] = pA[0];
        pA[0] = temp;

        // Restore heap order
	arm_heapify(pA, i, 0, S->dir);
    }
}
/**
  @} end of Sorting group
 */
