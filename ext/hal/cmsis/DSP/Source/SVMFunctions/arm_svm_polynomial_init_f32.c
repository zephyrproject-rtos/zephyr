/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_svm_polynomial_init_f32.c
 * Description:  SVM Polynomial Instance Initialization
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


/**
 * @addtogroup groupSVM
 * @{
 */



/**
 * @brief        SVM polynomial instance init function
 *
 * Classes are integer used as output of the function (instead of having -1,1
 * as class values).
 *
 * @param[in]    S                      points to an instance of the polynomial SVM structure.
 * @param[in]    nbOfSupportVectors     Number of support vectors
 * @param[in]    vectorDimension        Dimension of vector space
 * @param[in]    intercept              Intercept
 * @param[in]    dualCoefficients       Array of dual coefficients
 * @param[in]    supportVectors         Array of support vectors
 * @param[in]    classes                Array of 2 classes ID
 * @param[in]    degree                 Polynomial degree
 * @param[in]    coef0                  coeff0 (scikit-learn terminology)
 * @param[in]    gamma                  gamma (scikit-learn terminology)
 * @return none.
 *
 */


void arm_svm_polynomial_init_f32(arm_svm_polynomial_instance_f32 *S, 
  uint32_t nbOfSupportVectors,
  uint32_t vectorDimension,
  float32_t intercept,
  const float32_t *dualCoefficients,
  const float32_t *supportVectors,
  const int32_t *classes,
  int32_t      degree,
  float32_t coef0,
  float32_t gamma
  )
{
   S->nbOfSupportVectors = nbOfSupportVectors;
   S->vectorDimension = vectorDimension;
   S->intercept = intercept;
   S->dualCoefficients = dualCoefficients;
   S->supportVectors = supportVectors;
   S->classes = classes;
   S->degree = degree;
   S->coef0 = coef0;
   S->gamma = gamma;
}



/**
 * @} end of groupSVM group
 */
