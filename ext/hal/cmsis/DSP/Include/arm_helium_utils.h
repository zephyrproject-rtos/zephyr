/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_helium_utils.h
 * Description:  Utility functions for Helium development
 *
 * $Date:        09. September 2019
 * $Revision:    V.1.5.1
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

#ifndef _ARM_UTILS_HELIUM_H_
#define _ARM_UTILS_HELIUM_H_

/***************************************

Definitions available for MVEF and MVEI

***************************************/
#if defined (ARM_MATH_HELIUM) || defined(ARM_MATH_MVEF) || defined(ARM_MATH_MVEI)

#define INACTIVELANE            0 /* inactive lane content */


#endif /* defined (ARM_MATH_HELIUM) || defined(ARM_MATH_MVEF) || defined(ARM_MATH_MVEI) */

/***************************************

Definitions available for MVEF only

***************************************/
#if defined (ARM_MATH_HELIUM) || defined(ARM_MATH_MVEF)

__STATIC_FORCEINLINE float32_t vecAddAcrossF32Mve(float32x4_t in)
{
    float32_t acc;

    acc = vgetq_lane(in, 0) + vgetq_lane(in, 1) +
          vgetq_lane(in, 2) + vgetq_lane(in, 3);

    return acc;
}

/* newton initial guess */
#define INVSQRT_MAGIC_F32           0x5f3759df

#define INVSQRT_NEWTON_MVE_F32(invSqrt, xHalf, xStart)\
{                                                     \
    float32x4_t tmp;                                  \
                                                      \
    /* tmp = xhalf * x * x */                         \
    tmp = vmulq(xStart, xStart);                      \
    tmp = vmulq(tmp, xHalf);                          \
    /* (1.5f - xhalf * x * x) */                      \
    tmp = vsubq(vdupq_n_f32(1.5f), tmp);              \
    /* x = x*(1.5f-xhalf*x*x); */                     \
    invSqrt = vmulq(tmp, xStart);                     \
}
#endif /* defined (ARM_MATH_HELIUM) || defined(ARM_MATH_MVEF) */

/***************************************

Definitions available for MVEI only

***************************************/
#if defined (ARM_MATH_HELIUM) || defined(ARM_MATH_MVEI)


#include "arm_common_tables.h"

/* Following functions are used to transpose matrix in f32 and q31 cases */
__STATIC_INLINE arm_status arm_mat_trans_32bit_2x2_mve(
    uint32_t * pDataSrc,
    uint32_t * pDataDest)
{
    static const uint32x4_t vecOffs = { 0, 2, 1, 3 };
    /*
     *
     * | 0   1 |   =>  |  0   2 |
     * | 2   3 |       |  1   3 |
     *
     */
    uint32x4_t vecIn = vldrwq_u32((uint32_t const *)pDataSrc);
    vstrwq_scatter_shifted_offset_u32(pDataDest, vecOffs, vecIn);

    return (ARM_MATH_SUCCESS);
}

__STATIC_INLINE arm_status arm_mat_trans_32bit_3x3_mve(
    uint32_t * pDataSrc,
    uint32_t * pDataDest)
{
    const uint32x4_t vecOffs1 = { 0, 3, 6, 1};
    const uint32x4_t vecOffs2 = { 4, 7, 2, 5};
    /*
     *
     *  | 0   1   2 |       | 0   3   6 |  4 x 32 flattened version | 0   3   6   1 |
     *  | 3   4   5 |   =>  | 1   4   7 |            =>             | 4   7   2   5 |
     *  | 6   7   8 |       | 2   5   8 |       (row major)         | 8   .   .   . |
     *
     */
    uint32x4_t vecIn1 = vldrwq_u32((uint32_t const *) pDataSrc);
    uint32x4_t vecIn2 = vldrwq_u32((uint32_t const *) &pDataSrc[4]);

    vstrwq_scatter_shifted_offset_u32(pDataDest, vecOffs1, vecIn1);
    vstrwq_scatter_shifted_offset_u32(pDataDest, vecOffs2, vecIn2);

    pDataDest[8] = pDataSrc[8];

    return (ARM_MATH_SUCCESS);
}

