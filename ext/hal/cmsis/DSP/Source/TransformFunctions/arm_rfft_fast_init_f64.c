/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cfft_init_f64.c
 * Description:  Split Radix Decimation in Frequency CFFT Double Precision Floating point processing function
 *
 * $Date:        29. November 2019
 * $Revision:    V1.0.0
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
  @addtogroup RealFFT
  @{
 */

#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_16) && defined(ARM_TABLE_BITREVIDX_FLT64_16) && defined(ARM_TABLE_TWIDDLECOEF_F64_16) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_32))

/**
  @brief         Initialization function for the 32pt double precision floating-point real FFT.
  @param[in,out] S  points to an arm_rfft_fast_instance_f64 structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS        : Operation successful
                   - \ref ARM_MATH_ARGUMENT_ERROR : an error is detected
 */

arm_status arm_rfft_32_fast_init_f64( arm_rfft_fast_instance_f64 * S ) {

  arm_cfft_instance_f64 * Sint;

  if( !S ) return ARM_MATH_ARGUMENT_ERROR;

  Sint = &(S->Sint);
  Sint->fftLen = 16U;
  S->fftLenRFFT = 32U;

  Sint->bitRevLength = ARMBITREVINDEXTABLEF64_16_TABLE_LENGTH;
  Sint->pBitRevTable = (uint16_t *)armBitRevIndexTableF64_16;
  Sint->pTwiddle     = (float64_t *) twiddleCoefF64_16;
  S->pTwiddleRFFT    = (float64_t *) twiddleCoefF64_rfft_32;

  return ARM_MATH_SUCCESS;
}
#endif

#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_32) && defined(ARM_TABLE_BITREVIDX_FLT64_32) && defined(ARM_TABLE_TWIDDLECOEF_F64_32) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_64))

/**
  @brief         Initialization function for the 64pt Double Precision floating-point real FFT.
  @param[in,out] S  points to an arm_rfft_fast_instance_f64 structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS        : Operation successful
                   - \ref ARM_MATH_ARGUMENT_ERROR : an error is detected
 */

arm_status arm_rfft_64_fast_init_f64( arm_rfft_fast_instance_f64 * S ) {

  arm_cfft_instance_f64 * Sint;

  if( !S ) return ARM_MATH_ARGUMENT_ERROR;

  Sint = &(S->Sint);
  Sint->fftLen = 32U;
  S->fftLenRFFT = 64U;

  Sint->bitRevLength = ARMBITREVINDEXTABLEF64_32_TABLE_LENGTH;
  Sint->pBitRevTable = (uint16_t *)armBitRevIndexTableF64_32;
  Sint->pTwiddle     = (float64_t *) twiddleCoefF64_32;
  S->pTwiddleRFFT    = (float64_t *) twiddleCoefF64_rfft_64;

  return ARM_MATH_SUCCESS;
}
#endif

#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_64) && defined(ARM_TABLE_BITREVIDX_FLT64_64) && defined(ARM_TABLE_TWIDDLECOEF_F64_64) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_128))

/**
  @brief         Initialization function for the 128pt Double Precision floating-point real FFT.
  @param[in,out] S  points to an arm_rfft_fast_instance_f64 structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS        : Operation successful
                   - \ref ARM_MATH_ARGUMENT_ERROR : an error is detected
 */

arm_status arm_rfft_128_fast_init_f64( arm_rfft_fast_instance_f64 * S ) {

  arm_cfft_instance_f64 * Sint;

  if( !S ) return ARM_MATH_ARGUMENT_ERROR;

  Sint = &(S->Sint);
  Sint->fftLen = 64U;
  S->fftLenRFFT = 128U;

  Sint->bitRevLength = ARMBITREVINDEXTABLEF64_64_TABLE_LENGTH;
  Sint->pBitRevTable = (uint16_t *)armBitRevIndexTableF64_64;
  Sint->pTwiddle     = (float64_t *) twiddleCoefF64_64;
  S->pTwiddleRFFT    = (float64_t *) twiddleCoefF64_rfft_128;

  return ARM_MATH_SUCCESS;
}
#endif

#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_128) && defined(ARM_TABLE_BITREVIDX_FLT64_128) && defined(ARM_TABLE_TWIDDLECOEF_F64_128) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_256))

/**
  @brief         Initialization function for the 256pt Double Precision floating-point real FFT.
  @param[in,out] S  points to an arm_rfft_fast_instance_f64 structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS        : Operation successful
                   - \ref ARM_MATH_ARGUMENT_ERROR : an error is detected
*/

arm_status arm_rfft_256_fast_init_f64( arm_rfft_fast_instance_f64 * S ) {

  arm_cfft_instance_f64 * Sint;

  if( !S ) return ARM_MATH_ARGUMENT_ERROR;

  Sint = &(S->Sint);
  Sint->fftLen = 128U;
  S->fftLenRFFT = 256U;

  Sint->bitRevLength = ARMBITREVINDEXTABLEF64_128_TABLE_LENGTH;
  Sint->pBitRevTable = (uint16_t *)armBitRevIndexTableF64_128;
  Sint->pTwiddle     = (float64_t *) twiddleCoefF64_128;
  S->pTwiddleRFFT    = (float64_t *) twiddleCoefF64_rfft_256;

  return ARM_MATH_SUCCESS;
}
#endif

#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_256) && defined(ARM_TABLE_BITREVIDX_FLT64_256) && defined(ARM_TABLE_TWIDDLECOEF_F64_256) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_512))

/**
  @brief         Initialization function for the 512pt Double Precision floating-point real FFT.
  @param[in,out] S  points to an arm_rfft_fast_instance_f64 structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS        : Operation successful
                   - \ref ARM_MATH_ARGUMENT_ERROR : an error is detected
 */

arm_status arm_rfft_512_fast_init_f64( arm_rfft_fast_instance_f64 * S ) {

  arm_cfft_instance_f64 * Sint;

  if( !S ) return ARM_MATH_ARGUMENT_ERROR;

  Sint = &(S->Sint);
  Sint->fftLen = 256U;
  S->fftLenRFFT = 512U;

  Sint->bitRevLength = ARMBITREVINDEXTABLEF64_256_TABLE_LENGTH;
  Sint->pBitRevTable = (uint16_t *)armBitRevIndexTableF64_256;
  Sint->pTwiddle     = (float64_t *) twiddleCoefF64_256;
  S->pTwiddleRFFT    = (float64_t *) twiddleCoefF64_rfft_512;

  return ARM_MATH_SUCCESS;
}
#endif

#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_512) && defined(ARM_TABLE_BITREVIDX_FLT64_512) && defined(ARM_TABLE_TWIDDLECOEF_F64_512) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_1024))
/**
  @brief         Initialization function for the 1024pt Double Precision floating-point real FFT.
  @param[in,out] S  points to an arm_rfft_fast_instance_f64 structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS        : Operation successful
                   - \ref ARM_MATH_ARGUMENT_ERROR : an error is detected
 */

arm_status arm_rfft_1024_fast_init_f64( arm_rfft_fast_instance_f64 * S ) {

  arm_cfft_instance_f64 * Sint;

  if( !S ) return ARM_MATH_ARGUMENT_ERROR;

  Sint = &(S->Sint);
  Sint->fftLen = 512U;
  S->fftLenRFFT = 1024U;

  Sint->bitRevLength = ARMBITREVINDEXTABLEF64_512_TABLE_LENGTH;
  Sint->pBitRevTable = (uint16_t *)armBitRevIndexTableF64_512;
  Sint->pTwiddle     = (float64_t *) twiddleCoefF64_512;
  S->pTwiddleRFFT    = (float64_t *) twiddleCoefF64_rfft_1024;

  return ARM_MATH_SUCCESS;
}
#endif

#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_1024) && defined(ARM_TABLE_BITREVIDX_FLT64_1024) && defined(ARM_TABLE_TWIDDLECOEF_F64_1024) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_2048))
/**
  @brief         Initialization function for the 2048pt Double Precision floating-point real FFT.
  @param[in,out] S  points to an arm_rfft_fast_instance_f64 structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS        : Operation successful
                   - \ref ARM_MATH_ARGUMENT_ERROR : an error is detected
 */
arm_status arm_rfft_2048_fast_init_f64( arm_rfft_fast_instance_f64 * S ) {

  arm_cfft_instance_f64 * Sint;

  if( !S ) return ARM_MATH_ARGUMENT_ERROR;

  Sint = &(S->Sint);
  Sint->fftLen = 1024U;
  S->fftLenRFFT = 2048U;

  Sint->bitRevLength = ARMBITREVINDEXTABLEF64_1024_TABLE_LENGTH;
  Sint->pBitRevTable = (uint16_t *)armBitRevIndexTableF64_1024;
  Sint->pTwiddle     = (float64_t *) twiddleCoefF64_1024;
  S->pTwiddleRFFT    = (float64_t *) twiddleCoefF64_rfft_2048;

  return ARM_MATH_SUCCESS;
}
#endif

#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_2048) && defined(ARM_TABLE_BITREVIDX_FLT64_2048) && defined(ARM_TABLE_TWIDDLECOEF_F64_2048) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_4096))
/**
* @brief         Initialization function for the 4096pt Double Precision floating-point real FFT.
* @param[in,out] S  points to an arm_rfft_fast_instance_f64 structure
  @return        execution status
                   - \ref ARM_MATH_SUCCESS        : Operation successful
                   - \ref ARM_MATH_ARGUMENT_ERROR : an error is detected
 */

arm_status arm_rfft_4096_fast_init_f64( arm_rfft_fast_instance_f64 * S ) {

  arm_cfft_instance_f64 * Sint;

  if( !S ) return ARM_MATH_ARGUMENT_ERROR;

  Sint = &(S->Sint);
  Sint->fftLen = 2048U;
  S->fftLenRFFT = 4096U;

  Sint->bitRevLength = ARMBITREVINDEXTABLEF64_2048_TABLE_LENGTH;
  Sint->pBitRevTable = (uint16_t *)armBitRevIndexTableF64_2048;
  Sint->pTwiddle     = (float64_t *) twiddleCoefF64_2048;
  S->pTwiddleRFFT    = (float64_t *) twiddleCoefF64_rfft_4096;

  return ARM_MATH_SUCCESS;
}
#endif

/**
  @brief         Initialization function for the Double Precision floating-point real FFT.
  @param[in,out] S       points to an arm_rfft_fast_instance_f64 structure
  @param[in]     fftLen  length of the Real Sequence
  @return        execution status
                   - \ref ARM_MATH_SUCCESS        : Operation successful
                   - \ref ARM_MATH_ARGUMENT_ERROR : <code>fftLen</code> is not a supported length

  @par           Description
                   The parameter <code>fftLen</code> specifies the length of RFFT/CIFFT process.
                   Supported FFT Lengths are 32, 64, 128, 256, 512, 1024, 2048, 4096.
  @par
                   This Function also initializes Twiddle factor table pointer and Bit reversal table pointer.
 */

arm_status arm_rfft_fast_init_f64(
  arm_rfft_fast_instance_f64 * S,
  uint16_t fftLen)
{
  typedef arm_status(*fft_init_ptr)( arm_rfft_fast_instance_f64 *);
  fft_init_ptr fptr = 0x0;

  switch (fftLen)
  {
#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_2048) && defined(ARM_TABLE_BITREVIDX_FLT64_2048) && defined(ARM_TABLE_TWIDDLECOEF_F64_2048) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_4096))
  case 4096U:
    fptr = arm_rfft_4096_fast_init_f64;
    break;
#endif
#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_1024) && defined(ARM_TABLE_BITREVIDX_FLT64_1024) && defined(ARM_TABLE_TWIDDLECOEF_F64_1024) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_2048))
  case 2048U:
    fptr = arm_rfft_2048_fast_init_f64;
    break;
#endif
#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_512) && defined(ARM_TABLE_BITREVIDX_FLT64_512) && defined(ARM_TABLE_TWIDDLECOEF_F64_512) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_1024))
  case 1024U:
    fptr = arm_rfft_1024_fast_init_f64;
    break;
#endif
#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_256) && defined(ARM_TABLE_BITREVIDX_FLT64_256) && defined(ARM_TABLE_TWIDDLECOEF_F64_256) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_512))
  case 512U:
    fptr = arm_rfft_512_fast_init_f64;
    break;
#endif
#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_128) && defined(ARM_TABLE_BITREVIDX_FLT64_128) && defined(ARM_TABLE_TWIDDLECOEF_F64_128) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_256))
  case 256U:
    fptr = arm_rfft_256_fast_init_f64;
    break;
#endif
#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_64) && defined(ARM_TABLE_BITREVIDX_FLT64_64) && defined(ARM_TABLE_TWIDDLECOEF_F64_64) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_128))
  case 128U:
    fptr = arm_rfft_128_fast_init_f64;
    break;
#endif
#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_32) && defined(ARM_TABLE_BITREVIDX_FLT64_32) && defined(ARM_TABLE_TWIDDLECOEF_F64_32) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_64))
  case 64U:
    fptr = arm_rfft_64_fast_init_f64;
    break;
#endif
#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FFT_TABLES) || (defined(ARM_TABLE_TWIDDLECOEF_F64_16) && defined(ARM_TABLE_BITREVIDX_FLT64_16) && defined(ARM_TABLE_TWIDDLECOEF_F64_16) && defined(ARM_TABLE_TWIDDLECOEF_RFFT_F64_32))
  case 32U:
    fptr = arm_rfft_32_fast_init_f64;
    break;
#endif
  default:
    return ARM_MATH_ARGUMENT_ERROR;
  }

  if( ! fptr ) return ARM_MATH_ARGUMENT_ERROR;
  return fptr( S );

}

/**
  @} end of RealFFT group
 */
