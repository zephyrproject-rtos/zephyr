/******************************************************************************
 * @file     arm_vec_math.h
 * @brief    Public header file for CMSIS DSP Library
 * @version  V1.7.0
 * @date     15. October 2019
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

#ifndef _ARM_VEC_MATH_H
#define _ARM_VEC_MATH_H

#include "arm_math.h"
#include "arm_common_tables.h"
#include "arm_helium_utils.h"

#ifdef   __cplusplus
extern "C"
{
#endif

#if (defined(ARM_MATH_MVEF) || defined(ARM_MATH_HELIUM)) && !defined(ARM_MATH_AUTOVECTORIZE)

#define INV_NEWTON_INIT_F32         0x7EF127EA

static const float32_t __logf_rng_f32=0.693147180f;

/* fast inverse approximation (3x newton) */
__STATIC_INLINE f32x4_t vrecip_medprec_f32(
    f32x4_t x)
{
    q31x4_t         m;
    f32x4_t         b;
    any32x4_t       xinv;
    f32x4_t         ax = vabsq(x);

    xinv.f = ax;
    m = 0x3F800000 - (xinv.i & 0x7F800000);
    xinv.i = xinv.i + m;
    xinv.f = 1.41176471f - 0.47058824f * xinv.f;
    xinv.i = xinv.i + m;

    b = 2.0f - xinv.f * ax;
    xinv.f = xinv.f * b;

    b = 2.0f - xinv.f * ax;
    xinv.f = xinv.f * b;

    b = 2.0f - xinv.f * ax;
    xinv.f = xinv.f * b;

    xinv.f = vdupq_m(xinv.f, INFINITY, vcmpeqq(x, 0.0f));
    /*
     * restore sign
     */
    xinv.f = vnegq_m(xinv.f, xinv.f, vcmpltq(x, 0.0f));

    return xinv.f;
}

/* fast inverse approximation (4x newton) */
__STATIC_INLINE f32x4_t vrecip_hiprec_f32(
    f32x4_t x)
{
    q31x4_t         m;
    f32x4_t         b;
    any32x4_t       xinv;
    f32x4_t         ax = vabsq(x);

    xinv.f = ax;

    m = 0x3F800000 - (xinv.i & 0x7F800000);
    xinv.i = xinv.i + m;
    xinv.f = 1.41176471f - 0.47058824f * xinv.f;
    xinv.i = xinv.i + m;

    b = 2.0f - xinv.f * ax;
    xinv.f = xinv.f * b;

    b = 2.0f - xinv.f * ax;
    xinv.f = xinv.f * b;

    b = 2.0f - xinv.f * ax;
    xinv.f = xinv.f * b;

    b = 2.0f - xinv.f * ax;
    xinv.f = xinv.f * b;

    xinv.f = vdupq_m(xinv.f, INFINITY, vcmpeqq(x, 0.0f));
    /*
     * restore sign
     */
    xinv.f = vnegq_m(xinv.f, xinv.f, vcmpltq(x, 0.0f));

    return xinv.f;
}

__STATIC_INLINE f32x4_t vdiv_f32(
    f32x4_t num, f32x4_t den)
{
    return vmulq(num, vrecip_hiprec_f32(den));
}

/**
  @brief         Single-precision taylor dev.
  @param[in]     x              f32 quad vector input
  @param[in]     coeffs         f32 quad vector coeffs
  @return        destination    f32 quad vector
 */

__STATIC_INLINE f32x4_t vtaylor_polyq_f32(
        f32x4_t           x,
        const float32_t * coeffs)
{
    f32x4_t         A = vfmasq(vdupq_n_f32(coeffs[4]), x, coeffs[0]);
    f32x4_t         B = vfmasq(vdupq_n_f32(coeffs[6]), x, coeffs[2]);
    f32x4_t         C = vfmasq(vdupq_n_f32(coeffs[5]), x, coeffs[1]);
    f32x4_t         D = vfmasq(vdupq_n_f32(coeffs[7]), x, coeffs[3]);
    f32x4_t         x2 = vmulq(x, x);
    f32x4_t         x4 = vmulq(x2, x2);
    f32x4_t         res = vfmaq(vfmaq_f32(A, B, x2), vfmaq_f32(C, D, x2), x4);

    return res;
}

__STATIC_INLINE f32x4_t vmant_exp_f32(
    f32x4_t     x,
    int32x4_t * e)
{
    any32x4_t       r;
    int32x4_t       n;

    r.f = x;
    n = r.i >> 23;
    n = n - 127;
    r.i = r.i - (n << 23);

    *e = n;
    return r.f;
}


