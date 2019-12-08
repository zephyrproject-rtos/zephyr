/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_quick_sort_f32.c
 * Description:  Floating point quick sort
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



static void arm_quick_sort_core_f32(float32_t *pSrc, uint32_t first, uint32_t last, uint8_t dir)
{
    uint32_t i, j, pivot;
    float32_t temp;

    if(dir)
    {
        if(first<last)
        {
            pivot=first; // First element chosen as pivot
            i=first;
            j=last;
    
	    // Look for a pair of elements (one greater than the pivot, one 
	    // smaller) that are in the wrong order relative to each other.
            while(i<j)
            {
		// Compare left elements with pivot
                while(pSrc[i]<=pSrc[pivot] && i<last) 
                    i++; // Move to the right
    
		// Compare right elements with pivot
                while(pSrc[j]>pSrc[pivot])
                    j--; // Move to the left
    
                if(i<j)
                {
                    // Swap i <-> j
                    temp=pSrc[i];
                    pSrc[i]=pSrc[j];
                    pSrc[j]=temp;
                }
            }
 
            // Swap pivot <-> j   
            temp=pSrc[pivot];
            pSrc[pivot]=pSrc[j];
            pSrc[j]=temp;
    
            arm_quick_sort_core_f32(pSrc, first, j-1,  dir);
            arm_quick_sort_core_f32(pSrc, j+1,   last, dir);
        }
    }
    else
    {
        if(first<last)
        {
            pivot=first;
            i=first;
            j=last;
    
            while(i<j)
            {
                while(pSrc[i]>=pSrc[pivot] && i<last)
                    i++;
    
                while(pSrc[j]<pSrc[pivot])
                    j--;
    
                if(i<j)
                {
                    temp=pSrc[i];
                    pSrc[i]=pSrc[j];
                    pSrc[j]=temp;
                }
            }
    
            temp=pSrc[pivot];
            pSrc[pivot]=pSrc[j];
            pSrc[j]=temp;
    
            arm_quick_sort_core_f32(pSrc, first, j-1,  dir);
            arm_quick_sort_core_f32(pSrc, j+1,   last, dir);
        }
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
   *                The quick sort algorithm is a comparison algorithm that
   *                divides the input array into two smaller sub-arrays and 
   *                recursively sort them. An element of the array (the pivot)
   *                is chosen, all the elements with values smaller than the 
   *                pivot are moved before the pivot, while all elements with 
   *                values greater than the pivot are moved after it (partition).
   *
   * @par
   *       In this implementation the Hoare partition scheme has been 
   *       used and the first element has always been chosen as the pivot.
   *
   * @par          It's an in-place algorithm. In order to obtain an out-of-place
   *               function, a memcpy of the source vector is performed.
   */
void arm_quick_sort_f32(
  const arm_sort_instance_f32 * S, 
        float32_t * pSrc, 
        float32_t * pDst, 
        uint32_t blockSize)
{
   float32_t * pA;

   if(pSrc != pDst) // out-of-place
   {   
       memcpy(pDst, pSrc, blockSize*sizeof(float32_t) );
       pA = pDst;
   }
   else
       pA = pSrc;

    arm_quick_sort_core_f32(pA, 0, blockSize-1, S->dir);
}

/**
  @} end of Sorting group
 */
