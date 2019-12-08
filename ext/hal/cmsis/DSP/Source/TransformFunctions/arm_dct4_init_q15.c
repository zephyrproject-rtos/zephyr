/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_dct4_init_q15.c
 * Description:  Initialization function of DCT-4 & IDCT4 Q15
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
#include "arm_common_tables.h"

/**
  @ingroup groupTransforms
 */

 /**
  @addtogroup DCT4_IDCT4
  @{
 */

/**
  @brief         Initialization function for the Q15 DCT4/IDCT4.
  @param[in,out] S         points to an instance of Q15 DCT4/IDCT4 structure
  @param[in]     S_RFFT    points to an instance of Q15 RFFT/RIFFT structure
  @param[in]     S_CFFT    points to an instance of Q15 CFFT/CIFFT structure
  @param[in]     N          length of the DCT4
  @param[in]     Nby2       half of the length of the DCT4
  @param[in]     normalize  normalizing factor
  @return        execution status
                   - \ref ARM_MATH_SUCCESS        : Operation successful
                   - \ref ARM_MATH_ARGUMENT_ERROR : <code>N</code> is not a supported transform length

  @par           Normalizing factor
                   The normalizing factor is <code>sqrt(2/N)</code>, which depends on the size of transform <code>N</code>.
                   Normalizing factors in 1.15 format are mentioned in the table below for different DCT sizes:

                   \image html dct4NormalizingQ15Table.gif
 */

arm_status arm_dct4_init_q15(
  arm_dct4_instance_q15 * S,
  arm_rfft_instance_q15 * S_RFFT,
  arm_cfft_radix4_instance_q15 * S_CFFT,
  uint16_t N,
  uint16_t Nby2,
  q15_t normalize)
{
  /*  Initialise the default arm status */
  arm_status status = ARM_MATH_SUCCESS;

  /* Initialize the DCT4 length */
  S->N = N;

  /* Initialize the half of DCT4 length */
  S->Nby2 = Nby2;

  /* Initialize the DCT4 Normalizing factor */
  S->normalize = normalize;

  /* Initialize Real FFT Instance */
  S->pRfft = S_RFFT;

  /* Initialize Complex FFT Instance */
  S->pCfft = S_CFFT;

  switch (N)
  {
  #if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || defined(ARM_TABLE_DCT4_Q15_8192)
    /* Initialize the table modifier values */
  case 8192U:
    S->pTwiddle = WeightsQ15_8192;
    S->pCosFactor = cos_factorsQ15_8192;
    break;
  #endif

  #if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || defined(ARM_TABLE_DCT4_Q15_2048)
  case 2048U:
    S->pTwiddle = WeightsQ15_2048;
    S->pCosFactor = cos_factorsQ15_2048;
    break;
  #endif

  #if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || defined(ARM_TABLE_DCT4_Q15_512)
  case 512U:
    S->pTwiddle = WeightsQ15_512;
    S->pCosFactor = cos_factorsQ15_512;
    break;
  #endif 

  #if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || defined(ARM_TABLE_DCT4_Q15_128)
  case 128U:
    S->pTwiddle = WeightsQ15_128;
    S->pCosFactor = cos_factorsQ15_128;
    break;
  #endif 

  default:
    status = ARM_MATH_ARGUMENT_ERROR;
  }

  /* Initialize the RFFT/RIFFT */
  arm_rfft_init_q15(S->pRfft, S->N, 0U, 1U);

  /* return the status of DCT4 Init function */
  return (status);
}

/**
  @} end of DCT4_IDCT4 group
 */
