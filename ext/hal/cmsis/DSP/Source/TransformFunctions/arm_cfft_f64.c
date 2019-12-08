/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cfft_f64.c
 * Description:  Combined Radix Decimation in Frequency CFFT Double Precision Floating point processing function
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


extern void arm_radix4_butterfly_f64(
        float64_t * pSrc,
        uint16_t fftLen,
  const float64_t * pCoef,
        uint16_t twidCoefModifier);

extern void arm_bitreversal_64(
        uint64_t * pSrc,
  const uint16_t   bitRevLen,
  const uint16_t * pBitRevTable);

/**
* @} end of ComplexFFT group
*/

/* ----------------------------------------------------------------------
 * Internal helper function used by the FFTs
 * ---------------------------------------------------------------------- */

/*
* @brief  Core function for the Double Precision floating-point CFFT butterfly process.
* @param[in, out] *pSrc            points to the in-place buffer of F64 data type.
* @param[in]      fftLen           length of the FFT.
* @param[in]      *pCoef           points to the twiddle coefficient buffer.
* @param[in]      twidCoefModifier twiddle coefficient modifier that supports different size FFTs with the same twiddle factor table.
* @return none.
*/

void arm_radix4_butterfly_f64(
        float64_t * pSrc,
        uint16_t fftLen,
  const float64_t * pCoef,
        uint16_t twidCoefModifier)
{

   float64_t co1, co2, co3, si1, si2, si3;
   uint32_t ia1, ia2, ia3;
   uint32_t i0, i1, i2, i3;
   uint32_t n1, n2, j, k;

   float64_t t1, t2, r1, r2, s1, s2;


   /*  Initializations for the fft calculation */
   n2 = fftLen;
   n1 = n2;
   for (k = fftLen; k > 1U; k >>= 2U)
   {
      /*  Initializations for the fft calculation */
      n1 = n2;
      n2 >>= 2U;
      ia1 = 0U;

      /*  FFT Calculation */
      j = 0;
      do
      {
         /*  index calculation for the coefficients */
         ia2 = ia1 + ia1;
         ia3 = ia2 + ia1;
         co1 = pCoef[ia1 * 2U];
         si1 = pCoef[(ia1 * 2U) + 1U];
         co2 = pCoef[ia2 * 2U];
         si2 = pCoef[(ia2 * 2U) + 1U];
         co3 = pCoef[ia3 * 2U];
         si3 = pCoef[(ia3 * 2U) + 1U];

         /*  Twiddle coefficients index modifier */
         ia1 = ia1 + twidCoefModifier;

         i0 = j;
         do
         {
            /*  index calculation for the input as, */
            /*  pSrc[i0 + 0], pSrc[i0 + fftLen/4], pSrc[i0 + fftLen/2], pSrc[i0 + 3fftLen/4] */
            i1 = i0 + n2;
            i2 = i1 + n2;
            i3 = i2 + n2;

            /* xa + xc */
            r1 = pSrc[(2U * i0)] + pSrc[(2U * i2)];

            /* xa - xc */
            r2 = pSrc[(2U * i0)] - pSrc[(2U * i2)];

            /* ya + yc */
            s1 = pSrc[(2U * i0) + 1U] + pSrc[(2U * i2) + 1U];

            /* ya - yc */
            s2 = pSrc[(2U * i0) + 1U] - pSrc[(2U * i2) + 1U];

            /* xb + xd */
            t1 = pSrc[2U * i1] + pSrc[2U * i3];

            /* xa' = xa + xb + xc + xd */
            pSrc[2U * i0] = r1 + t1;

            /* xa + xc -(xb + xd) */
            r1 = r1 - t1;

            /* yb + yd */
            t2 = pSrc[(2U * i1) + 1U] + pSrc[(2U * i3) + 1U];

            /* ya' = ya + yb + yc + yd */
            pSrc[(2U * i0) + 1U] = s1 + t2;

            /* (ya + yc) - (yb + yd) */
            s1 = s1 - t2;

            /* (yb - yd) */
            t1 = pSrc[(2U * i1) + 1U] - pSrc[(2U * i3) + 1U];

            /* (xb - xd) */
            t2 = pSrc[2U * i1] - pSrc[2U * i3];

            /* xc' = (xa-xb+xc-xd)co2 + (ya-yb+yc-yd)(si2) */
            pSrc[2U * i1] = (r1 * co2) + (s1 * si2);

            /* yc' = (ya-yb+yc-yd)co2 - (xa-xb+xc-xd)(si2) */
            pSrc[(2U * i1) + 1U] = (s1 * co2) - (r1 * si2);

            /* (xa - xc) + (yb - yd) */
            r1 = r2 + t1;

            /* (xa - xc) - (yb - yd) */
            r2 = r2 - t1;

            /* (ya - yc) -  (xb - xd) */
            s1 = s2 - t2;

            /* (ya - yc) +  (xb - xd) */
            s2 = s2 + t2;

            /* xb' = (xa+yb-xc-yd)co1 + (ya-xb-yc+xd)(si1) */
            pSrc[2U * i2] = (r1 * co1) + (s1 * si1);

            /* yb' = (ya-xb-yc+xd)co1 - (xa+yb-xc-yd)(si1) */
            pSrc[(2U * i2) + 1U] = (s1 * co1) - (r1 * si1);

            /* xd' = (xa-yb-xc+yd)co3 + (ya+xb-yc-xd)(si3) */
            pSrc[2U * i3] = (r2 * co3) + (s2 * si3);

            /* yd' = (ya+xb-yc-xd)co3 - (xa-yb-xc+yd)(si3) */
            pSrc[(2U * i3) + 1U] = (s2 * co3) - (r2 * si3);

            i0 += n1;
         } while ( i0 < fftLen);
         j++;
      } while (j <= (n2 - 1U));
      twidCoefModifier <<= 2U;
   }
}