__STATIC_INLINE arm_status arm_mat_trans_32bit_4x4_mve(uint32_t * pDataSrc, uint32_t * pDataDest)
{
    /*
     * 4x4 Matrix transposition
     * is 4 x de-interleave operation
     *
     * 0   1   2   3       0   4   8   12
     * 4   5   6   7       1   5   9   13
     * 8   9   10  11      2   6   10  14
     * 12  13  14  15      3   7   11  15
     */

    uint32x4x4_t vecIn;

    vecIn = vld4q((uint32_t const *) pDataSrc);
    vstrwq(pDataDest, vecIn.val[0]);
    pDataDest += 4;
    vstrwq(pDataDest, vecIn.val[1]);
    pDataDest += 4;
    vstrwq(pDataDest, vecIn.val[2]);
    pDataDest += 4;
    vstrwq(pDataDest, vecIn.val[3]);

    return (ARM_MATH_SUCCESS);
}


__STATIC_INLINE arm_status arm_mat_trans_32bit_generic_mve(
    uint16_t    srcRows,
    uint16_t    srcCols,
    uint32_t  * pDataSrc,
    uint32_t  * pDataDest)
{
    uint32x4_t vecOffs;
    uint32_t  i;
    uint32_t  blkCnt;
    uint32_t const *pDataC;
    uint32_t *pDataDestR;
    uint32x4_t vecIn;

    vecOffs = vidupq_u32(0, 1);
    vecOffs = vecOffs * srcCols;

    i = srcCols;
    do
    {
        pDataC = (uint32_t const *) pDataSrc;
        pDataDestR = pDataDest;

        blkCnt = srcRows >> 2;
        while (blkCnt > 0U)
        {
            vecIn = vldrwq_gather_shifted_offset_u32(pDataC, vecOffs);
            vstrwq(pDataDestR, vecIn); 
            pDataDestR += 4;
            pDataC = pDataC + srcCols * 4;
            /*
             * Decrement the blockSize loop counter
             */
            blkCnt--;
        }

        /*
         * tail
         */
        blkCnt = srcRows & 3;
        if (blkCnt > 0U)
        {
            mve_pred16_t p0 = vctp32q(blkCnt);
            vecIn = vldrwq_gather_shifted_offset_u32(pDataC, vecOffs);
            vstrwq_p(pDataDestR, vecIn, p0);
        }

        pDataSrc += 1;
        pDataDest += srcRows;
    }
    while (--i);

    return (ARM_MATH_SUCCESS);
}

#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FAST_TABLES) || defined(ARM_TABLE_FAST_SQRT_Q31_MVE)
__STATIC_INLINE q31x4_t FAST_VSQRT_Q31(q31x4_t vecIn)
{
    q63x2_t         vecTmpLL;
    q31x4_t         vecTmp0, vecTmp1;
    q31_t           scale;
    q63_t           tmp64;
    q31x4_t         vecNrm, vecDst, vecIdx, vecSignBits;


    vecSignBits = vclsq(vecIn);
    vecSignBits = vbicq(vecSignBits, 1);
    /*
     * in = in << no_of_sign_bits;
     */
    vecNrm = vshlq(vecIn, vecSignBits);
    /*
     * index = in >> 24;
     */
    vecIdx = vecNrm >> 24;
    vecIdx = vecIdx << 1;

    vecTmp0 = vldrwq_gather_shifted_offset_s32(sqrtTable_Q31, vecIdx);

    vecIdx = vecIdx + 1;

    vecTmp1 = vldrwq_gather_shifted_offset_s32(sqrtTable_Q31, vecIdx);

    vecTmp1 = vqrdmulhq(vecTmp1, vecNrm);
    vecTmp0 = vecTmp0 - vecTmp1;
    vecTmp1 = vqrdmulhq(vecTmp0, vecTmp0);
    vecTmp1 = vqrdmulhq(vecNrm, vecTmp1);
    vecTmp1 = vdupq_n_s32(0x18000000) - vecTmp1;
    vecTmp0 = vqrdmulhq(vecTmp0, vecTmp1);
    vecTmpLL = vmullbq_int(vecNrm, vecTmp0);

    /*
     * scale elements 0, 2
     */
    scale = 26 + (vecSignBits[0] >> 1);
    tmp64 = asrl(vecTmpLL[0], scale);
    vecDst[0] = (q31_t) tmp64;

    scale = 26 + (vecSignBits[2] >> 1);
    tmp64 = asrl(vecTmpLL[1], scale);
    vecDst[2] = (q31_t) tmp64;

    vecTmpLL = vmulltq_int(vecNrm, vecTmp0);

    /*
     * scale elements 1, 3
     */
    scale = 26 + (vecSignBits[1] >> 1);
    tmp64 = asrl(vecTmpLL[0], scale);
    vecDst[1] = (q31_t) tmp64;

    scale = 26 + (vecSignBits[3] >> 1);
    tmp64 = asrl(vecTmpLL[1], scale);
    vecDst[3] = (q31_t) tmp64;
    /*
     * set negative values to 0
     */
    vecDst = vdupq_m(vecDst, 0, vcmpltq_n_s32(vecIn, 0));

    return vecDst;
}
#endif

