/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_spline_interp_init_f32.c
 * Description:  Floating-point cubic spline initialization function
 *
 * $Date:        13 November 2019
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
 * @ingroup groupInterpolation
 */

/**
  @addtogroup SplineInterpolate
  @{
 */

/**
  * @brief Initialization function for the floating-point cubic spline interpolation.
  * @param[in,out] S        points to an instance of the floating-point spline structure.
  * @param[in]     n        number of known data points (must > 1).
  * @param[in]     type     type of cubic spline interpolation (boundary conditions)
  */

void arm_spline_init_f32(
    arm_spline_instance_f32 * S, 
    uint32_t n, 
    arm_spline_type type)
{
    S->n_x    = n;
    S->type   = type;

    /* Type (boundary conditions):
     *   0: Natural spline          ( S1''(x1)=0 ; Sn''(xn)=0 )
     *   1: Parabolic runout spline ( S1''(x1)=S2''(x2) ; Sn-1''(xn-1)=Sn''(xn) )
     */
}

/**
 *   @} end of SplineInterpolate group
 */