/*
* @brief  Core function for the Double Precision floating-point CFFT butterfly process.
* @param[in, out] *pSrc            points to the in-place buffer of F64 data type.
* @param[in]      fftLen           length of the FFT.
* @param[in]      *pCoef           points to the twiddle coefficient buffer.
* @param[in]      twidCoefModifier twiddle coefficient modifier that supports different size FFTs with the same twiddle factor table.
* @return none.
*/

void arm_cfft_radix4by2_f64(
    float64_t * pSrc,
    uint32_t fftLen,
    const float64_t * pCoef)
{
    uint32_t i, l;
    uint32_t n2, ia;
    float64_t xt, yt, cosVal, sinVal;
    float64_t p0, p1,p2,p3,a0,a1;

    n2 = fftLen >> 1;
    ia = 0;
    for (i = 0; i < n2; i++)
    {
        cosVal = pCoef[2*ia];
        sinVal = pCoef[2*ia + 1];
        ia++;

        l = i + n2;

        /*  Butterfly implementation */
        a0 = pSrc[2 * i] + pSrc[2 * l];
        xt = pSrc[2 * i] - pSrc[2 * l];

        yt = pSrc[2 * i + 1] - pSrc[2 * l + 1];
        a1 = pSrc[2 * l + 1] + pSrc[2 * i + 1];

        p0 = xt * cosVal;
        p1 = yt * sinVal;
        p2 = yt * cosVal;
        p3 = xt * sinVal;

        pSrc[2 * i]     = a0;
        pSrc[2 * i + 1] = a1;

        pSrc[2 * l]     = p0 + p1;
        pSrc[2 * l + 1] = p2 - p3;

    }

    // first col
    arm_radix4_butterfly_f64( pSrc, n2, (float64_t*)pCoef, 2U);
    // second col
    arm_radix4_butterfly_f64( pSrc + fftLen, n2, (float64_t*)pCoef, 2U);

}

/**
  @addtogroup ComplexFFT
  @{
 */

/**
  @brief         Processing function for the Double Precision floating-point complex FFT.
  @param[in]     S              points to an instance of the Double Precision floating-point CFFT structure
  @param[in,out] p1             points to the complex data buffer of size <code>2*fftLen</code>. Processing occurs in-place
  @param[in]     ifftFlag       flag that selects transform direction
                   - value = 0: forward transform
                   - value = 1: inverse transform
  @param[in]     bitReverseFlag flag that enables / disables bit reversal of output
                   - value = 0: disables bit reversal of output
                   - value = 1: enables bit reversal of output
  @return        none
 */

void arm_cfft_f64(
  const arm_cfft_instance_f64 * S,
        float64_t * p1,
        uint8_t ifftFlag,
        uint8_t bitReverseFlag)
{
    uint32_t  L = S->fftLen, l;
    float64_t invL, * pSrc;

    if (ifftFlag == 1U)
    {
        /*  Conjugate input data  */
        pSrc = p1 + 1;
        for(l=0; l<L; l++)
        {
            *pSrc = -*pSrc;
            pSrc += 2;
        }
    }

    switch (L)
    {
        case 16:
        case 64:
        case 256:
        case 1024:
        case 4096:
        arm_radix4_butterfly_f64  (p1, L, (float64_t*)S->pTwiddle, 1U);
        break;

        case 32:
        case 128:
        case 512:
        case 2048:
        arm_cfft_radix4by2_f64  ( p1, L, (float64_t*)S->pTwiddle);
        break;

    }

    if ( bitReverseFlag )
        arm_bitreversal_64((uint64_t*)p1, S->bitRevLength,S->pBitRevTable);

    if (ifftFlag == 1U)
    {
        invL = 1.0 / (float64_t)L;
        /*  Conjugate and scale output data */
        pSrc = p1;
        for(l=0; l<L; l++)
        {
            *pSrc++ *=   invL ;
            *pSrc  = -(*pSrc) * invL;
            pSrc++;
        }
    }
}

/**
  @} end of ComplexFFT group
 */
