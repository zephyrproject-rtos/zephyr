/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_selection_sort_f32.c
 * Description:  Floating point selection sort
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
   *               The Selection sort algorithm is a comparison algorithm that
   *               divides the input array into a sorted and an unsorted sublist 
   *               (initially the sorted sublist is empty and the unsorted sublist
   *               is the input array), looks for the smallest (or biggest)
   *               element in the unsorted sublist, swapping it with the leftmost
   *               one, and moving the sublists boundary one element to the right.
   *
   * @par          It's an in-place algorithm. In order to obtain an out-of-place
   *               function, a memcpy of the source vector is performed.
   */

void arm_selection_sort_f32(
  const arm_sort_instance_f32 * S, 
        float32_t * pSrc, 
        float32_t * pDst, 
        uint32_t blockSize)
{
    uint32_t i, j, k;
    uint8_t dir = S->dir;
    float32_t temp;

    float32_t * pA;

    if(pSrc != pDst) // out-of-place
    {
        memcpy(pDst, pSrc, blockSize*sizeof(float32_t) );
        pA = pDst;
    }
    else
        pA = pSrc;

    /*  Move the boundary one element to the right */
    for (i=0; i<blockSize-1; i++)
    {
        /* Initialize the minimum/maximum as the first element */
        k = i;

        /* Look in the unsorted list to find the minimum/maximum value */
        for (j=i+1; j<blockSize; j++)
        {
            if (dir==(pA[j] < pA[k]) )
            {
                /* Update value */
                k = j;
            }
        }
    
        if (k != i) 
        {
            /* Swap the minimum/maximum with the leftmost element */
            temp=pA[i];
	    pA[i]=pA[k];
	    pA[k]=temp;
        }
    }
}

/**
  @} end of Sorting group
 */
