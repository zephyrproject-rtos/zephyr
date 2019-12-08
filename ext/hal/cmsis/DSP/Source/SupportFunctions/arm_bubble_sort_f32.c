/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_bubble_sort_f32.c
 * Description:  Floating point bubble sort
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
   *               The bubble sort algorithm is a simple comparison algorithm that
   *               reads the elements of a vector from the beginning to the end,
   *               compares the adjacent ones and swaps them if they are in the 
   *               wrong order. The procedure is repeated until there is nothing 
   *               left to swap. Bubble sort is fast for input vectors that are
   *               nearly sorted.
   *
   * @par          It's an in-place algorithm. In order to obtain an out-of-place
   *               function, a memcpy of the source vector is performed
   */

void arm_bubble_sort_f32(
  const arm_sort_instance_f32 * S, 
        float32_t * pSrc, 
        float32_t * pDst, 
        uint32_t blockSize)
{
    uint8_t dir = S->dir;
    uint32_t i;
    uint8_t swapped =1;
    float32_t * pA;
    float32_t temp;

    if(pSrc != pDst) // out-of-place
    {
	memcpy(pDst, pSrc, blockSize*sizeof(float32_t) );
	pA = pDst;
    }
    else
	pA = pSrc;

    while(swapped==1) // If nothing has been swapped after one loop stop
    {
	swapped=0;

        for(i=0; i<blockSize-1; i++)
	{
	    if(dir==(pA[i]>pA[i+1]))
	    {
		// Swap
		temp = pA[i];
		pA[i] = pA[i+1];
		pA[i+1] = temp;

		// Update flag
		swapped = 1;
	    }
	}

	blockSize--;
    }
}

/**
  @} end of Sorting group
 */
