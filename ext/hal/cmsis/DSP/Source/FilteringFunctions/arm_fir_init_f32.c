/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_fir_init_f32.c
 * Description:  Floating-point FIR filter initialization function
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
  @addtogroup FIR
  @{
 */

/**
  @brief         Initialization function for the floating-point FIR filter.
  @param[in,out] S          points to an instance of the floating-point FIR filter structure
  @param[in] 	 numTaps    number of filter coefficients in the filter
  @param[in]     pCoeffs    points to the filter coefficients buffer
  @param[in]     pState     points to the state buffer
  @param[in]     blockSize  number of samples processed per call
  @return        none

  @par           Details
                   <code>pCoeffs</code> points to the array of filter coefficients stored in time reversed order:
  <pre>
      {b[numTaps-1], b[numTaps-2], b[N-2], ..., b[1], b[0]}
  </pre>
  @par
                   <code>pState</code> points to the array of state variables.
                   <code>pState</code> is of length <code>numTaps+blockSize-1</code> samples, where <code>blockSize</code> is the number of input samples processed by each call to <code>arm_fir_f32()</code>.
  @par          Initialization of Helium version
                 For Helium version the array of coefficients must be a multiple of 16 even if less
                 then 16 coefficients are used. The additional coefficients must be set to 0.
                 It does not mean that all the coefficients will be used in the filter (numTaps
                 is still set to its right value in the init function.) It just means that
                 the implementation may require to read more coefficients due to the vectorization and
                 to avoid having to manage too many different cases in the code.

 */

void arm_fir_init_f32(
        arm_fir_instance_f32 * S,
        uint16_t numTaps,
  const float32_t * pCoeffs,
        float32_t * pState,
        uint32_t blockSize)
{
  /* Assign filter taps */
  S->numTaps = numTaps;

  /* Assign coefficient pointer */
  S->pCoeffs = pCoeffs;

  /* Clear state buffer. The size is always (blockSize + numTaps - 1) */
  memset(pState, 0, (numTaps + (blockSize - 1U)) * sizeof(float32_t));

  /* Assign state pointer */
  S->pState = pState;
}

/**
  @} end of FIR group
 */
