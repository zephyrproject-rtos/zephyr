/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_naive_gaussian_bayes_predict_f32
 * Description:  Naive Gaussian Bayesian Estimator
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

#define PI_F 3.1415926535897932384626433832795f
#define DPI_F (2.0f*3.1415926535897932384626433832795f)

/**
 * @addtogroup groupBayes
 * @{
 */

/**
 * @brief Naive Gaussian Bayesian Estimator
 *
 * @param[in]  *S         points to a naive bayes instance structure
 * @param[in]  *in        points to the elements of the input vector.
 * @param[in]  *pBuffer   points to a buffer of length numberOfClasses
 * @return The predicted class
 *
 * @par If the number of classes is big, MVE version will consume lot of
 * stack since the log prior are computed on the stack.
 *
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"
#include "arm_vec_math.h"

uint32_t arm_gaussian_naive_bayes_predict_f32(const arm_gaussian_naive_bayes_instance_f32 *S, 
   const float32_t * in, 
   float32_t *pBuffer)
{
    uint32_t         nbClass;
    const float32_t *pTheta = S->theta;
    const float32_t *pSigma = S->sigma;
    float32_t      *buffer = pBuffer;
    const float32_t *pIn = in;
    float32_t       result;
    f32x4_t         vsigma;
    float32_t       tmp;
    f32x4_t         vacc1, vacc2;
    uint32_t        index;
    float32_t       logclassPriors[S->numberOfClasses];
    float32_t      *pLogPrior = logclassPriors;

    arm_vlog_f32((float32_t *) S->classPriors, logclassPriors, S->numberOfClasses);

    pTheta = S->theta;
    pSigma = S->sigma;

    for (nbClass = 0; nbClass < S->numberOfClasses; nbClass++) {
        pIn = in;

        vacc1 = vdupq_n_f32(0);
        vacc2 = vdupq_n_f32(0);

        uint32_t         blkCnt =S->vectorDimension >> 2;
        while (blkCnt > 0U) {
            f32x4_t         vinvSigma, vtmp;

            vsigma = vaddq_n_f32(vld1q(pSigma), S->epsilon);
            vacc1 = vaddq(vacc1, vlogq_f32(vmulq_n_f32(vsigma, 2.0f * PI)));

            vinvSigma = vrecip_medprec_f32(vsigma);

            vtmp = vsubq(vld1q(pIn), vld1q(pTheta));
            /* squaring */
            vtmp = vmulq(vtmp, vtmp);

            vacc2 = vfmaq(vacc2, vtmp, vinvSigma);

            pIn += 4;
            pTheta += 4;
            pSigma += 4;
            blkCnt--;
        }

        blkCnt = S->vectorDimension & 3;
        if (blkCnt > 0U) {
            mve_pred16_t    p0 = vctp32q(blkCnt);
            f32x4_t         vinvSigma, vtmp;

            vsigma = vaddq_n_f32(vld1q(pSigma), S->epsilon);
            vacc1 =
                vaddq_m_f32(vacc1, vacc1, vlogq_f32(vmulq_n_f32(vsigma, 2.0f * PI)), p0);

            vinvSigma = vrecip_medprec_f32(vsigma);

            vtmp = vsubq(vld1q(pIn), vld1q(pTheta));
            /* squaring */
            vtmp = vmulq(vtmp, vtmp);

            vacc2 = vfmaq_m_f32(vacc2, vtmp, vinvSigma, p0);

            pTheta += blkCnt;
            pSigma += blkCnt;
        }

        tmp = -0.5f * vecAddAcrossF32Mve(vacc1);
        tmp -= 0.5f * vecAddAcrossF32Mve(vacc2);

        *buffer = tmp + *pLogPrior++;
        buffer++;
    }

    arm_max_f32(pBuffer, S->numberOfClasses, &result, &index);

    return (index);
}

#else

#if defined(ARM_MATH_NEON)

#include "NEMath.h"