__STATIC_INLINE f32x4_t vlogq_f32(f32x4_t vecIn)
{
    q31x4_t         vecExpUnBiased;
    f32x4_t         vecTmpFlt0, vecTmpFlt1;
    f32x4_t         vecAcc0, vecAcc1, vecAcc2, vecAcc3;
    f32x4_t         vecExpUnBiasedFlt;

    /*
     * extract exponent
     */
    vecTmpFlt1 = vmant_exp_f32(vecIn, &vecExpUnBiased);

    vecTmpFlt0 = vecTmpFlt1 * vecTmpFlt1;
    /*
     * a = (__logf_lut_f32[4] * r.f) + (__logf_lut_f32[0]);
     */
    vecAcc0 = vdupq_n_f32(__logf_lut_f32[0]);
    vecAcc0 = vfmaq(vecAcc0, vecTmpFlt1, __logf_lut_f32[4]);
    /*
     * b = (__logf_lut_f32[6] * r.f) + (__logf_lut_f32[2]);
     */
    vecAcc1 = vdupq_n_f32(__logf_lut_f32[2]);
    vecAcc1 = vfmaq(vecAcc1, vecTmpFlt1, __logf_lut_f32[6]);
    /*
     * c = (__logf_lut_f32[5] * r.f) + (__logf_lut_f32[1]);
     */
    vecAcc2 = vdupq_n_f32(__logf_lut_f32[1]);
    vecAcc2 = vfmaq(vecAcc2, vecTmpFlt1, __logf_lut_f32[5]);
    /*
     * d = (__logf_lut_f32[7] * r.f) + (__logf_lut_f32[3]);
     */
    vecAcc3 = vdupq_n_f32(__logf_lut_f32[3]);
    vecAcc3 = vfmaq(vecAcc3, vecTmpFlt1, __logf_lut_f32[7]);
    /*
     * a = a + b * xx;
     */
    vecAcc0 = vfmaq(vecAcc0, vecAcc1, vecTmpFlt0);
    /*
     * c = c + d * xx;
     */
    vecAcc2 = vfmaq(vecAcc2, vecAcc3, vecTmpFlt0);
    /*
     * xx = xx * xx;
     */
    vecTmpFlt0 = vecTmpFlt0 * vecTmpFlt0;
    vecExpUnBiasedFlt = vcvtq_f32_s32(vecExpUnBiased);
    /*
     * r.f = a + c * xx;
     */
    vecAcc0 = vfmaq(vecAcc0, vecAcc2, vecTmpFlt0);
    /*
     * add exponent
     * r.f = r.f + ((float32_t) m) * __logf_rng_f32;
     */
    vecAcc0 = vfmaq(vecAcc0, vecExpUnBiasedFlt, __logf_rng_f32);
    // set log0 down to -inf
    vecAcc0 = vdupq_m(vecAcc0, -INFINITY, vcmpeqq(vecIn, 0.0f));
    return vecAcc0;
}

__STATIC_INLINE f32x4_t vexpq_f32(
    f32x4_t x)
{
    // Perform range reduction [-log(2),log(2)]
    int32x4_t       m = vcvtq_s32_f32(vmulq_n_f32(x, 1.4426950408f));
    f32x4_t         val = vfmsq_f32(x, vcvtq_f32_s32(m), vdupq_n_f32(0.6931471805f));

    // Polynomial Approximation
    f32x4_t         poly = vtaylor_polyq_f32(val, exp_tab);

    // Reconstruct
    poly = (f32x4_t) (vqaddq_s32((q31x4_t) (poly), vqshlq_n_s32(m, 23)));

    poly = vdupq_m(poly, 0.0f, vcmpltq_n_s32(m, -126));
    return poly;
}

__STATIC_INLINE f32x4_t arm_vec_exponent_f32(f32x4_t x, int32_t nb)
{
    f32x4_t         r = x;
    nb--;
    while (nb > 0) {
        r = vmulq(r, x);
        nb--;
    }
    return (r);
}

