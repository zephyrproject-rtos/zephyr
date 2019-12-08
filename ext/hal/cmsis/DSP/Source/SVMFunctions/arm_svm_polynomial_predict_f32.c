/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_svm_polynomial_predict_f32.c
 * Description:  SVM Polynomial Classifier
 *
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
#include <limits.h>
#include <math.h>

#if defined(ARM_MATH_NEON) && !defined(ARM_MATH_AUTOVECTORIZE)
#include "arm_vec_math.h"
#endif

/**
 * @addtogroup groupSVM
 * @{
 */


/**
 * @brief SVM polynomial prediction
 * @param[in]    S          Pointer to an instance of the polynomial SVM structure.
 * @param[in]    in         Pointer to input vector
 * @param[out]   pResult    Decision value
 * @return none.
 *
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"
#include "arm_vec_math.h"

void arm_svm_polynomial_predict_f32(
    const arm_svm_polynomial_instance_f32 *S,
    const float32_t * in,
    int32_t * pResult)
{
        /* inlined Matrix x Vector function interleaved with dot prod */
    uint32_t        numRows = S->nbOfSupportVectors;
    uint32_t        numCols = S->vectorDimension;
    const float32_t *pSupport = S->supportVectors;
    const float32_t *pSrcA = pSupport;
    const float32_t *pInA0;
    const float32_t *pInA1;
    uint32_t         row;
    uint32_t         blkCnt;     /* loop counters */
    const float32_t *pDualCoef = S->dualCoefficients;
    float32_t       sum = S->intercept;
    f32x4_t         vSum = vdupq_n_f32(0.0f);

    row = numRows;

    /*
     * compute 4 rows in parrallel
     */
    while (row >= 4) {
        const float32_t *pInA2, *pInA3;
        float32_t const *pSrcA0Vec, *pSrcA1Vec, *pSrcA2Vec, *pSrcA3Vec, *pInVec;
        f32x4_t         vecIn, acc0, acc1, acc2, acc3;
        float32_t const *pSrcVecPtr = in;

        /*
         * Initialize the pointers to 4 consecutive MatrixA rows
         */
        pInA0 = pSrcA;
        pInA1 = pInA0 + numCols;
        pInA2 = pInA1 + numCols;
        pInA3 = pInA2 + numCols;
        /*
         * Initialize the vector pointer
         */
        pInVec = pSrcVecPtr;
        /*
         * reset accumulators
         */
        acc0 = vdupq_n_f32(0.0f);
        acc1 = vdupq_n_f32(0.0f);
        acc2 = vdupq_n_f32(0.0f);
        acc3 = vdupq_n_f32(0.0f);

        pSrcA0Vec = pInA0;
        pSrcA1Vec = pInA1;
        pSrcA2Vec = pInA2;
        pSrcA3Vec = pInA3;

        blkCnt = numCols >> 2;
        while (blkCnt > 0U) {
            f32x4_t         vecA;

            vecIn = vld1q(pInVec);
            pInVec += 4;
            vecA = vld1q(pSrcA0Vec);
            pSrcA0Vec += 4;
            acc0 = vfmaq(acc0, vecIn, vecA);
            vecA = vld1q(pSrcA1Vec);
            pSrcA1Vec += 4;
            acc1 = vfmaq(acc1, vecIn, vecA);
            vecA = vld1q(pSrcA2Vec);
            pSrcA2Vec += 4;
            acc2 = vfmaq(acc2, vecIn, vecA);
            vecA = vld1q(pSrcA3Vec);
            pSrcA3Vec += 4;
            acc3 = vfmaq(acc3, vecIn, vecA);

            blkCnt--;
        }
        /*
         * tail
         * (will be merged thru tail predication)
         */
        blkCnt = numCols & 3;
        if (blkCnt > 0U) {
            mve_pred16_t    p0 = vctp32q(blkCnt);
            f32x4_t         vecA;

            vecIn = vldrwq_z_f32(pInVec, p0);
            vecA = vldrwq_z_f32(pSrcA0Vec, p0);
            acc0 = vfmaq(acc0, vecIn, vecA);
            vecA = vldrwq_z_f32(pSrcA1Vec, p0);
            acc1 = vfmaq(acc1, vecIn, vecA);
            vecA = vldrwq_z_f32(pSrcA2Vec, p0);
            acc2 = vfmaq(acc2, vecIn, vecA);
            vecA = vldrwq_z_f32(pSrcA3Vec, p0);
            acc3 = vfmaq(acc3, vecIn, vecA);
        }
        /*
         * Sum the partial parts
         */
        f32x4_t         vtmp = vuninitializedq_f32();
        vtmp = vsetq_lane(vecAddAcrossF32Mve(acc0), vtmp, 0);
        vtmp = vsetq_lane(vecAddAcrossF32Mve(acc1), vtmp, 1);
        vtmp = vsetq_lane(vecAddAcrossF32Mve(acc2), vtmp, 2);
        vtmp = vsetq_lane(vecAddAcrossF32Mve(acc3), vtmp, 3);

        vSum = vfmaq_f32(vSum, vld1q(pDualCoef),
                             arm_vec_exponent_f32
                             (vaddq_n_f32(vmulq_n_f32(vtmp, S->gamma), S->coef0), S->degree));
        
        pDualCoef += 4;

        pSrcA += numCols * 4;
        /*
         * Decrement the row loop counter
         */
        row -= 4;
    }

    /*
     * compute 2 rows in parrallel
     */
    if (row >= 2) {
        float32_t const *pSrcA0Vec, *pSrcA1Vec, *pInVec;
        f32x4_t         vecIn, acc0, acc1;
        float32_t const *pSrcVecPtr = in;

        /*
         * Initialize the pointers to 2 consecutive MatrixA rows
         */
        pInA0 = pSrcA;
        pInA1 = pInA0 + numCols;
        /*
         * Initialize the vector pointer
         */
        pInVec = pSrcVecPtr;
        /*
         * reset accumulators
         */
        acc0 = vdupq_n_f32(0.0f);
        acc1 = vdupq_n_f32(0.0f);
        pSrcA0Vec = pInA0;
        pSrcA1Vec = pInA1;

        blkCnt = numCols >> 2;
        while (blkCnt > 0U) {
            f32x4_t         vecA;

            vecIn = vld1q(pInVec);
            pInVec += 4;
            vecA = vld1q(pSrcA0Vec);
            pSrcA0Vec += 4;
            acc0 = vfmaq(acc0, vecIn, vecA);
            vecA = vld1q(pSrcA1Vec);
            pSrcA1Vec += 4;
            acc1 = vfmaq(acc1, vecIn, vecA);

            blkCnt--;
        }
        /*
         * tail
         * (will be merged thru tail predication)
         */
        blkCnt = numCols & 3;
        if (blkCnt > 0U) {
            mve_pred16_t    p0 = vctp32q(blkCnt);
            f32x4_t         vecA;

            vecIn = vldrwq_z_f32(pInVec, p0);
            vecA = vldrwq_z_f32(pSrcA0Vec, p0);
            acc0 = vfmaq(acc0, vecIn, vecA);
            vecA = vldrwq_z_f32(pSrcA1Vec, p0);
            acc1 = vfmaq(acc1, vecIn, vecA);
        }
        /*
         * Sum the partial parts
         */
        f32x4_t         vtmp = vuninitializedq_f32();
        vtmp = vsetq_lane(vecAddAcrossF32Mve(acc0), vtmp, 0);
        vtmp = vsetq_lane(vecAddAcrossF32Mve(acc1), vtmp, 1);

        vSum = vfmaq_m_f32(vSum, vld1q(pDualCoef),
                             arm_vec_exponent_f32
                             (vaddq_n_f32(vmulq_n_f32(vtmp, S->gamma), S->coef0), S->degree), 
                             vctp32q(2));
        
        pDualCoef += 2;
        pSrcA += numCols * 2;
        row -= 2;
    }

    if (row >= 1) {
        f32x4_t         vecIn, acc0;
        float32_t const *pSrcA0Vec, *pInVec;
        float32_t const *pSrcVecPtr = in;
        /*
         * Initialize the pointers to last MatrixA row
         */
        pInA0 = pSrcA;
        /*
         * Initialize the vector pointer
         */
        pInVec = pSrcVecPtr;
        /*
         * reset accumulators
         */
        acc0 = vdupq_n_f32(0.0f);

        pSrcA0Vec = pInA0;

        blkCnt = numCols >> 2;
        while (blkCnt > 0U) {
            f32x4_t         vecA;

            vecIn = vld1q(pInVec);
            pInVec += 4;
            vecA = vld1q(pSrcA0Vec);
            pSrcA0Vec += 4;
            acc0 = vfmaq(acc0, vecIn, vecA);

            blkCnt--;
        }
        /*
         * tail
         * (will be merged thru tail predication)
         */
        blkCnt = numCols & 3;
        if (blkCnt > 0U) {
            mve_pred16_t    p0 = vctp32q(blkCnt);
            f32x4_t         vecA;

            vecIn = vldrwq_z_f32(pInVec, p0);
            vecA = vldrwq_z_f32(pSrcA0Vec, p0);
            acc0 = vfmaq(acc0, vecIn, vecA);
        }
        /*
         * Sum the partial parts
         */
        f32x4_t         vtmp = vuninitializedq_f32();
        vtmp = vsetq_lane(vecAddAcrossF32Mve(acc0), vtmp, 0);
        vSum = vfmaq_m_f32(vSum, vld1q(pDualCoef),
                             arm_vec_exponent_f32
                             (vaddq_n_f32(vmulq_n_f32(vtmp, S->gamma), S->coef0), S->degree), 
                             vctp32q(1));
    }
    sum += vecAddAcrossF32Mve(vSum);

    
    *pResult = S->classes[STEP(sum)];
}