uint32_t arm_gaussian_naive_bayes_predict_f32(const arm_gaussian_naive_bayes_instance_f32 *S, 
   const float32_t * in, 
   float32_t *pBuffer)
{
    
    const float32_t *pPrior = S->classPriors;

    const float32_t *pTheta = S->theta;
    const float32_t *pSigma = S->sigma;

    const float32_t *pTheta1 = S->theta + S->vectorDimension;
    const float32_t *pSigma1 = S->sigma + S->vectorDimension;

    float32_t *buffer = pBuffer;
    const float32_t *pIn=in;

    float32_t result;
    float32_t sigma,sigma1;
    float32_t tmp,tmp1;
    uint32_t index;
    uint32_t vecBlkCnt;
    uint32_t classBlkCnt;
    float32x4_t epsilonV;
    float32x4_t sigmaV,sigmaV1;
    float32x4_t tmpV,tmpVb,tmpV1;
    float32x2_t tmpV2;
    float32x4_t thetaV,thetaV1;
    float32x4_t inV;

    epsilonV = vdupq_n_f32(S->epsilon);

    classBlkCnt = S->numberOfClasses >> 1;
    while(classBlkCnt > 0)
    {

        
        pIn = in;

        tmp = logf(*pPrior++);
        tmp1 = logf(*pPrior++);
        tmpV = vdupq_n_f32(0.0f);
        tmpV1 = vdupq_n_f32(0.0f);

        vecBlkCnt = S->vectorDimension >> 2;
        while(vecBlkCnt > 0)
        {
           sigmaV = vld1q_f32(pSigma);
           thetaV = vld1q_f32(pTheta);

           sigmaV1 = vld1q_f32(pSigma1);
           thetaV1 = vld1q_f32(pTheta1);

           inV = vld1q_f32(pIn);

           sigmaV = vaddq_f32(sigmaV, epsilonV);
           sigmaV1 = vaddq_f32(sigmaV1, epsilonV);

           tmpVb = vmulq_n_f32(sigmaV,DPI_F);
           tmpVb = vlogq_f32(tmpVb);
           tmpV = vmlsq_n_f32(tmpV,tmpVb,0.5f);

           tmpVb = vmulq_n_f32(sigmaV1,DPI_F);
           tmpVb = vlogq_f32(tmpVb);
           tmpV1 = vmlsq_n_f32(tmpV1,tmpVb,0.5f);
           
           tmpVb = vsubq_f32(inV,thetaV);
           tmpVb = vmulq_f32(tmpVb,tmpVb);
           tmpVb = vmulq_f32(tmpVb, vinvq_f32(sigmaV));
           tmpV = vmlsq_n_f32(tmpV,tmpVb,0.5f);

           tmpVb = vsubq_f32(inV,thetaV1);
           tmpVb = vmulq_f32(tmpVb,tmpVb);
           tmpVb = vmulq_f32(tmpVb, vinvq_f32(sigmaV1));
           tmpV1 = vmlsq_n_f32(tmpV1,tmpVb,0.5f);

           pIn += 4;
           pTheta += 4;
           pSigma += 4;
           pTheta1 += 4;
           pSigma1 += 4;

           vecBlkCnt--;
        }
        tmpV2 = vpadd_f32(vget_low_f32(tmpV),vget_high_f32(tmpV));
        tmp += vget_lane_f32(tmpV2, 0) + vget_lane_f32(tmpV2, 1);
         
        tmpV2 = vpadd_f32(vget_low_f32(tmpV1),vget_high_f32(tmpV1));
        tmp1 += vget_lane_f32(tmpV2, 0) + vget_lane_f32(tmpV2, 1);

        vecBlkCnt = S->vectorDimension & 3;
        while(vecBlkCnt > 0)
        {
           sigma = *pSigma + S->epsilon;
           sigma1 = *pSigma1 + S->epsilon;

           tmp -= 0.5f*logf(2.0f * PI_F * sigma);
           tmp -= 0.5f*(*pIn - *pTheta) * (*pIn - *pTheta) / sigma;

           tmp1 -= 0.5f*logf(2.0f * PI_F * sigma1);
           tmp1 -= 0.5f*(*pIn - *pTheta1) * (*pIn - *pTheta1) / sigma1;

           pIn++;
           pTheta++;
           pSigma++;
           pTheta1++;
           pSigma1++;
           vecBlkCnt--;
        }

        *buffer++ = tmp;
        *buffer++ = tmp1;

        pSigma += S->vectorDimension;
        pTheta += S->vectorDimension;
        pSigma1 += S->vectorDimension;
        pTheta1 += S->vectorDimension;
        
        classBlkCnt--;
    }

    classBlkCnt = S->numberOfClasses & 1;

    while(classBlkCnt > 0)
    {

        
        pIn = in;

        tmp = logf(*pPrior++);
        tmpV = vdupq_n_f32(0.0f);

        vecBlkCnt = S->vectorDimension >> 2;
        while(vecBlkCnt > 0)
        {
           sigmaV = vld1q_f32(pSigma);
           thetaV = vld1q_f32(pTheta);
           inV = vld1q_f32(pIn);

           sigmaV = vaddq_f32(sigmaV, epsilonV);

           tmpVb = vmulq_n_f32(sigmaV,DPI_F);
           tmpVb = vlogq_f32(tmpVb);
           tmpV = vmlsq_n_f32(tmpV,tmpVb,0.5f);
           
           tmpVb = vsubq_f32(inV,thetaV);
           tmpVb = vmulq_f32(tmpVb,tmpVb);
           tmpVb = vmulq_f32(tmpVb, vinvq_f32(sigmaV));
           tmpV = vmlsq_n_f32(tmpV,tmpVb,0.5f);

           pIn += 4;
           pTheta += 4;
           pSigma += 4;

           vecBlkCnt--;
        }
        tmpV2 = vpadd_f32(vget_low_f32(tmpV),vget_high_f32(tmpV));
        tmp += vget_lane_f32(tmpV2, 0) + vget_lane_f32(tmpV2, 1);

        vecBlkCnt = S->vectorDimension & 3;
        while(vecBlkCnt > 0)
        {
           sigma = *pSigma + S->epsilon;
           tmp -= 0.5f*logf(2.0f * PI_F * sigma);
           tmp -= 0.5f*(*pIn - *pTheta) * (*pIn - *pTheta) / sigma;

           pIn++;
           pTheta++;
           pSigma++;
           vecBlkCnt--;
        }

        *buffer++ = tmp;
        
        classBlkCnt--;
    }

    arm_max_f32(pBuffer,S->numberOfClasses,&result,&index);

    return(index);
}