#if !defined(ARM_DSP_CONFIG_TABLES) || defined(ARM_ALL_FAST_TABLES) || defined(ARM_TABLE_FAST_SQRT_Q15_MVE)
__STATIC_INLINE q15x8_t FAST_VSQRT_Q15(q15x8_t vecIn)
{
    q31x4_t         vecTmpLev, vecTmpLodd, vecSignL;
    q15x8_t         vecTmp0, vecTmp1;
    q15x8_t         vecNrm, vecDst, vecIdx, vecSignBits;

    vecDst = vuninitializedq_s16();

    vecSignBits = vclsq(vecIn);
    vecSignBits = vbicq(vecSignBits, 1);
    /*
     * in = in << no_of_sign_bits;
     */
    vecNrm = vshlq(vecIn, vecSignBits);

    vecIdx = vecNrm >> 8;
    vecIdx = vecIdx << 1;

    vecTmp0 = vldrhq_gather_shifted_offset_s16(sqrtTable_Q15, vecIdx);

    vecIdx = vecIdx + 1;

    vecTmp1 = vldrhq_gather_shifted_offset_s16(sqrtTable_Q15, vecIdx);

    vecTmp1 = vqrdmulhq(vecTmp1, vecNrm);
    vecTmp0 = vecTmp0 - vecTmp1;
    vecTmp1 = vqrdmulhq(vecTmp0, vecTmp0);
    vecTmp1 = vqrdmulhq(vecNrm, vecTmp1);
    vecTmp1 = vdupq_n_s16(0x1800) - vecTmp1;
    vecTmp0 = vqrdmulhq(vecTmp0, vecTmp1);

    vecSignBits = vecSignBits >> 1;

    vecTmpLev = vmullbq_int(vecNrm, vecTmp0);
    vecTmpLodd = vmulltq_int(vecNrm, vecTmp0);

    vecTmp0 = vecSignBits + 10;
    /*
     * negate sign to apply register based vshl
     */
    vecTmp0 = -vecTmp0;

    /*
     * shift even elements
     */
    vecSignL = vmovlbq(vecTmp0);
    vecTmpLev = vshlq(vecTmpLev, vecSignL);
    /*
     * shift odd elements
     */
    vecSignL = vmovltq(vecTmp0);
    vecTmpLodd = vshlq(vecTmpLodd, vecSignL);
    /*
     * merge and narrow odd and even parts
     */
    vecDst = vmovnbq_s32(vecDst, vecTmpLev);
    vecDst = vmovntq_s32(vecDst, vecTmpLodd);
    /*
     * set negative values to 0
     */
    vecDst = vdupq_m(vecDst, 0, vcmpltq_n_s16(vecIn, 0));

    return vecDst;
}
#endif

#endif /* defined (ARM_MATH_HELIUM) || defined(ARM_MATH_MVEI) */

#endif