__STATIC_INLINE f32x4_t vrecip_f32(f32x4_t vecIn)
{
    f32x4_t     vecSx, vecW, vecTmp;
    any32x4_t   v;

    vecSx = vabsq(vecIn);

    v.f = vecIn;
    v.i = vsubq(vdupq_n_s32(INV_NEWTON_INIT_F32), v.i);

    vecW = vmulq(vecSx, v.f);

    // v.f = v.f * (8 + w * (-28 + w * (56 + w * (-70 + w *(56 + w * (-28 + w * (8 - w)))))));
    vecTmp = vsubq(vdupq_n_f32(8.0f), vecW);
    vecTmp = vfmasq(vecW, vecTmp, -28.0f);
    vecTmp = vfmasq(vecW, vecTmp, 56.0f);
    vecTmp = vfmasq(vecW, vecTmp, -70.0f);
    vecTmp = vfmasq(vecW, vecTmp, 56.0f);
    vecTmp = vfmasq(vecW, vecTmp, -28.0f);
    vecTmp = vfmasq(vecW, vecTmp, 8.0f);
    v.f = vmulq(v.f,  vecTmp);

    v.f = vdupq_m(v.f, INFINITY, vcmpeqq(vecIn, 0.0f));
    /*
     * restore sign
     */
    v.f = vnegq_m(v.f, v.f, vcmpltq(vecIn, 0.0f));
    return v.f;
}

__STATIC_INLINE f32x4_t vtanhq_f32(
    f32x4_t val)
{
    f32x4_t         x =
        vminnmq_f32(vmaxnmq_f32(val, vdupq_n_f32(-10.f)), vdupq_n_f32(10.0f));
    f32x4_t         exp2x = vexpq_f32(vmulq_n_f32(x, 2.f));
    f32x4_t         num = vsubq_n_f32(exp2x, 1.f);
    f32x4_t         den = vaddq_n_f32(exp2x, 1.f);
    f32x4_t         tanh = vmulq_f32(num, vrecip_f32(den));
    return tanh;
}

__STATIC_INLINE f32x4_t vpowq_f32(
    f32x4_t val,
    f32x4_t n)
{
    return vexpq_f32(vmulq_f32(n, vlogq_f32(val)));
}

#endif /* (defined(ARM_MATH_MVEF) || defined(ARM_MATH_HELIUM)) && !defined(ARM_MATH_AUTOVECTORIZE)*/

#if (defined(ARM_MATH_MVEI) || defined(ARM_MATH_HELIUM))
#endif /* (defined(ARM_MATH_MVEI) || defined(ARM_MATH_HELIUM)) */

#if (defined(ARM_MATH_NEON) || defined(ARM_MATH_NEON_EXPERIMENTAL)) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "NEMath.h"
/**
 * @brief Vectorized integer exponentiation
 * @param[in]    x           value
 * @param[in]    nb          integer exponent >= 1
 * @return x^nb
 *
 */
__STATIC_INLINE  float32x4_t arm_vec_exponent_f32(float32x4_t x, int32_t nb)
{
    float32x4_t r = x;
    nb --;
    while(nb > 0)
    {
        r = vmulq_f32(r , x);
        nb--;
    }
    return(r);
}


__STATIC_INLINE float32x4_t __arm_vec_sqrt_f32_neon(float32x4_t  x)
{
    float32x4_t x1 = vmaxq_f32(x, vdupq_n_f32(FLT_MIN));
    float32x4_t e = vrsqrteq_f32(x1);
    e = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x1, e), e), e);
    e = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x1, e), e), e);
    return vmulq_f32(x, e);
}

__STATIC_INLINE int16x8_t __arm_vec_sqrt_q15_neon(int16x8_t vec)
{
    float32x4_t tempF;
    int32x4_t tempHI,tempLO;

    tempLO = vmovl_s16(vget_low_s16(vec));
    tempF = vcvtq_n_f32_s32(tempLO,15);
    tempF = __arm_vec_sqrt_f32_neon(tempF);
    tempLO = vcvtq_n_s32_f32(tempF,15);

    tempHI = vmovl_s16(vget_high_s16(vec));
    tempF = vcvtq_n_f32_s32(tempHI,15);
    tempF = __arm_vec_sqrt_f32_neon(tempF);
    tempHI = vcvtq_n_s32_f32(tempF,15);

    return(vcombine_s16(vqmovn_s32(tempLO),vqmovn_s32(tempHI)));
}

__STATIC_INLINE int32x4_t __arm_vec_sqrt_q31_neon(int32x4_t vec)
{
  float32x4_t temp;

  temp = vcvtq_n_f32_s32(vec,31);
  temp = __arm_vec_sqrt_f32_neon(temp);
  return(vcvtq_n_s32_f32(temp,31));
}

#endif /*  (defined(ARM_MATH_NEON) || defined(ARM_MATH_NEON_EXPERIMENTAL)) && !defined(ARM_MATH_AUTOVECTORIZE) */

#ifdef   __cplusplus
}
#endif


#endif /* _ARM_VEC_MATH_H */

/**
 *
 * End of file.
 */