#else

/**
 * @brief Naive Gaussian Bayesian Estimator
 *
 * @param[in]  *S         points to a naive bayes instance structure
 * @param[in]  *in        points to the elements of the input vector.
 * @param[in]  *pBuffer   points to a buffer of length numberOfClasses
 * @return The predicted class
 *
 */
uint32_t arm_gaussian_naive_bayes_predict_f32(const arm_gaussian_naive_bayes_instance_f32 *S, 
   const float32_t * in, 
   float32_t *pBuffer)
{
    uint32_t nbClass;
    uint32_t nbDim;
    const float32_t *pPrior = S->classPriors;
    const float32_t *pTheta = S->theta;
    const float32_t *pSigma = S->sigma;
    float32_t *buffer = pBuffer;
    const float32_t *pIn=in;
    float32_t result;
    float32_t sigma;
    float32_t tmp;
    float32_t acc1,acc2;
    uint32_t index;

    pTheta=S->theta;
    pSigma=S->sigma;

    for(nbClass = 0; nbClass < S->numberOfClasses; nbClass++)
    {

        
        pIn = in;

        tmp = 0.0;
        acc1 = 0.0f;
        acc2 = 0.0f;
        for(nbDim = 0; nbDim < S->vectorDimension; nbDim++)
        {
           sigma = *pSigma + S->epsilon;
           acc1 += logf(2.0f * PI_F * sigma);
           acc2 += (*pIn - *pTheta) * (*pIn - *pTheta) / sigma;

           pIn++;
           pTheta++;
           pSigma++;
        }

        tmp = -0.5f * acc1;
        tmp -= 0.5f * acc2;


        *buffer = tmp + logf(*pPrior++);
        buffer++;
    }

    arm_max_f32(pBuffer,S->numberOfClasses,&result,&index);

    return(index);
}

#endif
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
 * @} end of groupBayes group
 */