#else
#if defined(ARM_MATH_NEON)
void arm_svm_polynomial_predict_f32(
    const arm_svm_polynomial_instance_f32 *S,
    const float32_t * in,
    int32_t * pResult)
{
    float32_t sum = S->intercept;
   
    float32_t dot;
    float32x4_t dotV; 

    float32x4_t accuma,accumb,accumc,accumd,accum;
    float32x2_t accum2;
    float32x4_t vec1;
    float32x4_t coef0 = vdupq_n_f32(S->coef0);

    float32x4_t vec2,vec2a,vec2b,vec2c,vec2d;

    uint32_t blkCnt;   
    uint32_t vectorBlkCnt;   

    const float32_t *pIn = in;

    const float32_t *pSupport = S->supportVectors;

    const float32_t *pSupporta = S->supportVectors;
    const float32_t *pSupportb;
    const float32_t *pSupportc;
    const float32_t *pSupportd;

    pSupportb = pSupporta + S->vectorDimension;
    pSupportc = pSupportb + S->vectorDimension;
    pSupportd = pSupportc + S->vectorDimension;

    const float32_t *pDualCoefs = S->dualCoefficients;

    vectorBlkCnt = S->nbOfSupportVectors >> 2;
    while (vectorBlkCnt > 0U)
    {
        accuma = vdupq_n_f32(0);
        accumb = vdupq_n_f32(0);
        accumc = vdupq_n_f32(0);
        accumd = vdupq_n_f32(0);

        pIn = in;

        blkCnt = S->vectorDimension >> 2;
        while (blkCnt > 0U)
        {
        
            vec1 = vld1q_f32(pIn);
            vec2a = vld1q_f32(pSupporta);
            vec2b = vld1q_f32(pSupportb);
            vec2c = vld1q_f32(pSupportc);
            vec2d = vld1q_f32(pSupportd);

            pIn += 4;
            pSupporta += 4;
            pSupportb += 4;
            pSupportc += 4;
            pSupportd += 4;

            accuma = vmlaq_f32(accuma, vec1,vec2a);
            accumb = vmlaq_f32(accumb, vec1,vec2b);
            accumc = vmlaq_f32(accumc, vec1,vec2c);
            accumd = vmlaq_f32(accumd, vec1,vec2d);

            blkCnt -- ;
        }
        accum2 = vpadd_f32(vget_low_f32(accuma),vget_high_f32(accuma));
        dotV = vsetq_lane_f32(vget_lane_f32(accum2, 0) + vget_lane_f32(accum2, 1),dotV,0);

        accum2 = vpadd_f32(vget_low_f32(accumb),vget_high_f32(accumb));
        dotV = vsetq_lane_f32(vget_lane_f32(accum2, 0) + vget_lane_f32(accum2, 1),dotV,1);

        accum2 = vpadd_f32(vget_low_f32(accumc),vget_high_f32(accumc));
        dotV = vsetq_lane_f32(vget_lane_f32(accum2, 0) + vget_lane_f32(accum2, 1),dotV,2);

        accum2 = vpadd_f32(vget_low_f32(accumd),vget_high_f32(accumd));
        dotV = vsetq_lane_f32(vget_lane_f32(accum2, 0) + vget_lane_f32(accum2, 1),dotV,3);


        blkCnt = S->vectorDimension & 3;
        while (blkCnt > 0U)
        {
            dotV = vsetq_lane_f32(vgetq_lane_f32(dotV,0) + *pIn * *pSupporta++, dotV,0);
            dotV = vsetq_lane_f32(vgetq_lane_f32(dotV,1) + *pIn * *pSupportb++, dotV,1);
            dotV = vsetq_lane_f32(vgetq_lane_f32(dotV,2) + *pIn * *pSupportc++, dotV,2);
            dotV = vsetq_lane_f32(vgetq_lane_f32(dotV,3) + *pIn * *pSupportd++, dotV,3);

            pIn++;

            blkCnt -- ;
        }

        vec1 = vld1q_f32(pDualCoefs);
        pDualCoefs += 4; 

        // To vectorize later
        dotV = vmulq_n_f32(dotV, S->gamma);
        dotV = vaddq_f32(dotV, coef0);

        dotV = arm_vec_exponent_f32(dotV,S->degree);

        accum = vmulq_f32(vec1,dotV);
        accum2 = vpadd_f32(vget_low_f32(accum),vget_high_f32(accum));
        sum += vget_lane_f32(accum2, 0) + vget_lane_f32(accum2, 1);

        pSupporta += 3*S->vectorDimension;
        pSupportb += 3*S->vectorDimension;
        pSupportc += 3*S->vectorDimension;
        pSupportd += 3*S->vectorDimension;

        vectorBlkCnt -- ;
    }

    pSupport = pSupporta;
    vectorBlkCnt = S->nbOfSupportVectors & 3;

    while (vectorBlkCnt > 0U)
    {
        accum = vdupq_n_f32(0);
        dot = 0.0f;
        pIn = in;

        blkCnt = S->vectorDimension >> 2;
        while (blkCnt > 0U)
        {
        
            vec1 = vld1q_f32(pIn);
            vec2 = vld1q_f32(pSupport);
            pIn += 4;
            pSupport += 4;

            accum = vmlaq_f32(accum, vec1,vec2);

            blkCnt -- ;
        }
        accum2 = vpadd_f32(vget_low_f32(accum),vget_high_f32(accum));
        dot = vget_lane_f32(accum2, 0) + vget_lane_f32(accum2, 1);


        blkCnt = S->vectorDimension & 3;
        while (blkCnt > 0U)
        {
            dot = dot + *pIn++ * *pSupport++;

            blkCnt -- ;
        }

        sum += *pDualCoefs++ * arm_exponent_f32(S->gamma * dot + S->coef0, S->degree);
        vectorBlkCnt -- ;
    }

    *pResult=S->classes[STEP(sum)];
}
#else
void arm_svm_polynomial_predict_f32(
    const arm_svm_polynomial_instance_f32 *S,
    const float32_t * in,
    int32_t * pResult)
{
    float32_t sum=S->intercept;
    float32_t dot=0;
    uint32_t i,j;
    const float32_t *pSupport = S->supportVectors;

    for(i=0; i < S->nbOfSupportVectors; i++)
    {
        dot=0;
        for(j=0; j < S->vectorDimension; j++)
        {
            dot = dot + in[j]* *pSupport++;
        }
        sum += S->dualCoefficients[i] * arm_exponent_f32(S->gamma * dot + S->coef0, S->degree);
    }

    *pResult=S->classes[STEP(sum)];
}
#endif
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */


/**
 * @} end of groupSVM group
 */
