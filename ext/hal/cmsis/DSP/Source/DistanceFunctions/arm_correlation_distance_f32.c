
/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_correlation_distance_f32.c
 * Description:  Correlation distance between two vectors
 *
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
#include <limits.h>
#include <math.h>



/**
  @addtogroup FloatDist
  @{
 */


/**
 * @brief        Correlation distance between two vectors
 *
 * The input vectors are modified in place !
 *
 * @param[in]    pA         First vector
 * @param[in]    pB         Second vector
 * @param[in]    blockSize  vector length
 * @return distance
 *
 */

float32_t arm_correlation_distance_f32(float32_t *pA,float32_t *pB, uint32_t blockSize)
{
    float32_t ma,mb,pwra,pwrb,dot,tmp;

    arm_mean_f32(pA, blockSize, &ma);
    arm_mean_f32(pB, blockSize, &mb);

    arm_offset_f32(pA, -ma, pA, blockSize);
    arm_offset_f32(pB, -mb, pB, blockSize);

    arm_power_f32(pA, blockSize, &pwra);
    arm_power_f32(pB, blockSize, &pwrb);

    arm_dot_prod_f32(pA,pB,blockSize,&dot);

    dot = dot / blockSize;
    pwra = pwra / blockSize;
    pwrb = pwrb / blockSize;

    arm_sqrt_f32(pwra * pwrb,&tmp);
 
    return(1.0f - dot / tmp);

   
}



/**
 * @} end of FloatDist group
 */
