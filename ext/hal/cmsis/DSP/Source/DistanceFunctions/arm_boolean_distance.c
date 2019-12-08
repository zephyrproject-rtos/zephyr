
/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_svm_linear_init_f32.c
 * Description:  SVM Linear Instance Initialization
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




#if defined(ARM_MATH_NEON)

#include "NEMath.h"

#endif


#define TT
#define TF 
#define FT
#define EXT _TT_TF_FT
#include "arm_boolean_distance_template.h"

#undef TT
#undef FF
#undef TF
#undef FT 
#undef EXT
#define TF 
#define FT
#define EXT _TF_FT
#include "arm_boolean_distance_template.h"

#undef TT
#undef FF
#undef TF
#undef FT 
#undef EXT
#define TT
#define FF
#define TF 
#define FT
#define EXT _TT_FF_TF_FT
#include "arm_boolean_distance_template.h"

#undef TT
#undef FF
#undef TF
#undef FT 
#undef EXT
#define TT
#define EXT _TT
#include "arm_boolean_distance_template.h"

