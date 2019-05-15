/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_POWERQUAD_H_
#define _FSL_POWERQUAD_H_

#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
#include <arm_acle.h>
#elif defined(__ICCARM__)
#include <intrinsics.h>
#elif defined(__GNUC__)
#include <arm_acle.h>
#endif /* defined(__CC_ARM) */

#include "fsl_common.h"
#include "fsl_powerquad_data.h"

/*!
 * @addtogroup powerquad
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_POWERQUAD_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Version 2.0.0. */
/*@}*/

#define PQ_FLOAT32 0U
#define PQ_FIXEDPT 1U

#define CP_PQ 0U
#define CP_MTX 1U
#define CP_FFT 2U
#define CP_FIR 3U
#define CP_CORDIC 5U

#define PQ_TRANS 0U
#define PQ_TRIG 1U
#define PQ_BIQUAD 2U

#define PQ_TRANS_FIXED 4U
#define PQ_TRIG_FIXED 5U
#define PQ_BIQUAD_FIXED 6U

#define PQ_INV 0U
#define PQ_LN 1U
#define PQ_SQRT 2U
#define PQ_INVSQRT 3U
#define PQ_ETOX 4U
#define PQ_ETONX 5U
#define PQ_DIV 6U

#define PQ_SIN 0U
#define PQ_COS 1U

#define PQ_BIQ0_CALC 1U
#define PQ_BIQ1_CALC 1U

#define PQ_COMP0_ONLY (0U << 1)
#define PQ_COMP1_ONLY (1U << 1)

#define CORDIC_ITER(x) (x << 2)
#define CORDIC_MIU(x) (x << 1)
#define CORDIC_T(x) (x << 0)
#define CORDIC_ARCTAN CORDIC_T(1) | CORDIC_MIU(0)
#define CORDIC_ARCTANH CORDIC_T(1) | CORDIC_MIU(1)

#define INST_BUSY 0x80000000U

#define PQ_ERRSTAT_OVERFLOW 0U
#define PQ_ERRSTAT_NAN 1U
#define PQ_ERRSTAT_FIXEDOVERFLOW 2U
#define PQ_ERRSTAT_UNDERFLOW 3U

#define PQ_TRANS_CFFT 0U
#define PQ_TRANS_IFFT 1U
#define PQ_TRANS_CDCT 2U
#define PQ_TRANS_IDCT 3U
#define PQ_TRANS_RFFT 4U
#define PQ_TRANS_RDCT 6U

#define PQ_MTX_SCALE 1U
#define PQ_MTX_MULT 2U
#define PQ_MTX_ADD 3U
#define PQ_MTX_INV 4U
#define PQ_MTX_PROD 5U
#define PQ_MTX_SUB 7U
#define PQ_VEC_DOTP 9U
#define PQ_MTX_TRAN 10U

/* FIR engine operation type */
#define PQ_FIR_FIR 0U
#define PQ_FIR_CONVOLUTION 1U
#define PQ_FIR_CORRELATION 2U
#define PQ_FIR_INCREMENTAL 4U

#define _pq_ln0(x) __arm_mcr(CP_PQ, PQ_LN, x, PQ_FLOAT32 | PQ_COMP0_ONLY, 0, PQ_TRANS)
#define _pq_inv0(x) __arm_mcr(CP_PQ, PQ_INV, x, PQ_FLOAT32 | PQ_COMP0_ONLY, 0, PQ_TRANS)
#define _pq_sqrt0(x) __arm_mcr(CP_PQ, PQ_SQRT, x, PQ_FLOAT32 | PQ_COMP0_ONLY, 0, PQ_TRANS)
#define _pq_invsqrt0(x) __arm_mcr(CP_PQ, PQ_INVSQRT, x, PQ_FLOAT32 | PQ_COMP0_ONLY, 0, PQ_TRANS)
#define _pq_etox0(x) __arm_mcr(CP_PQ, PQ_ETOX, x, PQ_FLOAT32 | PQ_COMP0_ONLY, 0, PQ_TRANS)
#define _pq_etonx0(x) __arm_mcr(CP_PQ, PQ_ETONX, x, PQ_FLOAT32 | PQ_COMP0_ONLY, 0, PQ_TRANS)
#define _pq_sin0(x) __arm_mcr(CP_PQ, PQ_SIN, x, PQ_FLOAT32 | PQ_COMP0_ONLY, 0, PQ_TRIG)
#define _pq_cos0(x) __arm_mcr(CP_PQ, PQ_COS, x, PQ_FLOAT32 | PQ_COMP0_ONLY, 0, PQ_TRIG)
#define _pq_biquad0(x) __arm_mcr(CP_PQ, PQ_BIQ0_CALC, x, PQ_FLOAT32 | PQ_COMP0_ONLY, 0, PQ_BIQUAD)

#define _pq_ln_fx0(x) __arm_mcr(CP_PQ, PQ_LN, x, PQ_FIXEDPT | PQ_COMP0_ONLY, 0, PQ_TRANS_FIXED)
#define _pq_inv_fx0(x) __arm_mcr(CP_PQ, PQ_INV, x, PQ_FIXEDPT | PQ_COMP0_ONLY, 0, PQ_TRANS_FIXED)
#define _pq_sqrt_fx0(x) __arm_mcr(CP_PQ, PQ_SQRT, x, PQ_FIXEDPT | PQ_COMP0_ONLY, 0, PQ_TRANS_FIXED)
#define _pq_invsqrt_fx0(x) __arm_mcr(CP_PQ, PQ_INVSQRT, x, PQ_FIXEDPT | PQ_COMP0_ONLY, 0, PQ_TRANS_FIXED)
#define _pq_etox_fx0(x) __arm_mcr(CP_PQ, PQ_ETOX, x, PQ_FIXEDPT | PQ_COMP0_ONLY, 0, PQ_TRANS_FIXED)
#define _pq_etonx_fx0(x) __arm_mcr(CP_PQ, PQ_ETONX, x, PQ_FIXEDPT | PQ_COMP0_ONLY, 0, PQ_TRANS_FIXED)
#define _pq_sin_fx0(x) __arm_mcr(CP_PQ, PQ_SIN, x, PQ_FIXEDPT | PQ_COMP0_ONLY, 0, PQ_TRIG_FIXED)
#define _pq_cos_fx0(x) __arm_mcr(CP_PQ, PQ_COS, x, PQ_FIXEDPT | PQ_COMP0_ONLY, 0, PQ_TRIG_FIXED)
#define _pq_biquad0_fx(x) __arm_mcr(CP_PQ, PQ_BIQ0_CALC, x, PQ_FIXEDPT | PQ_COMP0_ONLY, 0, PQ_BIQUAD_FIXED)

#define _pq_div0(x) __arm_mcrr(CP_PQ, PQ_FLOAT32 | PQ_COMP0_ONLY, x, PQ_DIV)
#define _pq_div1(x) __arm_mcrr(CP_PQ, PQ_FLOAT32 | PQ_COMP1_ONLY, x, PQ_DIV)

#define _pq_ln1(x) __arm_mcr(CP_PQ, PQ_LN, x, PQ_FLOAT32 | PQ_COMP1_ONLY, 0, PQ_TRANS)
#define _pq_inv1(x) __arm_mcr(CP_PQ, PQ_INV, x, PQ_FLOAT32 | PQ_COMP1_ONLY, 0, PQ_TRANS)
#define _pq_sqrt1(x) __arm_mcr(CP_PQ, PQ_SQRT, x, PQ_FLOAT32 | PQ_COMP1_ONLY, 0, PQ_TRANS)
#define _pq_invsqrt1(x) __arm_mcr(CP_PQ, PQ_INVSQRT, x, PQ_FLOAT32 | PQ_COMP1_ONLY, 0, PQ_TRANS)
#define _pq_etox1(x) __arm_mcr(CP_PQ, PQ_ETOX, x, PQ_FLOAT32 | PQ_COMP1_ONLY, 0, PQ_TRANS)
#define _pq_etonx1(x) __arm_mcr(CP_PQ, PQ_ETONX, x, PQ_FLOAT32 | PQ_COMP1_ONLY, 0, PQ_TRANS)
#define _pq_sin1(x) __arm_mcr(CP_PQ, PQ_SIN, x, PQ_FLOAT32 | PQ_COMP1_ONLY, 0, PQ_TRIG)
#define _pq_cos1(x) __arm_mcr(CP_PQ, PQ_COS, x, PQ_FLOAT32 | PQ_COMP1_ONLY, 0, PQ_TRIG)
#define _pq_biquad1(x) __arm_mcr(CP_PQ, PQ_BIQ1_CALC, x, PQ_FLOAT32 | PQ_COMP1_ONLY, 0, PQ_BIQUAD)

#define _pq_ln_fx1(x) __arm_mcr(CP_PQ, PQ_LN, x, PQ_FIXEDPT | PQ_COMP1_ONLY, 0, PQ_TRANS_FIXED)
#define _pq_inv_fx1(x) __arm_mcr(CP_PQ, PQ_INV, x, PQ_FIXEDPT | PQ_COMP1_ONLY, 0, PQ_TRANS_FIXED)
#define _pq_sqrt_fx1(x) __arm_mcr(CP_PQ, PQ_SQRT, x, PQ_FIXEDPT | PQ_COMP1_ONLY, 0, PQ_TRANS_FIXED)
#define _pq_invsqrt_fx1(x) __arm_mcr(CP_PQ, PQ_INVSQRT, x, PQ_FIXEDPT | PQ_COMP1_ONLY, 0, PQ_TRANS_FIXED)
#define _pq_etox_fx1(x) __arm_mcr(CP_PQ, PQ_ETOX, x, PQ_FIXEDPT | PQ_COMP1_ONLY, 0, PQ_TRANS_FIXED)
#define _pq_etonx_fx1(x) __arm_mcr(CP_PQ, PQ_ETONX, x, PQ_FIXEDPT | PQ_COMP1_ONLY, 0, PQ_TRANS_FIXED)
#define _pq_sin_fx1(x) __arm_mcr(CP_PQ, PQ_SIN, x, PQ_FIXEDPT | PQ_COMP1_ONLY, 0, PQ_TRIG_FIXED)
#define _pq_cos_fx1(x) __arm_mcr(CP_PQ, PQ_COS, x, PQ_FIXEDPT | PQ_COMP1_ONLY, 0, PQ_TRIG_FIXED)
#define _pq_biquad1_fx(x) __arm_mcr(CP_PQ, PQ_BIQ1_CALC, x, PQ_FIXEDPT | PQ_COMP1_ONLY, 0, PQ_BIQUAD_FIXED)

#define _pq_readMult0() __arm_mrc(CP_PQ, 0, PQ_FLOAT32 | PQ_COMP0_ONLY, 0, 0)
#define _pq_readAdd0() __arm_mrc(CP_PQ, 1, PQ_FLOAT32 | PQ_COMP0_ONLY, 0, 0)
#define _pq_readMult1() __arm_mrc(CP_PQ, 0, PQ_FLOAT32 | PQ_COMP1_ONLY, 0, 0)
#define _pq_readAdd1() __arm_mrc(CP_PQ, 1, PQ_FLOAT32 | PQ_COMP1_ONLY, 0, 0)
#define _pq_readMult0_fx() __arm_mrc(CP_PQ, 0, PQ_FIXEDPT | PQ_COMP0_ONLY, 0, 0)
#define _pq_readAdd0_fx() __arm_mrc(CP_PQ, 1, PQ_FIXEDPT | PQ_COMP0_ONLY, 0, 0)
#define _pq_readMult1_fx() __arm_mrc(CP_PQ, 0, PQ_FIXEDPT | PQ_COMP1_ONLY, 0, 0)
#define _pq_readAdd1_fx() __arm_mrc(CP_PQ, 1, PQ_FIXEDPT | PQ_COMP1_ONLY, 0, 0)

/*! Parameter used for vector ln(x) */
#define PQ_LN_INF PQ_LN, 1, PQ_TRANS
/*! Parameter used for vector 1/x */
#define PQ_INV_INF PQ_INV, 0, PQ_TRANS
/*! Parameter used for vector sqrt(x) */
#define PQ_SQRT_INF PQ_SQRT, 0, PQ_TRANS
/*! Parameter used for vector 1/sqrt(x) */
#define PQ_ISQRT_INF PQ_INVSQRT, 0, PQ_TRANS
/*! Parameter used for vector e^x */
#define PQ_ETOX_INF PQ_ETOX, 0, PQ_TRANS
/*! Parameter used for vector e^(-x) */
#define PQ_ETONX_INF PQ_ETONX, 0, PQ_TRANS
/*! Parameter used for vector sin(x) */
#define PQ_SIN_INF PQ_SIN, 1, PQ_TRIG
/*! Parameter used for vector cos(x) */
#define PQ_COS_INF PQ_COS, 1, PQ_TRIG

/*
 * Register assignment for the vector calculation assembly.
 * r0: pSrc, r1: pDest, r2-r7: Data
 */

#define _PQ_RUN_FLOAT_OPCODE_R3_R2(BATCH_OPCODE, BATCH_MACHINE)                      \
    __asm volatile(                                                                  \
        "    MCR  p0,%[opcode],r3,c2,c0,%[machine] \n"                               \
        "    MCR  p0,%[opcode],r2,c0,c0,%[machine] \n" ::[opcode] "i"(BATCH_OPCODE), \
        [machine] "i"(BATCH_MACHINE))

#define _PQ_RUN_FLOAT_OPCODE_R5_R4(BATCH_OPCODE, BATCH_MACHINE)                      \
    __asm volatile(                                                                  \
        "    MCR  p0,%[opcode],r5,c2,c0,%[machine] \n"                               \
        "    MCR  p0,%[opcode],r4,c0,c0,%[machine] \n" ::[opcode] "i"(BATCH_OPCODE), \
        [machine] "i"(BATCH_MACHINE))

#define _PQ_RUN_FLOAT_OPCODE_R7_R6(BATCH_OPCODE, BATCH_MACHINE)                      \
    __asm volatile(                                                                  \
        "    MCR  p0,%[opcode],r7,c2,c0,%[machine] \n"                               \
        "    MCR  p0,%[opcode],r6,c0,c0,%[machine] \n" ::[opcode] "i"(BATCH_OPCODE), \
        [machine] "i"(BATCH_MACHINE))

/*!
 * @brief Float data vector calculation.
 *
 * Float data vector calculation, the input data should be Float and must be 8 bytes.
 *
 * @param middle  Determine if it is the first set of data, true if not.
 * @param last    Determine if it is the last set of data, true if yes.
 *
 * The last three parameters could be PQ_LN_INF, PQ_INV_INF, PQ_SQRT_INF, PQ_ISQRT_INF, PQ_ETOX_INF, PQ_ETONX_INF.
 * For example, to calculate sqrt of a vector, use like this:
 * @code
   #define VECTOR_LEN 16
   Float input[VECTOR_LEN] = {1.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
   Float output[VECTOR_LEN];

   PQ_Initiate_Vector_Func(pSrc,pDst);
   PQ_Vector8_FP(false,false,PQ_SQRT_INF);
   PQ_Vector8_FP(true,true,PQ_SQRT_INF);
   PQ_End_Vector_Func();
   @endcode
 *
 */

#define PQ_Vector8_FP(middle, last, BATCH_OPCODE, DOUBLE_READ_ADDERS, BATCH_MACHINE) \
    _PQ_RUN_FLOAT_OPCODE_R3_R2(BATCH_OPCODE, BATCH_MACHINE);                         \
    if (middle)                                                                      \
    {                                                                                \
        __asm volatile("STRD r4,r5,[r1],#8"); /* store fourth two results */         \
    }                                                                                \
    __asm volatile("LDMIA  r0!,{r4-r5}"); /* load next 2 datas */                    \
    if (DOUBLE_READ_ADDERS)                                                          \
    {                                                                                \
        __asm volatile("MRRC p0,#0,r2,r3,c1");                                       \
    }                                                                                \
    else                                                                             \
    {                                                                                \
        __asm volatile("MRRC p0,#0,r2,r3,c0");                                       \
    }                                                                                \
    _PQ_RUN_FLOAT_OPCODE_R5_R4(BATCH_OPCODE, BATCH_MACHINE);                         \
    __asm volatile("STRD r2,r3,[r1],#8"); /* store first two results */              \
    __asm volatile("LDMIA  r0!,{r6-r7}"); /* load next 2 datas */                    \
    if (DOUBLE_READ_ADDERS)                                                          \
    {                                                                                \
        __asm volatile("MRRC p0,#0,r4,r5,c1");                                       \
    }                                                                                \
    else                                                                             \
    {                                                                                \
        __asm volatile("MRRC p0,#0,r4,r5,c0");                                       \
    }                                                                                \
    _PQ_RUN_FLOAT_OPCODE_R7_R6(BATCH_OPCODE, BATCH_MACHINE);                         \
    __asm volatile("STRD r4,r5,[r1],#8"); /* store second two results */             \
    __asm volatile("LDRD r4,r5,[r0],#8"); /* load last 2 of the 8 */                 \
    if (DOUBLE_READ_ADDERS)                                                          \
    {                                                                                \
        __asm volatile("MRRC p0,#0,r6,r7,c1");                                       \
    }                                                                                \
    else                                                                             \
    {                                                                                \
        __asm volatile("MRRC p0,#0,r6,r7,c0");                                       \
    }                                                                                \
    _PQ_RUN_FLOAT_OPCODE_R5_R4(BATCH_OPCODE, BATCH_MACHINE);                         \
    __asm volatile("STRD r6,r7,[r1],#8"); /* store third two results */              \
    if (!last)                                                                       \
    {                                                                                \
        __asm volatile("LDRD r2,r3,[r0],#8"); /* load first two of next 8 */         \
    }                                                                                \
    else                                                                             \
    {                                                                                \
        __asm volatile("NOP");                                                       \
    }                                                                                \
    if (DOUBLE_READ_ADDERS)                                                          \
    {                                                                                \
        __asm volatile("MRRC p0,#0,r4,r5,c1");                                       \
    }                                                                                \
    else                                                                             \
    {                                                                                \
        __asm volatile("MRRC p0,#0,r4,r5,c0");                                       \
    }                                                                                \
    if (last)                                                                        \
    {                                                                                \
        __asm volatile("STRD r4,r5,[r1],#8"); /* store fourth two results */         \
    }

#define _PQ_RUN_FIXED32_OPCODE_R2_R3(BATCH_OPCODE, BATCH_MACHINE)                    \
    __asm volatile(                                                                  \
        "    MCR  p0,%[opcode],r2,c1,c0,%[machine] \n"                               \
        "    MCR  p0,%[opcode],r3,c3,c0,%[machine] \n" ::[opcode] "i"(BATCH_OPCODE), \
        [machine] "i"(BATCH_MACHINE))

#define _PQ_RUN_FIXED32_OPCODE_R4_R5(BATCH_OPCODE, BATCH_MACHINE)                    \
    __asm volatile(                                                                  \
        "    MCR  p0,%[opcode],r4,c1,c0,%[machine] \n"                               \
        "    MCR  p0,%[opcode],r5,c3,c0,%[machine] \n" ::[opcode] "i"(BATCH_OPCODE), \
        [machine] "i"(BATCH_MACHINE))

#define _PQ_RUN_FIXED32_OPCODE_R6_R7(BATCH_OPCODE, BATCH_MACHINE)                    \
    __asm volatile(                                                                  \
        "    MCR  p0,%[opcode],r6,c1,c0,%[machine] \n"                               \
        "    MCR  p0,%[opcode],r7,c3,c0,%[machine] \n" ::[opcode] "i"(BATCH_OPCODE), \
        [machine] "i"(BATCH_MACHINE))

/*!
 * @brief Fixed data vector calculation.
 *
 * Fixed data vector calculation, the input data should be Fixed and must be 8 bytes.
 *
 * @param middle  Determine if it is the first set of data, true if not.
 * @param last    Determine if it is the last set of data, true if yes.
 *
 * The last three parameters could be PQ_LN_INF, PQ_INV_INF, PQ_SQRT_INF, PQ_ISQRT_INF, PQ_ETOX_INF, PQ_ETONX_INF.
 * For example, to calculate sqrt of a vector, use like this:
 * @code
   #define VECTOR_LEN 16
   uint32_t input[VECTOR_LEN] = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
   uint32_t output[VECTOR_LEN];

   PQ_Initiate_Vector_Func(pSrc,pDst);
   PQ_Vector8_FX(false,false,PQ_SQRT_INF);
   PQ_Vector8_FX(true,true,PQ_SQRT_INF);
   PQ_End_Vector_Func();
   @endcode
 *
 */

#define PQ_Vector8_FX(middle, last, BATCH_OPCODE, DOUBLE_READ_ADDERS, BATCH_MACHINE) \
    _PQ_RUN_FIXED32_OPCODE_R2_R3(BATCH_OPCODE, BATCH_MACHINE);                       \
    if (middle)                                                                      \
    {                                                                                \
        __asm volatile("STRD r4,r5,[r1],#8"); /* store fourth two results */         \
    }                                                                                \
    __asm volatile("LDMIA  r0!,{r4-r7}"); /* load next 4 datas */                    \
    if (DOUBLE_READ_ADDERS)                                                          \
    {                                                                                \
        __asm volatile("MRC  p0,#0x1,r2,c1,c0,#0");                                  \
        __asm volatile("MRC  p0,#0x1,r3,c3,c0,#0");                                  \
    }                                                                                \
    else                                                                             \
    {                                                                                \
        __asm volatile("MRC  p0,#0,r2,c1,c0,#0");                                    \
        __asm volatile("MRC  p0,#0,r3,c3,c0,#0");                                    \
    }                                                                                \
    _PQ_RUN_FIXED32_OPCODE_R4_R5(BATCH_OPCODE, BATCH_MACHINE);                       \
    __asm volatile("STRD r2,r3,[r1],#8"); /* store first two results */              \
    if (DOUBLE_READ_ADDERS)                                                          \
    {                                                                                \
        __asm volatile("MRC  p0,#0x1,r4,c1,c0,#0");                                  \
        __asm volatile("MRC  p0,#0x1,r5,c3,c0,#0");                                  \
    }                                                                                \
    else                                                                             \
    {                                                                                \
        __asm volatile("MRC  p0,#0,r4,c1,c0,#0");                                    \
        __asm volatile("MRC  p0,#0,r5,c3,c0,#0");                                    \
    }                                                                                \
    _PQ_RUN_FIXED32_OPCODE_R6_R7(BATCH_OPCODE, BATCH_MACHINE);                       \
    __asm volatile("STRD r4,r5,[r1],#8"); /* store second two results */             \
    __asm volatile("LDRD r4,r5,[r0],#8"); /* load last 2 of the 8 */                 \
    if (DOUBLE_READ_ADDERS)                                                          \
    {                                                                                \
        __asm volatile("MRC  p0,#0x1,r6,c1,c0,#0");                                  \
        __asm volatile("MRC  p0,#0x1,r7,c3,c0,#0");                                  \
    }                                                                                \
    else                                                                             \
    {                                                                                \
        __asm volatile("MRC  p0,#0,r6,c1,c0,#0");                                    \
        __asm volatile("MRC  p0,#0,r7,c3,c0,#0");                                    \
    }                                                                                \
    _PQ_RUN_FIXED32_OPCODE_R4_R5(BATCH_OPCODE, BATCH_MACHINE);                       \
    __asm volatile("STRD r6,r7,[r1],#8"); /* store third two results */              \
    if (!last)                                                                       \
        __asm volatile("LDRD r2,r3,[r0],#8"); /* load first two of next 8 */         \
    if (DOUBLE_READ_ADDERS)                                                          \
    {                                                                                \
        __asm volatile("MRC  p0,#0x1,r4,c1,c0,#0");                                  \
        __asm volatile("MRC  p0,#0x1,r5,c3,c0,#0");                                  \
    }                                                                                \
    else                                                                             \
    {                                                                                \
        __asm volatile("MRC  p0,#0,r4,c1,c0,#0");                                    \
        __asm volatile("MRC  p0,#0,r5,c3,c0,#0");                                    \
    }                                                                                \
    if (last)                                                                        \
    {                                                                                \
        __asm volatile("STRD r4,r5,[r1],#8"); /* store fourth two results */         \
    }

/*!
 * @brief Start 32-bit data vector calculation.
 *
 * Start the vector calculation, the input data could be float, int32_t or Q31.
 *
 * @param PSRC  Pointer to the source data.
 * @param PDST  Pointer to the destination data.
 */
#define PQ_Initiate_Vector_Func(pSrc, pDst)              \
    __asm volatile(                                      \
        "MOV r0, %[psrc]         \n"                     \
        "MOV r1, %[pdst]         \n"                     \
        "PUSH {r2-r7}            \n"                     \
        "LDRD r2,r3,[r0],#8      \n" ::[psrc] "r"(pSrc), \
        [pdst] "r"(pDst)                                 \
        : "r0", "r1")

/*!
 * @brief End vector calculation.
 *
 * This function should be called after vector calculation.
 */
#define PQ_End_Vector_Func() __asm volatile("POP {r2-r7}")

/*
 * Register assignment for the vector calculation assembly.
 * r0: pSrc, r1: pDest, r2: length, r3: middle, r4-r9: Data, r10:dra
 */

/*!
 * @brief Start 32-bit data vector calculation.
 *
 * Start the vector calculation, the input data could be float, int32_t or Q31.
 *
 * @param PSRC  Pointer to the source data.
 * @param PDST  Pointer to the destination data.
 * @param LENGTH Number of the data, must be multiple of 8.
 */
#define PQ_StartVector(PSRC, PDST, LENGTH)               \
    __asm volatile(                                      \
        "MOV r0, %[psrc]         \n"                     \
        "MOV r1, %[pdst]         \n"                     \
        "MOV r2, %[length]       \n"                     \
        "PUSH {r3-r10}           \n"                     \
        "MOV r3, #0              \n"                     \
        "MOV r10, #0             \n"                     \
        "LDRD r4,r5,[r0],#8      \n" ::[psrc] "r"(PSRC), \
        [pdst] "r"(PDST), [length] "r"(LENGTH)           \
        : "r0", "r1", "r2")

/*!
 * @brief Start 16-bit data vector calculation.
 *
 * Start the vector calculation, the input data could be int16_t. This function
 * should be use with @ref PQ_Vector8Fixed16.
 *
 * @param PSRC  Pointer to the source data.
 * @param PDST  Pointer to the destination data.
 * @param LENGTH Number of the data, must be multiple of 8.
 */
#define PQ_StartVectorFixed16(PSRC, PDST, LENGTH)         \
    __asm volatile(                                       \
        "MOV r0, %[psrc]          \n"                     \
        "MOV r1, %[pdst]          \n"                     \
        "MOV r2, %[length]        \n"                     \
        "PUSH {r3-r10}            \n"                     \
        "MOV r3, #0               \n"                     \
        "LDRSH r4,[r0],#2         \n"                     \
        "LDRSH r5,[r0],#2         \n" ::[psrc] "r"(PSRC), \
        [pdst] "r"(PDST), [length] "r"(LENGTH)            \
        : "r0", "r1", "r2")

/*!
 * @brief Start Q15-bit data vector calculation.
 *
 * Start the vector calculation, the input data could be Q15. This function
 * should be use with @ref PQ_Vector8Q15. This function is dedicate for
 * SinQ15/CosQ15 vector calculation. Because PowerQuad only supports Q31 Sin/Cos
 * fixed function, so the input Q15 data is left shift 16 bits first, after
 * Q31 calculation, the output data is right shift 16 bits.
 *
 * @param PSRC  Pointer to the source data.
 * @param PDST  Pointer to the destination data.
 * @param LENGTH Number of the data, must be multiple of 8.
 */
#define PQ_StartVectorQ15(PSRC, PDST, LENGTH)             \
    __asm volatile(                                       \
        "MOV r0, %[psrc]          \n"                     \
        "MOV r1, %[pdst]          \n"                     \
        "MOV r2, %[length]        \n"                     \
        "PUSH {r3-r10}            \n"                     \
        "MOV r3, #0               \n"                     \
        "LDR r5,[r0],#4           \n"                     \
        "LSL r4,r5,#16            \n"                     \
        "BFC r5,#0,#16            \n" ::[psrc] "r"(PSRC), \
        [pdst] "r"(PDST), [length] "r"(LENGTH)            \
        : "r0", "r1", "r2")

/*!
 * @brief End vector calculation.
 *
 * This function should be called after vector calculation.
 */
#define PQ_EndVector() __asm volatile("POP {r3-r10}            \n")

/*!
 * @brief Float data vector calculation.
 *
 * Float data vector calculation, the input data should be float. The parameter
 * could be PQ_LN_INF, PQ_INV_INF, PQ_SQRT_INF, PQ_ISQRT_INF, PQ_ETOX_INF, PQ_ETONX_INF.
 * For example, to calculate sqrt of a vector, use like this:
 * @code
   #define VECTOR_LEN 8
   float input[VECTOR_LEN] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
   float output[VECTOR_LEN];

   PQ_StartVector(input, output, VECTOR_LEN);
   PQ_Vector8F32(PQ_SQRT_INF);
   PQ_EndVector();
   @endcode
 *
 */
#define PQ_Vector8F32(BATCH_OPCODE, DOUBLE_READ_ADDERS, BATCH_MACHINE)                     \
    __asm volatile(                                                                        \
        "1:                                        \n"                                     \
        "    MCR  p0,%[opcode],r5,c2,c0,%[machine] \n"                                     \
        "    MCR  p0,%[opcode],r4,c0,c0,%[machine] \n"                                     \
        "    CMP  r3, #0                           \n"                                     \
        "    ITE  NE                               \n"                                     \
        "    STRDNE r6,r7,[r1],#8                  \n" /* store fourth two results */      \
        "    MOVEQ r3, #1                          \n" /* middle = 1 */                    \
        "    LDMIA  r0!,{r6-r9}                    \n" /* load next 4 datas */             \
        "    MOV  r10,%[dra]                       \n"                                     \
        "    CMP  r10, #0                          \n"                                     \
        "    ITE  NE                               \n"                                     \
        "    MRRCNE  p0,#0,r4,r5,c1                \n"                                     \
        "    MRRCEQ  p0,#0,r4,r5,c0                \n"                                     \
        "    MCR  p0,%[opcode],r7,c2,c0,%[machine] \n"                                     \
        "    MCR  p0,%[opcode],r6,c0,c0,%[machine] \n"                                     \
        "    STRD r4,r5,[r1],#8                    \n" /* store first two results */       \
        "    MOV  r10,%[dra]                       \n"                                     \
        "    CMP  r10, #0                          \n"                                     \
        "    ITE  NE                               \n"                                     \
        "    MRRCNE  p0,#0,r6,r7,c1                \n"                                     \
        "    MRRCEQ  p0,#0,r6,r7,c0                \n"                                     \
        "    MCR  p0,%[opcode],r9,c2,c0,%[machine] \n"                                     \
        "    MCR  p0,%[opcode],r8,c0,c0,%[machine] \n"                                     \
        "    STRD r6,r7,[r1],#8                    \n" /* store second two results */      \
        "    LDRD r6,r7,[r0],#8                    \n" /* load last 2 of the 8 */          \
        "    CMP  r10, #0                          \n"                                     \
        "    ITE  NE                               \n"                                     \
        "    MRRCNE  p0,#0,r8,r9,c1                \n"                                     \
        "    MRRCEQ  p0,#0,r8,r9,c0                \n"                                     \
        "    MCR  p0,%[opcode],r7,c2,c0,%[machine] \n"                                     \
        "    MCR  p0,%[opcode],r6,c0,c0,%[machine] \n"                                     \
        "    STRD r8,r9,[r1],#8                    \n" /* store third two results */       \
        "    SUBS r2, r2, #8                       \n" /* length -= 8; if (length != 0) */ \
        "    IT   NE                               \n"                                     \
        "    LDRDNE r4,r5,[r0],#8                  \n" /* load first two of next 8 */      \
        "    CMP  r10, #0                          \n"                                     \
        "    ITE  NE                               \n"                                     \
        "    MRRCNE  p0,#0,r6,r7,c1                \n"                                     \
        "    MRRCEQ  p0,#0,r6,r7,c0                \n"                                     \
        "    CMP  r2, #0                           \n" /* if (length == 0) */              \
        "    BNE  1b                               \n"                                     \
        "    STRD r6,r7,[r1],#8                    \n" /* store fourth two results */      \
        ::[opcode] "i"(BATCH_OPCODE),                                                      \
        [dra] "i"(DOUBLE_READ_ADDERS), [machine] "i"(BATCH_MACHINE))

/*!
 * @brief Fixed 32bits data vector calculation.
 *
 * Float data vector calculation, the input data should be 32-bit integer. The parameter
 * could be PQ_LN_INF, PQ_INV_INF, PQ_SQRT_INF, PQ_ISQRT_INF, PQ_ETOX_INF, PQ_ETONX_INF.
 * PQ_SIN_INF, PQ_COS_INF. When this function is used for sin/cos calculation, the input
 * data should be in the format Q1.31.
 * For example, to calculate sqrt of a vector, use like this:
 * @code
   #define VECTOR_LEN 8
   int32_t input[VECTOR_LEN] = {1, 4, 9, 16, 25, 36, 49, 64};
   int32_t output[VECTOR_LEN];

   PQ_StartVector(input, output, VECTOR_LEN);
   PQ_Vector8F32(PQ_SQRT_INF);
   PQ_EndVector();
   @endcode
 *
 */
#define PQ_Vector8Fixed32(BATCH_OPCODE, DOUBLE_READ_ADDERS, BATCH_MACHINE)                 \
    __asm volatile(                                                                        \
        "1:                                        \n"                                     \
        "    MCR  p0,%[opcode],r4,c1,c0,%[machine] \n"                                     \
        "    ISB                                   \n"                                     \
        "    MCR  p0,%[opcode],r5,c3,c0,%[machine] \n"                                     \
        "    CMP  r3, #0                           \n"                                     \
        "    ITE  NE                               \n"                                     \
        "    STRDNE r6,r7,[r1],#8                  \n" /* store fourth two results */      \
        "    MOVEQ r3, #1                          \n" /* middle = 1 */                    \
        "    LDMIA  r0!,{r6-r9}                    \n" /* load next 4 datas */             \
        "    MRC  p0,%[dra],r4,c1,c0,#0            \n"                                     \
        "    MRC  p0,%[dra],r5,c3,c0,#0            \n"                                     \
        "    MCR  p0,%[opcode],r6,c1,c0,%[machine] \n"                                     \
        "    ISB                                   \n"                                     \
        "    MCR  p0,%[opcode],r7,c3,c0,%[machine] \n"                                     \
        "    STRD r4,r5,[r1],#8                    \n" /* store first two results */       \
        "    MRC  p0,%[dra],r6,c1,c0,#0            \n"                                     \
        "    MRC  p0,%[dra],r7,c3,c0,#0            \n"                                     \
        "    MCR  p0,%[opcode],r8,c1,c0,%[machine] \n"                                     \
        "    ISB                                   \n"                                     \
        "    MCR  p0,%[opcode],r9,c3,c0,%[machine] \n"                                     \
        "    STRD r6,r7,[r1],#8                    \n" /* store second two results */      \
        "    LDRD r6,r7,[r0],#8                    \n" /* load last 2 of the 8 */          \
        "    MRC  p0,%[dra],r8,c1,c0,#0            \n"                                     \
        "    MRC  p0,%[dra],r9,c3,c0,#0            \n"                                     \
        "    MCR  p0,%[opcode],r6,c1,c0,%[machine] \n"                                     \
        "    ISB                                   \n"                                     \
        "    MCR  p0,%[opcode],r7,c3,c0,%[machine] \n"                                     \
        "    STRD r8,r9,[r1],#8                    \n" /* store third two results */       \
        "    SUBS r2, r2, #8                       \n" /* length -= 8; if (length != 0) */ \
        "    IT   NE                               \n"                                     \
        "    LDRDNE r4,r5,[r0],#8                  \n" /* load first two of next 8 */      \
        "    MRC  p0,%[dra],r6,c1,c0,#0            \n"                                     \
        "    MRC  p0,%[dra],r7,c3,c0,#0            \n"                                     \
        "    CMP  r2, #0                           \n" /* if (length == 0) */              \
        "    BNE  1b                               \n"                                     \
        "    STRD r6,r7,[r1],#8                    \n" /* store fourth two results */      \
        ::[opcode] "i"(BATCH_OPCODE),                                                      \
        [dra] "i"(DOUBLE_READ_ADDERS), [machine] "i"(BATCH_MACHINE))

/*!
 * @brief Fixed 32bits data vector calculation.
 *
 * Float data vector calculation, the input data should be 16-bit integer. The parameter
 * could be PQ_LN_INF, PQ_INV_INF, PQ_SQRT_INF, PQ_ISQRT_INF, PQ_ETOX_INF, PQ_ETONX_INF.
 * For example, to calculate sqrt of a vector, use like this:
 * @code
   #define VECTOR_LEN 8
   int16_t input[VECTOR_LEN] = {1, 4, 9, 16, 25, 36, 49, 64};
   int16_t output[VECTOR_LEN];

   PQ_StartVector(input, output, VECTOR_LEN);
   PQ_Vector8F32(PQ_SQRT_INF);
   PQ_EndVector();
   @endcode
 *
 */
#define PQ_Vector8Fixed16(BATCH_OPCODE, DOUBLE_READ_ADDERS, BATCH_MACHINE)                 \
    __asm volatile(                                                                        \
        "1:                                        \n"                                     \
        "    MCR  p0,%[opcode],r4,c1,c0,%[machine] \n"                                     \
        "    MCR  p0,%[opcode],r5,c3,c0,%[machine] \n"                                     \
        "    CMP  r3, #0                           \n"                                     \
        "    ITTE NE                               \n"                                     \
        "    STRHNE r6,[r1],#2                     \n" /* store fourth two results */      \
        "    STRHNE r7,[r1],#2                     \n" /* store fourth two results */      \
        "    MOVEQ r3, #1                          \n" /* middle = 1 */                    \
        "    LDRSH r6,[r0],#2                      \n" /* load next 2 of the 8 */          \
        "    LDRSH r7,[r0],#2                      \n" /* load next 2 of the 8 */          \
        "    MRC  p0,%[dra],r4,c1,c0,#0            \n"                                     \
        "    MRC  p0,%[dra],r5,c3,c0,#0            \n"                                     \
        "    MCR  p0,%[opcode],r6,c1,c0,%[machine] \n"                                     \
        "    MCR  p0,%[opcode],r7,c3,c0,%[machine] \n"                                     \
        "    STRH r4,[r1],#2                       \n" /* store first two results */       \
        "    STRH r5,[r1],#2                       \n" /* store first two results */       \
        "    LDRSH r8,[r0],#2                      \n" /* load next 2 of the 8 */          \
        "    LDRSH r9,[r0],#2                      \n" /* load next 2 of the 8 */          \
        "    MRC  p0,%[dra],r6,c1,c0,#0            \n"                                     \
        "    MRC  p0,%[dra],r7,c3,c0,#0            \n"                                     \
        "    MCR  p0,%[opcode],r8,c1,c0,%[machine] \n"                                     \
        "    MCR  p0,%[opcode],r9,c3,c0,%[machine] \n"                                     \
        "    STRH r6,[r1],#2                       \n"  /* store second two results */     \
        "    STRH r7,[r1],#2                       \n"  /* store second two results */     \
        "    LDRSH r6,[r0],#2                       \n" /* load last 2 of the 8 */         \
        "    LDRSH r7,[r0],#2                       \n" /* load last 2 of the 8 */         \
        "    MRC  p0,%[dra],r8,c1,c0,#0            \n"                                     \
        "    MRC  p0,%[dra],r9,c3,c0,#0            \n"                                     \
        "    MCR  p0,%[opcode],r6,c1,c0,%[machine] \n"                                     \
        "    MCR  p0,%[opcode],r7,c3,c0,%[machine] \n"                                     \
        "    STRH r8,[r1],#2                       \n" /* store third two results */       \
        "    STRH r9,[r1],#2                       \n" /* store third two results */       \
        "    SUBS r2, r2, #8                       \n" /* length -= 8; if (length != 0) */ \
        "    ITT  NE                               \n"                                     \
        "    LDRSHNE r4,[r0],#2                    \n" /* load first two of next 8 */      \
        "    LDRSHNE r5,[r0],#2                    \n" /* load first two of next 8 */      \
        "    MRC  p0,%[dra],r6,c1,c0,#0            \n"                                     \
        "    MRC  p0,%[dra],r7,c3,c0,#0            \n"                                     \
        "    CMP  r2, #0                           \n" /* if (length == 0) */              \
        "    BNE  1b                               \n"                                     \
        "    STRH r6,[r1],#2                       \n" /* store fourth two results */      \
        "    STRH r7,[r1],#2                       \n" /* store fourth two results */      \
        ::[opcode] "i"(BATCH_OPCODE),                                                      \
        [dra] "i"(DOUBLE_READ_ADDERS), [machine] "i"(BATCH_MACHINE))

/*!
 * @brief Q15 data vector calculation.
 *
 * Q15 data vector calculation, this function should only be used for sin/cos Q15 calculation,
 * and the coprocessor output prescaler must be set to 31 before this function. This function
 * loads Q15 data and left shift 16 bits, calculate and right shift 16 bits, then stores to
 * the output array. The input range -1 to 1 means -pi to pi.
 * For example, to calculate sin of a vector, use like this:
 * @code
   #define VECTOR_LEN 8
   int16_t input[VECTOR_LEN] = {...}
   int16_t output[VECTOR_LEN];
   const pq_prescale_t prescale =
   {
       .inputPrescale = 0,
       .outputPrescale = 31,
       .outputSaturate = 0
   };

   PQ_SetCoprocessorScaler(POWERQUAD, const pq_prescale_t *prescale);

   PQ_StartVectorQ15(pSrc, pDst, length);
   PQ_Vector8Q15(PQ_SQRT_INF);
   PQ_EndVector();
   @endcode
 *
 */
#define PQ_Vector8Q15(BATCH_OPCODE, DOUBLE_READ_ADDERS, BATCH_MACHINE)                     \
    __asm volatile(                                                                        \
        "1:                                        \n"                                     \
        "    MCR  p0,%[opcode],r4,c1,c0,%[machine] \n"                                     \
        "    ISB                                   \n"                                     \
        "    MCR  p0,%[opcode],r5,c3,c0,%[machine] \n"                                     \
        "    CMP  r3, #0                           \n"                                     \
        "    ITTTE NE                              \n"                                     \
        "    LSRNE r6,r6,#16                       \n" /* store fourth two results */      \
        "    BFINE r7,r6,#0,#16                    \n" /* store fourth two results */      \
        "    STRNE r7,[r1],#4                      \n" /* store fourth two results */      \
        "    MOVEQ r3, #1                          \n" /* middle = 1 */                    \
        "    LDR r7,[r0],#4                        \n" /* load next 2 of the 8 */          \
        "    LSL r6,r7,#16                         \n" /* load next 2 of the 8 */          \
        "    BFC r7,#0,#16                         \n" /* load next 2 of the 8 */          \
        "    MRC  p0,%[dra],r4,c1,c0,#0            \n"                                     \
        "    MRC  p0,%[dra],r5,c3,c0,#0            \n"                                     \
        "    MCR  p0,%[opcode],r6,c1,c0,%[machine] \n"                                     \
        "    ISB                                   \n"                                     \
        "    MCR  p0,%[opcode],r7,c3,c0,%[machine] \n"                                     \
        "    LSR r4,r4,#16                         \n" /* store first two results */       \
        "    BFI r5,r4,#0,#16                      \n" /* store first two results */       \
        "    STR r5,[r1],#4                        \n" /* store first two results */       \
        "    LDR r9,[r0],#4                        \n" /* load next 2 of the 8 */          \
        "    LSL r8,r9,#16                         \n" /* load next 2 of the 8 */          \
        "    BFC r9,#0,#16                         \n" /* load next 2 of the 8 */          \
        "    MRC  p0,%[dra],r6,c1,c0,#0            \n"                                     \
        "    MRC  p0,%[dra],r7,c3,c0,#0            \n"                                     \
        "    MCR  p0,%[opcode],r8,c1,c0,%[machine] \n"                                     \
        "    ISB                                   \n"                                     \
        "    MCR  p0,%[opcode],r9,c3,c0,%[machine] \n"                                     \
        "    LSR r6,r6,#16                         \n" /* store second two results */      \
        "    BFI r7,r6,#0,#16                      \n" /* store second two results */      \
        "    STR r7,[r1],#4                        \n" /* store second two results */      \
        "    LDR r7,[r0],#4                        \n" /* load next 2 of the 8 */          \
        "    LSL r6,r7,#16                         \n" /* load next 2 of the 8 */          \
        "    BFC r7,#0,#16                         \n" /* load next 2 of the 8 */          \
        "    MRC  p0,%[dra],r8,c1,c0,#0            \n"                                     \
        "    MRC  p0,%[dra],r9,c3,c0,#0            \n"                                     \
        "    MCR  p0,%[opcode],r6,c1,c0,%[machine] \n"                                     \
        "    ISB                                   \n"                                     \
        "    MCR  p0,%[opcode],r7,c3,c0,%[machine] \n"                                     \
        "    LSR r8,r8,#16                         \n" /* store third two results */       \
        "    BFI r9,r8,#0,#16                      \n" /* store third two results */       \
        "    STR r9,[r1],#4                        \n" /* store third two results */       \
        "    SUBS r2, r2, #8                       \n" /* length -= 8; if (length != 0) */ \
        "    ITTT  NE                              \n"                                     \
        "    LDRNE r5,[r0],#4                      \n" /* load next 2 of the 8 */          \
        "    LSLNE r4,r5,#16                       \n" /* load next 2 of the 8 */          \
        "    BFCNE r5,#0,#16                       \n" /* load next 2 of the 8 */          \
        "    MRC  p0,%[dra],r6,c1,c0,#0            \n"                                     \
        "    MRC  p0,%[dra],r7,c3,c0,#0            \n"                                     \
        "    CMP  r2, #0                           \n" /* if (length == 0) */              \
        "    BNE  1b                               \n"                                     \
        "    LSR r6,r6,#16                         \n" /* store fourth two results */      \
        "    BFI r7,r6,#0,#16                      \n" /* store fourth two results */      \
        "    STR r7,[r1],#4                        \n" /* store fourth two results */      \
        ::[opcode] "i"(BATCH_OPCODE),                                                      \
        [dra] "i"(DOUBLE_READ_ADDERS), [machine] "i"(BATCH_MACHINE))

/*!
 * @brief Float data vector biquad direct form II calculation.
 *
 * Biquad filter, the input and output data are float data. Biquad side 0 is used. Example:
 * @code
   #define VECTOR_LEN 16
   float input[VECTOR_LEN] = {1024.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
   float output[VECTOR_LEN];
   pq_biquad_state_t state =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   PQ_BiquadRestoreInternalState(POWERQUAD, 0, &state);

   PQ_Initiate_Vector_Func(pSrc,pDst);
   PQ_DF2_Vector8_FP(false,false);
   PQ_DF2_Vector8_FP(true,true);
   PQ_End_Vector_Func();
   @endcode
 *
 */
#define PQ_DF2_Vector8_FP(middle, last)                                          \
    __asm volatile("MCR      p0,#0x1,r2,c0,c0,#6"); /* write biquad0*/           \
    if (middle)                                                                  \
    {                                                                            \
        __asm volatile("STR      r5,[r1],#4"); /* store last result*/            \
    }                                                                            \
    __asm volatile("LDRD     r4,r5,[r0],#8");       /* load next 2 datas */      \
    __asm volatile("MRC      p0,#0x1,r2,c0,c0,#0"); /* read  biquad0*/           \
    __asm volatile("MCR      p0,#0x1,r3,c0,c0,#6"); /* write biquad0 */          \
    __asm volatile("MRC      p0,#0x1,r3,c0,c0,#0"); /* read  biquad0*/           \
    __asm volatile("MCR      p0,#0x1,r4,c0,c0,#6"); /* write biquad0 */          \
    __asm volatile("STRD     r2,r3,[r1],#8");       /* store first 2 results */  \
    __asm volatile("MRC      p0,#0x1,r4,c0,c0,#0");                              \
    __asm volatile("MCR      p0,#0x1,r5,c0,c0,#6");                              \
    __asm volatile("LDRD     r6,r7,[r0],#8"); /* load next 2 datas */            \
    __asm volatile("MRC      p0,#0x1,r5,c0,c0,#0");                              \
    __asm volatile("MCR      p0,#0x1,r6,c0,c0,#6");                              \
    __asm volatile("STRD     r4,r5,[r1],#8"); /* store next 2 results */         \
    __asm volatile("MRC      p0,#0x1,r6,c0,c0,#0");                              \
    __asm volatile("MCR      p0,#0x1,r7,c0,c0,#6");                              \
    __asm volatile("LDRD     r4,r5,[r0],#8"); /* load next 2 datas */            \
    __asm volatile("MRC      p0,#0x1,r7,c0,c0,#0");                              \
    __asm volatile("MCR      p0,#0x1,r4,c0,c0,#6");                              \
    __asm volatile("STRD     r6,r7,[r1],#8"); /* store next 2 results */         \
    __asm volatile("MRC      p0,#0x1,r4,c0,c0,#0");                              \
    __asm volatile("MCR      p0,#0x1,r5,c0,c0,#6");                              \
    if (!last)                                                                   \
    {                                                                            \
        __asm volatile("LDRD     r2,r3,[r0],#8"); /* load first two of next 8 */ \
    }                                                                            \
    __asm volatile("STR      r4,[r1],#4");                                       \
    __asm volatile("MRC      p0,#0x1,r5,c0,c0,#0");                              \
    if (last)                                                                    \
    {                                                                            \
        __asm volatile("STR      r5,[r1],#4"); /* store last result */           \
    }

/*!
 * @brief Fixed data vector biquad direct form II calculation.
 *
 * Biquad filter, the input and output data are fixed data. Biquad side 0 is used. Example:
 * @code
   #define VECTOR_LEN 16
   int32_t input[VECTOR_LEN] = {1024, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
   int32_t output[VECTOR_LEN];
   pq_biquad_state_t state =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   PQ_BiquadRestoreInternalState(POWERQUAD, 0, &state);

   PQ_Initiate_Vector_Func(pSrc,pDst);
   PQ_DF2_Vector8_FX(false,false);
   PQ_DF2_Vector8_FX(true,true);
   PQ_End_Vector_Func();
   @endcode
 *
 */
#define PQ_DF2_Vector8_FX(middle, last)                                     \
    __asm volatile("MCR      p0,#0x1,r2,c1,c0,#6"); /* write biquad0*/      \
    if (middle)                                                             \
    {                                                                       \
        __asm volatile("STR      r5,[r1],#4"); /* store last result*/       \
    }                                                                       \
    __asm volatile("LDRD     r4,r5,[r0],#8");       /* load next 2 datas */ \
    __asm volatile("MRC      p0,#0x1,r2,c1,c0,#0"); /* read  biquad0*/      \
    __asm volatile("MCR      p0,#0x1,r3,c1,c0,#6"); /* write biquad0 */     \
    __asm volatile("MRC      p0,#0x1,r3,c1,c0,#0");                         \
    __asm volatile("MCR      p0,#0x1,r4,c1,c0,#6");                         \
    __asm volatile("STRD     r2,r3,[r1],#8"); /* store first 2 results */   \
    __asm volatile("MRC      p0,#0x1,r4,c1,c0,#0");                         \
    __asm volatile("MCR      p0,#0x1,r5,c1,c0,#6");                         \
    __asm volatile("LDRD     r6,r7,[r0],#8");                               \
    __asm volatile("MRC      p0,#0x1,r5,c1,c0,#0");                         \
    __asm volatile("MCR      p0,#0x1,r6,c1,c0,#6");                         \
    __asm volatile("STRD     r4,r5,[r1],#8"); /* store next 2 results */    \
    __asm volatile("MRC      p0,#0x1,r6,c1,c0,#0");                         \
    __asm volatile("MCR      p0,#0x1,r7,c1,c0,#6");                         \
    __asm volatile("LDRD     r4,r5,[r0],#8");                               \
    __asm volatile("MRC      p0,#0x1,r7,c1,c0,#0");                         \
    __asm volatile("MCR      p0,#0x1,r4,c1,c0,#6");                         \
    __asm volatile("STRD     r6,r7,[r1],#8"); /* store next 2 results */    \
    __asm volatile("MRC      p0,#0x1,r4,c1,c0,#0");                         \
    __asm volatile("MCR      p0,#0x1,r5,c1,c0,#6");                         \
    if (!last)                                                              \
    {                                                                       \
        __asm volatile("LDRD     r2,r3,[r0],#8"); /* load two of next 8 */  \
    }                                                                       \
    __asm volatile("STR      r4,[r1],#4"); /* store 7th results */          \
    __asm volatile("MRC      p0,#0x1,r5,c1,c0,#0");                         \
    if (last)                                                               \
    {                                                                       \
        __asm volatile("STR      r5,[r1],#4"); /* store last result */      \
    }

/*!
 * @brief Float data vector biquad direct form II calculation.
 *
 * Biquad filter, the input and output data are float data. Biquad side 0 is used. Example:
 * @code
   #define VECTOR_LEN 8
   float input[VECTOR_LEN] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
   float output[VECTOR_LEN];
   pq_biquad_state_t state =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   PQ_BiquadRestoreInternalState(POWERQUAD, 0, &state);

   PQ_StartVector(input, output, VECTOR_LEN);
   PQ_Vector8BiquadDf2F32();
   PQ_EndVector();
   @endcode
 *
 */
#define PQ_Vector8BiquadDf2F32()                                                         \
    __asm volatile(                                                                      \
        "1:                                      \n"                                     \
        "    MCR  p0,#0x1,r4,c0,c0,#6            \n" /* write biquad0*/                  \
        "    CMP  r3, #0                         \n"                                     \
        "    ITE  NE                             \n"                                     \
        "    STRNE r7,[r1],#4                    \n" /* store last result*/              \
        "    MOVEQ r3, #1                        \n" /* middle = 1 */                    \
        "    LDMIA  r0!,{r6-r9}                  \n" /* load next 4 datas */             \
        "    MRC  p0,#0x1,r4,c0,c0,#0            \n" /* read  biquad0*/                  \
        "    MCR  p0,#0x1,r5,c0,c0,#6            \n" /* write biquad0 */                 \
        "    MRC  p0,#0x1,r5,c0,c0,#0            \n" /* read  biquad0*/                  \
        "    MCR  p0,#0x1,r6,c0,c0,#6            \n" /* write biquad0 */                 \
        "    MRC  p0,#0x1,r6,c0,c0,#0            \n" /* read  biquad0 */                 \
        "    MCR  p0,#0x1,r7,c0,c0,#6            \n" /* write biquad0 */                 \
        "    MRC  p0,#0x1,r7,c0,c0,#0            \n" /* read  biquad0 */                 \
        "    MCR  p0,#0x1,r8,c0,c0,#6            \n" /* write biquad0*/                  \
        "    STMIA    r1!,{r4-r7}                \n" /* store first four results */      \
        "    MRC  p0,#0x1,r8,c0,c0,#0            \n" /* read  biquad0*/                  \
        "    MCR  p0,#0x1,r9,c0,c0,#6            \n" /* write biquad0*/                  \
        "    LDRD r6,r7,[r0],#8                  \n" /* load next 2 items*/              \
        "    MRC  p0,#0x1,r9,c0,c0,#0            \n" /* read  biquad0*/                  \
        "    MCR  p0,#0x1,r6,c0,c0,#6            \n" /* write biquad0*/                  \
        "    STRD r8,r9,[r1],#8                  \n" /* store third two results */       \
        "    MRC  p0,#0x1,r6,c0,c0,#0            \n" /* read  biquad0*/                  \
        "    MCR  p0,#0x1,r7,c0,c0,#6            \n" /* write biquad0*/                  \
        "    SUBS r2, r2, #8                     \n" /* length -= 8; if (length != 0) */ \
        "    IT   NE                             \n"                                     \
        "    LDRDNE r4,r5,[r0],#8                \n" /* load first two of next 8 */      \
        "    STR r6,[r1],#4                      \n" /* store 7th results */             \
        "    MRC  p0,#0x1,r7,c0,c0,#0            \n" /* read  biquad0*/                  \
        "    CMP  r2, #0                         \n" /* if (length == 0) */              \
        "    BNE  1b                             \n"                                     \
        "    STR r7,[r1],#4                      \n" /* store last result */             \
        )

/*!
 * @brief Fixed 32-bit data vector biquad direct form II calculation.
 *
 * Biquad filter, the input and output data are Q31 or 32-bit integer. Biquad side 0 is used. Example:
 * @code
   #define VECTOR_LEN 8
   int32_t input[VECTOR_LEN] = {1, 2, 3, 4, 5, 6, 7, 8};
   int32_t output[VECTOR_LEN];
   pq_biquad_state_t state =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   PQ_BiquadRestoreInternalState(POWERQUAD, 0, &state);

   PQ_StartVector(input, output, VECTOR_LEN);
   PQ_Vector8BiquadDf2Fixed32();
   PQ_EndVector();
   @endcode
 *
 */
#define PQ_Vector8BiquadDf2Fixed32()                                                     \
    __asm volatile(                                                                      \
        "1:                                      \n"                                     \
        "    MCR  p0,#0x1,r4,c1,c0,#6            \n" /* write biquad0*/                  \
        "    CMP  r3, #0                         \n"                                     \
        "    ITE  NE                             \n"                                     \
        "    STRNE r7,[r1],#4                    \n" /* store last result*/              \
        "    MOVEQ r3, #1                        \n" /* middle = 1 */                    \
        "    LDMIA  r0!,{r6-r9}                  \n" /* load next 4 datas */             \
        "    MRC  p0,#0x1,r4,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MCR  p0,#0x1,r5,c1,c0,#6            \n" /* write biquad0 */                 \
        "    MRC  p0,#0x1,r5,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MCR  p0,#0x1,r6,c1,c0,#6            \n" /* write biquad0 */                 \
        "    MRC  p0,#0x1,r6,c1,c0,#0            \n" /* read  biquad0 */                 \
        "    MCR  p0,#0x1,r7,c1,c0,#6            \n" /* write biquad0 */                 \
        "    MRC  p0,#0x1,r7,c1,c0,#0            \n" /* read  biquad0 */                 \
        "    MCR  p0,#0x1,r8,c1,c0,#6            \n" /* write biquad0*/                  \
        "    STMIA    r1!,{r4-r7}                \n" /* store first four results */      \
        "    MRC  p0,#0x1,r8,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MCR  p0,#0x1,r9,c1,c0,#6            \n" /* write biquad0*/                  \
        "    LDRD r6,r7,[r0],#8                  \n" /* load next 2 items*/              \
        "    MRC  p0,#0x1,r9,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MCR  p0,#0x1,r6,c1,c0,#6            \n" /* write biquad0*/                  \
        "    STRD r8,r9,[r1],#8                  \n" /* store third two results */       \
        "    MRC  p0,#0x1,r6,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MCR  p0,#0x1,r7,c1,c0,#6            \n" /* write biquad0*/                  \
        "    SUBS r2, r2, #8                     \n" /* length -= 8; if (length != 0) */ \
        "    IT   NE                             \n"                                     \
        "    LDRDNE r4,r5,[r0],#8                \n" /* load first two of next 8 */      \
        "    STR r6,[r1],#4                      \n" /* store 7th results */             \
        "    MRC  p0,#0x1,r7,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    CMP  r2, #0                         \n" /* if (length == 0) */              \
        "    BNE  1b                             \n"                                     \
        "    STR r7,[r1],#4                      \n" /* store last result */             \
        )

/*!
 * @brief Fixed 16-bit data vector biquad direct form II calculation.
 *
 * Biquad filter, the input and output data are Q15 or 16-bit integer. Biquad side 0 is used. Example:
 * @code
   #define VECTOR_LEN 8
   int16_t input[VECTOR_LEN] = {1, 2, 3, 4, 5, 6, 7, 8};
   int16_t output[VECTOR_LEN];
   pq_biquad_state_t state =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   PQ_BiquadRestoreInternalState(POWERQUAD, 0, &state);

   PQ_StartVector(input, output, VECTOR_LEN);
   PQ_Vector8BiquadDf2Fixed16();
   PQ_EndVector();
   @endcode
 *
 */
#define PQ_Vector8BiquadDf2Fixed16()                                                     \
    __asm volatile(                                                                      \
        "1:                                      \n"                                     \
        "    MCR  p0,#0x1,r4,c1,c0,#6            \n" /* write biquad0*/                  \
        "    CMP  r3, #0                         \n"                                     \
        "    ITE  NE                             \n"                                     \
        "    STRHNE r7,[r1],#2                   \n" /* store last result*/              \
        "    MOVEQ r3, #1                        \n" /* middle = 1 */                    \
        "    LDRSH r6,[r0],#2                    \n" /* load next 2 of the 8*/           \
        "    LDRSH r7,[r0],#2                    \n" /* load next 2 of the 8*/           \
        "    MRC  p0,#0x1,r4,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MCR  p0,#0x1,r5,c1,c0,#6            \n" /* write biquad0 */                 \
        "    MRC  p0,#0x1,r5,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    LDRSH r8,[r0],#2                    \n" /* load next 2 of the 8*/           \
        "    LDRSH r9,[r0],#2                    \n" /* load next 2 of the 8*/           \
        "    MCR  p0,#0x1,r6,c1,c0,#6            \n" /* write biquad0 */                 \
        "    MRC  p0,#0x1,r6,c1,c0,#0            \n" /* read  biquad0 */                 \
        "    MCR  p0,#0x1,r7,c1,c0,#6            \n" /* write biquad0 */                 \
        "    MRC  p0,#0x1,r7,c1,c0,#0            \n" /* read  biquad0 */                 \
        "    STRH r4,[r1],#2                     \n" /* store first 4 results */         \
        "    STRH r5,[r1],#2                     \n" /* store first 4 results */         \
        "    MCR  p0,#0x1,r8,c1,c0,#6            \n" /* write biquad0*/                  \
        "    STRH r6,[r1],#2                     \n" /* store first 4 results */         \
        "    STRH r7,[r1],#2                     \n" /* store first 4 results */         \
        "    MRC  p0,#0x1,r8,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MCR  p0,#0x1,r9,c1,c0,#6            \n" /* write biquad0*/                  \
        "    LDRSH r6,[r0],#2                    \n" /* load next 1 of the 8*/           \
        "    LDRSH r7,[r0],#2                    \n" /* load next 1 of the 8*/           \
        "    MRC  p0,#0x1,r9,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MCR  p0,#0x1,r6,c1,c0,#6            \n" /* write biquad0*/                  \
        "    STRH r8,[r1],#2                     \n" /* store next two results */        \
        "    STRH r9,[r1],#2                     \n" /* store next two results */        \
        "    MRC  p0,#0x1,r6,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MCR  p0,#0x1,r7,c1,c0,#6            \n" /* write biquad0*/                  \
        "    SUBS r2, r2, #8                     \n" /* length -= 8; if (length != 0) */ \
        "    ITT   NE                            \n"                                     \
        "    LDRSHNE r4,[r0],#2                  \n" /* load first two of next 8*/       \
        "    LDRSHNE r5,[r0],#2                  \n" /* load first two of next 8*/       \
        "    STRH r6,[r1],#2                     \n" /* store 7th results */             \
        "    MRC  p0,#0x1,r7,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    CMP  r2, #0                         \n" /* if (length == 0) */              \
        "    BNE  1b                             \n"                                     \
        "    STRH r7,[r1],#2                     \n" /* store last result */             \
        )

/*!
 * @brief Float data vector direct form II biquad cascade filter.
 *
 * The input and output data are float data. The data flow is
 * input  -> biquad side 1 -> biquad side 0 -> output.
 *
 * @code
   #define VECTOR_LEN 16
   float input[VECTOR_LEN] = {1024.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
   float output[VECTOR_LEN];
   pq_biquad_state_t state0 =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   pq_biquad_state_t state1 =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   PQ_BiquadRestoreInternalState(POWERQUAD, 0, &state0);
   PQ_BiquadRestoreInternalState(POWERQUAD, 1, &state1);

   PQ_Initiate_Vector_Func(pSrc, pDst);
   PQ_DF2_Cascade_Vector8_FP(false, false);
   PQ_DF2_Cascade_Vector8_FP(true, true);
   PQ_End_Vector_Func();
   @endcode
 *
 */
#define PQ_DF2_Cascade_Vector8_FP(middle, last)                                    \
    __asm volatile("MCR  p0,#0x1,r2,c2,c0,#6"); /* write biquad1*/                 \
    if (middle)                                                                    \
    {                                                                              \
        __asm volatile("MCR  p0,#0x1,r5,c0,c0,#6"); /* write biquad0*/             \
        __asm volatile("MRRC p0,#0,r5,r2,c1");      /* read both biquad*/          \
    }                                                                              \
    else                                                                           \
    {                                                                              \
        __asm volatile("MRC  p0,#0x1,r2,c2,c0,#0"); /* read  biquad1*/             \
    }                                                                              \
    __asm volatile("MCR  p0,#0x1,r3,c2,c0,#6"); /* write biquad1*/                 \
    __asm volatile("MCR  p0,#0x1,r2,c0,c0,#6"); /* write biquad0*/                 \
    if (middle)                                                                    \
    {                                                                              \
        __asm volatile("STRD r4,r5,[r1],#8"); /* store last two results*/          \
    }                                                                              \
    __asm volatile("LDRD r4,r5,[r0],#8");       /* load next 2 datas */            \
    __asm volatile("MRRC p0,#0,r2,r3,c1");      /* read both biquad*/              \
    __asm volatile("MCR  p0,#0x1,r4,c2,c0,#6"); /* write biquad1*/                 \
    __asm volatile("MCR  p0,#0x1,r3,c0,c0,#6"); /* write biquad0*/                 \
    __asm volatile("LDRD r6,r7,[r0],#8");                                          \
    __asm volatile("MRRC p0,#0,r3,r4,c1");                                         \
    __asm volatile("MCR  p0,#0x1,r5,c2,c0,#6");                                    \
    __asm volatile("MCR  p0,#0x1,r4,c0,c0,#6");                                    \
    __asm volatile("STRD r2,r3,[r1],#8"); /* store first two results */            \
    __asm volatile("MRRC p0,#0,r4,r5,c1");                                         \
    __asm volatile("MCR  p0,#0x1,r6,c2,c0,#6");                                    \
    __asm volatile("MCR  p0,#0x1,r5,c0,c0,#6");                                    \
    __asm volatile("STR  r4,[r1],#4");                                             \
    __asm volatile("MRRC p0,#0,r5,r6,c1");                                         \
    __asm volatile("MCR  p0,#0x1,r7,c2,c0,#6");                                    \
    __asm volatile("MCR  p0,#0x1,r6,c0,c0,#6");                                    \
    __asm volatile("STR  r5,[r1],#4");                                             \
    __asm volatile("LDRD r4,r5,[r0],#8");                                          \
    __asm volatile("MRRC p0,#0,r6,r7,c1");                                         \
    __asm volatile("MCR  p0,#0x1,r4,c2,c0,#6");                                    \
    __asm volatile("MCR  p0,#0x1,r7,c0,c0,#6");                                    \
    if (!last)                                                                     \
    {                                                                              \
        __asm volatile("LDRD r2,r3,[r0],#8"); /* load first two of next 8 */       \
    }                                                                              \
    __asm volatile("MRRC p0,#0,r7,r4,c1");                                         \
    __asm volatile("MCR  p0,#0x1,r5,c2,c0,#6");                                    \
    __asm volatile("MCR  p0,#0x1,r4,c0,c0,#6");                                    \
    __asm volatile("STRD r6,r7,[r1],#8"); /* store third two results */            \
    __asm volatile("MRRC p0,#0,r4,r5,c1");                                         \
    if (last)                                                                      \
    {                                                                              \
        __asm volatile("MCR  p0,#0x1,r5,c0,c0,#6"); /* write biquad0*/             \
        __asm volatile("MRC  p0,#0x1,r5,c0,c0,#0"); /* read  biquad0*/             \
        __asm volatile("STRD r4,r5,[r1],#8");       /* store fourth two results */ \
    }

/*!
 * @brief Fixed data vector direct form II biquad cascade filter.
 *
 * The input and output data are fixed data. The data flow is
 * input  -> biquad side 1 -> biquad side 0 -> output.
 *
 * @code
   #define VECTOR_LEN 16
   int32_t input[VECTOR_LEN] = {1024.0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
   int32_t output[VECTOR_LEN];
   pq_biquad_state_t state0 =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   pq_biquad_state_t state1 =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   PQ_BiquadRestoreInternalState(POWERQUAD, 0, &state0);
   PQ_BiquadRestoreInternalState(POWERQUAD, 1, &state1);

   PQ_Initiate_Vector_Func(pSrc, pDst);
   PQ_DF2_Cascade_Vector8_FX(false, false);
   PQ_DF2_Cascade_Vector8_FX(true, true);
   PQ_End_Vector_Func();
   @endcode
 *
 */
#define PQ_DF2_Cascade_Vector8_FX(middle, last)                                    \
    __asm volatile("MCR  p0,#0x1,r2,c3,c0,#6"); /* write biquad1*/                 \
    if (middle)                                                                    \
    {                                                                              \
        __asm volatile("MCR  p0,#0x1,r5,c1,c0,#6"); /* write biquad0*/             \
        __asm volatile("MRC  p0,#0x1,r5,c1,c0,#0"); /* read  biquad0*/             \
        __asm volatile("MRC  p0,#0x1,r2,c3,c0,#0"); /* read  biquad1*/             \
    }                                                                              \
    else                                                                           \
    {                                                                              \
        __asm volatile("MRC  p0,#0x1,r2,c3,c0,#0"); /* read  biquad1*/             \
    }                                                                              \
    __asm volatile("MCR  p0,#0x1,r3,c3,c0,#6"); /* write biquad1*/                 \
    __asm volatile("MCR  p0,#0x1,r2,c1,c0,#6"); /* write biquad0*/                 \
    if (middle)                                                                    \
    {                                                                              \
        __asm volatile("STRD r4,r5,[r1],#8"); /* store last two results*/          \
    }                                                                              \
    __asm volatile("LDRD r4,r5,[r0],#8");       /* load next 2 datas */            \
    __asm volatile("MRC  p0,#0x1,r2,c1,c0,#0"); /* read  biquad0*/                 \
    __asm volatile("MRC  p0,#0x1,r3,c3,c0,#0"); /* read  biquad1*/                 \
    __asm volatile("MCR  p0,#0x1,r4,c3,c0,#6"); /* write biquad1*/                 \
    __asm volatile("MCR  p0,#0x1,r3,c1,c0,#6"); /* write biquad0*/                 \
    __asm volatile("LDRD r6,r7,[r0],#8");                                          \
    __asm volatile("MRC  p0,#0x1,r3,c1,c0,#0");                                    \
    __asm volatile("MRC  p0,#0x1,r4,c3,c0,#0");                                    \
    __asm volatile("MCR  p0,#0x1,r5,c3,c0,#6");                                    \
    __asm volatile("MCR  p0,#0x1,r4,c1,c0,#6");                                    \
    __asm volatile("STRD r2,r3,[r1],#8");                                          \
    __asm volatile("MRC  p0,#0x1,r4,c1,c0,#0");                                    \
    __asm volatile("MRC  p0,#0x1,r5,c3,c0,#0");                                    \
    __asm volatile("MCR  p0,#0x1,r6,c3,c0,#6");                                    \
    __asm volatile("MCR  p0,#0x1,r5,c1,c0,#6");                                    \
    __asm volatile("STR  r4,[r1],#4");                                             \
    __asm volatile("MRC  p0,#0x1,r5,c1,c0,#0");                                    \
    __asm volatile("MRC  p0,#0x1,r6,c3,c0,#0");                                    \
    __asm volatile("MCR  p0,#0x1,r7,c3,c0,#6");                                    \
    __asm volatile("MCR  p0,#0x1,r6,c1,c0,#6");                                    \
    __asm volatile("STR  r5,[r1],#4");                                             \
    __asm volatile("LDRD r4,r5,[r0],#8");                                          \
    __asm volatile("MRC  p0,#0x1,r6,c1,c0,#0");                                    \
    __asm volatile("MRC  p0,#0x1,r7,c3,c0,#0");                                    \
    __asm volatile("MCR  p0,#0x1,r4,c3,c0,#6");                                    \
    __asm volatile("MCR  p0,#0x1,r7,c1,c0,#6");                                    \
    if (!last)                                                                     \
    {                                                                              \
        __asm volatile("LDRD r2,r3,[r0],#8"); /* load first two of next 8 */       \
    }                                                                              \
    __asm volatile("MRC  p0,#0x1,r7,c1,c0,#0");                                    \
    __asm volatile("MRC  p0,#0x1,r4,c3,c0,#0");                                    \
    __asm volatile("MCR  p0,#0x1,r5,c3,c0,#6");                                    \
    __asm volatile("MCR  p0,#0x1,r4,c1,c0,#6");                                    \
    __asm volatile("STRD r6,r7,[r1],#8");       /* store third two results */      \
    __asm volatile("MRC  p0,#0x1,r4,c1,c0,#0"); /* read  biquad0*/                 \
    __asm volatile("MRC  p0,#0x1,r5,c3,c0,#0"); /* read  biquad1*/                 \
    if (last)                                                                      \
    {                                                                              \
        __asm volatile("MCR  p0,#0x1,r5,c1,c0,#6"); /* write biquad0*/             \
        __asm volatile("MRC  p0,#0x1,r5,c1,c0,#0"); /* read  biquad0*/             \
        __asm volatile("STRD r4,r5,[r1],#8");       /* store fourth two results */ \
    }

/*!
 * @brief Float data vector direct form II biquad cascade filter.
 *
 * The input and output data are float data. The data flow is
 * input  -> biquad side 1 -> biquad side 0 -> output.
 *
 * @code
   #define VECTOR_LEN 8
   float input[VECTOR_LEN] = {1, 2, 3, 4, 5, 6, 7, 8};
   float output[VECTOR_LEN];
   pq_biquad_state_t state0 =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   pq_biquad_state_t state1 =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   PQ_BiquadRestoreInternalState(POWERQUAD, 0, &state0);
   PQ_BiquadRestoreInternalState(POWERQUAD, 1, &state1);

   PQ_StartVector(input, output, VECTOR_LEN);
   PQ_Vector8BiqaudDf2CascadeF32();
   PQ_EndVector();
   @endcode
 *
 */
#define PQ_Vector8BiqaudDf2CascadeF32()                                                       \
    __asm volatile(                                                                           \
        "1:                                      \n"                                          \
        "    MCR  p0,#0x1,r4,c2,c0,#2            \n" /* write biquad1*/                       \
        "    CMP  r3, #0                         \n"                                          \
        "    ITTE  NE                            \n"                                          \
        "    MCRNE  p0,#0x1,r7,c0,c0,#2          \n" /* write biquad0*/                       \
        "    MRRCNE p0,#0,r7,r4,c1               \n" /* read both biquad*/                    \
        "    MRCEQ  p0,#0x1,r4,c2,c0,#0          \n" /* read  biquad1*/                       \
        "    MCR  p0,#0x1,r5,c2,c0,#2            \n" /* write biquad1*/                       \
        "    MCR  p0,#0x1,r4,c0,c0,#2            \n" /* write biquad0*/                       \
        "    CMP  r3, #0                         \n"                                          \
        "    ITE  NE                             \n"                                          \
        "    STRDNE r6,r7,[r1],#8                \n" /* store last two results*/              \
        "    MOVEQ r3, #1                        \n" /* middle = 1 */                         \
        "    LDMIA r0!,{r6-r9}                   \n" /* load next 4 datas */                  \
        "    MRRC p0,#0,r4,r5,c1                 \n" /* read both biquad*/                    \
        "    MCR  p0,#0x1,r6,c2,c0,#2            \n" /* write biquad1*/                       \
        "    MCR  p0,#0x1,r5,c0,c0,#2            \n" /* write biquad0*/                       \
        "    MRRC p0,#0,r5,r6,c1                 \n" /* read both biquad*/                    \
        "    MCR  p0,#0x1,r7,c2,c0,#2            \n" /* write biquad1*/                       \
        "    MCR  p0,#0x1,r6,c0,c0,#2            \n" /* write biquad0*/                       \
        "    MRRC p0,#0,r6,r7,c1                 \n" /* read both biquad*/                    \
        "    MCR  p0,#0x1,r8,c2,c0,#2            \n" /* write biquad1*/                       \
        "    MCR  p0,#0x1,r7,c0,c0,#2            \n" /* write biquad0*/                       \
        "    MRRC p0,#0,r7,r8,c1                 \n" /* read both biquad*/                    \
        "    MCR  p0,#0x1,r9,c2,c0,#2            \n" /* write biquad1*/                       \
        "    MCR  p0,#0x1,r8,c0,c0,#2            \n" /* write biquad0*/                       \
        "    STMIA r1!,{R4-R7}                   \n" /* store first and second two results */ \
        "    LDRD r6,r7,[r0],#8                  \n" /* load last 2 of the 8 */               \
        "    MRRC p0,#0,r8,r9,c1                 \n" /* read both biquad*/                    \
        "    MCR  p0,#0x1,r6,c2,c0,#2            \n" /* write biquad1*/                       \
        "    MCR  p0,#0x1,r9,c0,c0,#2            \n" /* write biquad0*/                       \
        "    SUBS r2, r2, #8                     \n" /* length -= 8; if (length != 0) */      \
        "    IT   NE                             \n"                                          \
        "    LDRDNE r4,r5,[r0],#8                \n" /* load first two of next 8 */           \
        "    MRRC p0,#0,r9,r6,c1                 \n" /* read both biquad*/                    \
        "    MCR  p0,#0x1,r7,c2,c0,#2            \n" /* write biquad1*/                       \
        "    MCR  p0,#0x1,r6,c0,c0,#2            \n" /* write biquad0*/                       \
        "    STRD r8,r9,[r1],#8                  \n" /* store third two results */            \
        "    MRRC p0,#0,r6,r7,c1                 \n" /* read both biquad*/                    \
        "    CMP  r2, #0                         \n" /* if (length == 0) */                   \
        "    BNE  1b                             \n"                                          \
        "    MCR  p0,#0x1,r7,c0,c0,#2            \n" /* write biquad0*/                       \
        "    MRC  p0,#0x1,r7,c0,c0,#0            \n" /* read  biquad0*/                       \
        "    STRD r6,r7,[r1],#8                  \n" /* store fourth two results */           \
        )

/*!
 * @brief Fixed 32-bit data vector direct form II biquad cascade filter.
 *
 * The input and output data are fixed 32-bit data. The data flow is
 * input  -> biquad side 1 -> biquad side 0 -> output.
 *
 * @code
   #define VECTOR_LEN 8
   int32_t input[VECTOR_LEN] = {1, 2, 3, 4, 5, 6, 7, 8};
   int32_t output[VECTOR_LEN];
   pq_biquad_state_t state0 =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   pq_biquad_state_t state1 =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   PQ_BiquadRestoreInternalState(POWERQUAD, 0, &state0);
   PQ_BiquadRestoreInternalState(POWERQUAD, 1, &state1);

   PQ_StartVector(input, output, VECTOR_LEN);
   PQ_Vector8BiqaudDf2CascadeFixed32();
   PQ_EndVector();
   @endcode
 *
 */
#define PQ_Vector8BiqaudDf2CascadeFixed32()                                                   \
    __asm volatile(                                                                           \
        "1:                                      \n"                                          \
        "    MCR  p0,#0x1,r4,c3,c0,#6            \n" /* write biquad1*/                       \
        "    CMP  r3, #0                         \n"                                          \
        "    ITTTE  NE                           \n"                                          \
        "    MCRNE  p0,#0x1,r7,c1,c0,#6          \n" /* write biquad0*/                       \
        "    MRCNE  p0,#0x1,r7,c1,c0,#0          \n" /* read  biquad0*/                       \
        "    MRCNE  p0,#0x1,r4,c3,c0,#0          \n" /* read  biquad1*/                       \
        "    MRCEQ  p0,#0x1,r4,c3,c0,#0          \n" /* read  biquad1*/                       \
        "    MCR  p0,#0x1,r5,c3,c0,#6            \n" /* write biquad1*/                       \
        "    MCR  p0,#0x1,r4,c1,c0,#6            \n" /* write biquad0*/                       \
        "    CMP  r3, #0                         \n"                                          \
        "    ITE  NE                             \n"                                          \
        "    STRDNE r6,r7,[r1],#8                \n" /* store last two results*/              \
        "    MOVEQ r3, #1                        \n" /* middle = 1 */                         \
        "    LDMIA r0!,{r6-r9}                   \n" /* load next 4 datas */                  \
        "    MRC  p0,#0x1,r4,c1,c0,#0            \n" /* read  biquad0*/                       \
        "    MRC  p0,#0x1,r5,c3,c0,#0            \n" /* read  biquad1*/                       \
        "    MCR  p0,#0x1,r6,c3,c0,#6            \n" /* write biquad1*/                       \
        "    MCR  p0,#0x1,r5,c1,c0,#6            \n" /* write biquad0*/                       \
        "    MRC  p0,#0x1,r5,c1,c0,#0            \n" /* read  biquad0*/                       \
        "    MRC  p0,#0x1,r6,c3,c0,#0            \n" /* read  biquad1*/                       \
        "    MCR  p0,#0x1,r7,c3,c0,#6            \n" /* write biquad1*/                       \
        "    MCR  p0,#0x1,r6,c1,c0,#6            \n" /* write biquad0*/                       \
        "    MRC  p0,#0x1,r6,c1,c0,#0            \n" /* read  biquad0*/                       \
        "    MRC  p0,#0x1,r7,c3,c0,#0            \n" /* read  biquad1*/                       \
        "    MCR  p0,#0x1,r8,c3,c0,#6            \n" /* write biquad1*/                       \
        "    MCR  p0,#0x1,r7,c1,c0,#6            \n" /* write biquad0*/                       \
        "    MRC  p0,#0x1,r7,c1,c0,#0            \n" /* read  biquad0*/                       \
        "    MRC  p0,#0x1,r8,c3,c0,#0            \n" /* read  biquad1*/                       \
        "    MCR  p0,#0x1,r9,c3,c0,#6            \n" /* write biquad1*/                       \
        "    MCR  p0,#0x1,r8,c1,c0,#6            \n" /* write biquad0*/                       \
        "    STMIA r1!,{R4-R7}                   \n" /* store first and second two results */ \
        "    LDRD r6,r7,[r0],#8                  \n" /* load last 2 of the 8 */               \
        "    MRC  p0,#0x1,r8,c1,c0,#0            \n" /* read  biquad0*/                       \
        "    MRC  p0,#0x1,r9,c3,c0,#0            \n" /* read  biquad1*/                       \
        "    MCR  p0,#0x1,r6,c3,c0,#6            \n" /* write biquad1*/                       \
        "    MCR  p0,#0x1,r9,c1,c0,#6            \n" /* write biquad0*/                       \
        "    SUBS r2, r2, #8                     \n" /* length -= 8; if (length != 0) */      \
        "    IT   NE                             \n"                                          \
        "    LDRDNE r4,r5,[r0],#8                \n" /* load first two of next 8 */           \
        "    MRC  p0,#0x1,r9,c1,c0,#0            \n" /* read  biquad0*/                       \
        "    MRC  p0,#0x1,r6,c3,c0,#0            \n" /* read  biquad1*/                       \
        "    MCR  p0,#0x1,r7,c3,c0,#6            \n" /* write biquad1*/                       \
        "    MCR  p0,#0x1,r6,c1,c0,#6            \n" /* write biquad0*/                       \
        "    STRD r8,r9,[r1],#8                  \n" /* store third two results */            \
        "    MRC  p0,#0x1,r6,c1,c0,#0            \n" /* read  biquad0*/                       \
        "    MRC  p0,#0x1,r7,c3,c0,#0            \n" /* read  biquad1*/                       \
        "    CMP  r2, #0                         \n" /* if (length == 0) */                   \
        "    BNE  1b                             \n"                                          \
        "    MCR  p0,#0x1,r7,c1,c0,#6            \n" /* write biquad0*/                       \
        "    MRC  p0,#0x1,r7,c1,c0,#0            \n" /* read  biquad0*/                       \
        "    STRD r6,r7,[r1],#8                  \n" /* store fourth two results */           \
        )

/*!
 * @brief Fixed 16-bit data vector direct form II biquad cascade filter.
 *
 * The input and output data are fixed 16-bit data. The data flow is
 * input  -> biquad side 1 -> biquad side 0 -> output.
 *
 * @code
   #define VECTOR_LEN 8
   int32_t input[VECTOR_LEN] = {1, 2, 3, 4, 5, 6, 7, 8};
   int32_t output[VECTOR_LEN];
   pq_biquad_state_t state0 =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   pq_biquad_state_t state1 =
   {
        .param =
        {
            .a_1 = xxx,
            .a_2 = xxx,
            .b_0 = xxx,
            .b_1 = xxx,
            .b_2 = xxx,
        },
   };

   PQ_BiquadRestoreInternalState(POWERQUAD, 0, &state0);
   PQ_BiquadRestoreInternalState(POWERQUAD, 1, &state1);

   PQ_StartVector(input, output, VECTOR_LEN);
   PQ_Vector8BiqaudDf2CascadeFixed16();
   PQ_EndVector();
   @endcode
 *
 */
#define PQ_Vector8BiqaudDf2CascadeFixed16()                                              \
    __asm volatile(                                                                      \
        "1:                                      \n"                                     \
        "    MCR  p0,#0x1,r4,c3,c0,#6            \n" /* write biquad1*/                  \
        "    CMP  r3, #0                         \n"                                     \
        "    ITTTE  NE                           \n"                                     \
        "    MCRNE  p0,#0x1,r7,c1,c0,#6          \n" /* write biquad0*/                  \
        "    MRCNE  p0,#0x1,r7,c1,c0,#0          \n" /* read  biquad0*/                  \
        "    MRCNE  p0,#0x1,r4,c3,c0,#0          \n" /* read  biquad1*/                  \
        "    MRCEQ  p0,#0x1,r4,c3,c0,#0          \n" /* read  biquad1*/                  \
        "    MCR  p0,#0x1,r5,c3,c0,#6            \n" /* write biquad1*/                  \
        "    MCR  p0,#0x1,r4,c1,c0,#6            \n" /* write biquad0*/                  \
        "    CMP  r3, #0                         \n"                                     \
        "    ITTE  NE                            \n"                                     \
        "    STRHNE r6,[r1],#2                   \n" /* store last two results*/         \
        "    STRHNE r7,[r1],#2                   \n" /* store last two results*/         \
        "    MOVEQ r3, #1                        \n" /* middle = 1 */                    \
        "    LDRSH r6,[r0],#2                    \n" /* load next 2 of the 8*/           \
        "    LDRSH r7,[r0],#2                    \n" /* load next 2 of the 8*/           \
        "    MRC  p0,#0x1,r4,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MRC  p0,#0x1,r5,c3,c0,#0            \n" /* read  biquad1*/                  \
        "    MCR  p0,#0x1,r6,c3,c0,#6            \n" /* write biquad1*/                  \
        "    MCR  p0,#0x1,r5,c1,c0,#6            \n" /* write biquad0*/                  \
        "    MRC  p0,#0x1,r5,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MRC  p0,#0x1,r6,c3,c0,#0            \n" /* read  biquad1*/                  \
        "    LDRSH r8,[r0],#2                    \n" /* load next 2 of the 8*/           \
        "    LDRSH r9,[r0],#2                    \n" /* load next 2 of the 8*/           \
        "    MCR  p0,#0x1,r7,c3,c0,#6            \n" /* write biquad1*/                  \
        "    MCR  p0,#0x1,r6,c1,c0,#6            \n" /* write biquad0*/                  \
        "    MRC  p0,#0x1,r6,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MRC  p0,#0x1,r7,c3,c0,#0            \n" /* read  biquad1*/                  \
        "    STRH r4,[r1],#2                     \n" /* store first 4 results */         \
        "    STRH r5,[r1],#2                     \n" /* store first 4 results */         \
        "    MCR  p0,#0x1,r8,c3,c0,#6            \n" /* write biquad1*/                  \
        "    MCR  p0,#0x1,r7,c1,c0,#6            \n" /* write biquad0*/                  \
        "    MRC  p0,#0x1,r7,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MRC  p0,#0x1,r8,c3,c0,#0            \n" /* read  biquad1*/                  \
        "    MCR  p0,#0x1,r9,c3,c0,#6            \n" /* write biquad1*/                  \
        "    MCR  p0,#0x1,r8,c1,c0,#6            \n" /* write biquad0*/                  \
        "    STRH r6,[r1],#2                     \n" /* store first 4 results */         \
        "    STRH r7,[r1],#2                     \n" /* store first 4 results */         \
        "    LDRSH r6,[r0],#2                    \n" /* load last 2 of the 8*/           \
        "    LDRSH r7,[r0],#2                    \n" /* load last 2 of the 8*/           \
        "    MRC  p0,#0x1,r8,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MRC  p0,#0x1,r9,c3,c0,#0            \n" /* read  biquad1*/                  \
        "    MCR  p0,#0x1,r6,c3,c0,#6            \n" /* write biquad1*/                  \
        "    MCR  p0,#0x1,r9,c1,c0,#6            \n" /* write biquad0*/                  \
        "    SUBS r2, r2, #8                     \n" /* length -= 8; if (length != 0) */ \
        "    ITT   NE                            \n"                                     \
        "    LDRSHNE r4,[r0],#2                  \n" /* load first two of next 8*/       \
        "    LDRSHNE r5,[r0],#2                  \n" /* load first two of next 8*/       \
        "    MRC  p0,#0x1,r9,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MRC  p0,#0x1,r6,c3,c0,#0            \n" /* read  biquad1*/                  \
        "    MCR  p0,#0x1,r7,c3,c0,#6            \n" /* write biquad1*/                  \
        "    MCR  p0,#0x1,r6,c1,c0,#6            \n" /* write biquad0*/                  \
        "    STRH r8,[r1],#2                     \n" /* store third two results */       \
        "    STRH r9,[r1],#2                     \n" /* store third two results */       \
        "    MRC  p0,#0x1,r6,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    MRC  p0,#0x1,r7,c3,c0,#0            \n" /* read  biquad1*/                  \
        "    CMP  r2, #0                         \n" /* if (length == 0) */              \
        "    BNE  1b                             \n"                                     \
        "    MCR  p0,#0x1,r7,c1,c0,#6            \n" /* write biquad0*/                  \
        "    MRC  p0,#0x1,r7,c1,c0,#0            \n" /* read  biquad0*/                  \
        "    STRH r6,[r1],#2                     \n" /* store fourth two results */      \
        "    STRH r7,[r1],#2                     \n" /* store fourth two results */      \
        )

/*! @brief Make the length used for matrix functions. */
#define POWERQUAD_MAKE_MATRIX_LEN(mat1Row, mat1Col, mat2Col) \
    (((uint32_t)(mat1Row) << 0U) | ((uint32_t)(mat1Col) << 8U) | ((uint32_t)(mat2Col) << 16U))

/*! @brief Convert Q31 to float. */
#define PQ_Q31_2_FLOAT(x) (((float)(x)) / 2147483648.0f)

/*! @brief Convert Q15 to float. */
#define PQ_Q15_2_FLOAT(x) (((float)(x)) / 32768.0f)

/*! @brief powerquad computation engine */
typedef enum
{
    kPQ_CP_PQ = 0,    /*!< Math engine.*/
    kPQ_CP_MTX = 1,   /*!< Matrix engine.*/
    kPQ_CP_FFT = 2,   /*!< FFT engine.*/
    kPQ_CP_FIR = 3,   /*!< FIR engine.*/
    kPQ_CP_CORDIC = 5 /*!< CORDIC engine.*/
} pq_computationengine_t;

/*! @brief powerquad data structure format type */
typedef enum
{
    kPQ_16Bit = 0, /*!< Int16 Fixed point.*/
    kPQ_32Bit = 1, /*!< Int32 Fixed point.*/
    kPQ_Float = 2  /*!< Float point.*/
} pq_format_t;

/*! @brief Coprocessor prescale */
typedef struct
{
    int8_t inputPrescale;  /*!< Input prescale.*/
    int8_t outputPrescale; /*!< Output prescale.*/
    int8_t outputSaturate; /*!< Output saturate at n bits, for example 0x11 is 8 bit space,
                                  the value will be truncated at +127 or -128.*/
} pq_prescale_t;

/*! @brief powerquad data structure format */
typedef struct
{
    pq_format_t inputAFormat;  /*!< Input A format.*/
    int8_t inputAPrescale;     /*!< Input A prescale, for example 1.5 can be 1.5*2^n if you scale by 'shifting'
                                   ('scaling' by a factor of n).*/
    pq_format_t inputBFormat;  /*!< Input B format.*/
    int8_t inputBPrescale;     /*!< Input B prescale.*/
    pq_format_t outputFormat;  /*!< Out format.*/
    int8_t outputPrescale;     /*!< Out prescale.*/
    pq_format_t tmpFormat;     /*!< Temp format.*/
    int8_t tmpPrescale;        /*!< Temp prescale.*/
    pq_format_t machineFormat; /*!< Machine format.*/
    uint32_t *tmpBase;         /*!< Tmp base address.*/
} pq_config_t;

/*! @brief Struct to save biquad parameters. */
typedef struct _pq_biquad_param
{
    float v_n_1; /*!< v[n-1], set to 0 when initialization. */
    float v_n;   /*!< v[n], set to 0 when initialization.  */
    float a_1;   /*!< a[1] */
    float a_2;   /*!< a[2] */
    float b_0;   /*!< b[0] */
    float b_1;   /*!< b[1] */
    float b_2;   /*!< b[2] */
} pq_biquad_param_t;

/*! @brief Struct to save biquad state. */
typedef struct _pq_biquad_state
{
    pq_biquad_param_t param; /*!< Filter parameter. */
    uint32_t compreg;        /*!< Internal register, set to 0 when initialization. */
} pq_biquad_state_t;

/*! @brief Instance structure for the direct form II Biquad cascade filter */
typedef struct
{
    uint8_t numStages;         /**< Number of 2nd order stages in the filter.*/
    pq_biquad_state_t *pState; /**< Points to the array of state coefficients.*/
} pq_biquad_cascade_df2_instance;

/*! @brief CORDIC iteration */
typedef enum
{
    kPQ_Iteration_8 = 0, /*!< Iterate 8 times.*/
    kPQ_Iteration_16,    /*!< Iterate 16 times.*/
    kPQ_Iteration_24     /*!< Iterate 24 times.*/
} pq_cordic_iter_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @name POWERQUAD functional Operation
 * @{
 */

/*!
 * @brief Get default configuration.
 *
 * This function initializes the POWERQUAD configuration structure to a default value.
 * FORMAT register field definitions
 *   Bits[15:8] scaler (for scaled 'q31' formats)
 *   Bits[5:4] external format. 00b=q15, 01b=q31, 10b=float
 *   Bits[1:0] internal format. 00b=q15, 01b=q31, 10b=float
 *   POWERQUAD->INAFORMAT = (config->inputAPrescale << 8) | (config->inputAFormat << 4) | config->machineFormat
 *
 * For all Powerquad operations internal format must be float (with the only exception being
 * the FFT related functions, ie FFT/IFFT/DCT/IDCT which must be set to q31).
 * The default values are:
 *   config->inputAFormat = kPQ_Float;
 *   config->inputAPrescale = 0;
 *   config->inputBFormat = kPQ_Float;
 *   config->inputBPrescale = 0;
 *   config->outputFormat = kPQ_Float;
 *   config->outputPrescale = 0;
 *   config->tmpFormat = kPQ_Float;
 *   config->tmpPrescale = 0;
 *   config->machineFormat = kPQ_Float;
 *   config->tmpBase = 0xE0000000;
 *
 * @param config Pointer to "pq_config_t" structure.
 */
void PQ_GetDefaultConfig(pq_config_t *config);

/*!
 * @brief Set configuration with format/prescale.
 *
 * @param base  POWERQUAD peripheral base address
 * @param config Pointer to "pq_config_t" structure.
 */
void PQ_SetConfig(POWERQUAD_Type *base, const pq_config_t *config);

/*!
 * @brief set coprocessor scaler for coprocessor instructions, this function is used to
 * set output saturation and scaleing for input/output.
 *
 * @param base  POWERQUAD peripheral base address
 * @param prescale Pointer to "pq_prescale_t" structure.
 */
static inline void PQ_SetCoprocessorScaler(POWERQUAD_Type *base, const pq_prescale_t *prescale)
{
    assert(prescale);

    base->CPPRE = POWERQUAD_CPPRE_CPPRE_IN(prescale->inputPrescale) |
                  POWERQUAD_CPPRE_CPPRE_OUT(prescale->outputPrescale) |
                  ((uint32_t)prescale->outputSaturate << POWERQUAD_CPPRE_CPPRE_SAT_SHIFT);
}

/*!
 * @brief Initializes the POWERQUAD module.
 *
 * @param base   POWERQUAD peripheral base address.
 */
void PQ_Init(POWERQUAD_Type *base);

/*!
 * @brief De-initializes the POWERQUAD module.
 *
 * @param base POWERQUAD peripheral base address.
 */
void PQ_Deinit(POWERQUAD_Type *base);

/*!
 * @brief Set format for non-coprecessor instructions.
 *
 * @param base  POWERQUAD peripheral base address
 * @param engine Computation engine
 * @param format Data format
 */
void PQ_SetFormat(POWERQUAD_Type *base, pq_computationengine_t engine, pq_format_t format);

/*!
 * @brief Wait for the completion.
 *
 * @param base  POWERQUAD peripheral base address
 */
static inline void PQ_WaitDone(POWERQUAD_Type *base)
{
    /* wait for the completion */
    while ((base->CONTROL & INST_BUSY) == INST_BUSY)
    {
        __WFE();
    }
}

/*!
 * @brief Processing function for the floating-point natural log.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 */
static inline void PQ_LnF32(float *pSrc, float *pDst)
{
    _pq_ln0(*(int32_t *)pSrc);
    *(int32_t *)pDst = _pq_readAdd0();
}

/*!
 * @brief Processing function for the floating-point reciprocal.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 */
static inline void PQ_InvF32(float *pSrc, float *pDst)
{
    _pq_inv0(*(int32_t *)pSrc);
    *(int32_t *)pDst = _pq_readMult0();
}

/*!
 * @brief Processing function for the floating-point square-root.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 */
static inline void PQ_SqrtF32(float *pSrc, float *pDst)
{
    _pq_sqrt0(*(int32_t *)pSrc);
    *(int32_t *)pDst = _pq_readMult0();
}

/*!
 * @brief Processing function for the floating-point inverse square-root.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 */
static inline void PQ_InvSqrtF32(float *pSrc, float *pDst)
{
    _pq_invsqrt0(*(int32_t *)pSrc);
    *(int32_t *)pDst = _pq_readMult0();
}

/*!
 * @brief Processing function for the floating-point natural exponent.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 */
static inline void PQ_EtoxF32(float *pSrc, float *pDst)
{
    _pq_etox0(*(int32_t *)pSrc);
    *(int32_t *)pDst = _pq_readMult0();
}

/*!
 * @brief Processing function for the floating-point natural exponent with negative parameter.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 */
static inline void PQ_EtonxF32(float *pSrc, float *pDst)
{
    _pq_etonx0(*(int32_t *)pSrc);
    *(int32_t *)pDst = _pq_readMult0();
}

/*!
 * @brief Processing function for the floating-point sine.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 */
static inline void PQ_SinF32(float *pSrc, float *pDst)
{
    _pq_sin0(*(int32_t *)pSrc);
    *(int32_t *)pDst = _pq_readAdd0();
}

/*!
 * @brief Processing function for the floating-point cosine.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 */
static inline void PQ_CosF32(float *pSrc, float *pDst)
{
    _pq_cos0(*(int32_t *)pSrc);
    *(int32_t *)pDst = _pq_readAdd0();
}

/*!
 * @brief Processing function for the floating-point biquad.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 */
static inline void PQ_BiquadF32(float *pSrc, float *pDst)
{
    _pq_biquad0(*(int32_t *)pSrc);
    *(int32_t *)pDst = _pq_readAdd0();
}

/*!
 * @brief Processing function for the floating-point division.
 *
 * Get x1 / x2.
 *
 * @param  x1 x1
 * @param  x2 x2
 * @param  *pDst      points to the block of output data
 */
static inline void PQ_DivF32(float *x1, float *x2, float *pDst)
{
    uint32_t X1 = *(uint32_t *)x1;
    uint32_t X2 = *(uint32_t *)x2;
    uint64_t input = (uint64_t)(X2) | ((uint64_t)(X1) << 32U);

    _pq_div0(input);
    *(int32_t *)pDst = _pq_readMult0();
}

/*!
 * @brief Processing function for the floating-point biquad.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 */
static inline void PQ_Biquad1F32(float *pSrc, float *pDst)
{
    _pq_biquad1(*(int32_t *)pSrc);
    *(int32_t *)pDst = _pq_readAdd1();
}

/*!
 * @brief Processing function for the fixed natural log.
 *
 * @param val value to be calculated
 * @return returns ln(val).
 */
static inline int32_t PQ_LnFixed(int32_t val)
{
    _pq_ln_fx0(val);
    return _pq_readAdd0_fx();
}

/*!
 * @brief Processing function for the fixed reciprocal.
 *
 * @param val value to be calculated
 * @return returns inv(val).
 */
static inline int32_t PQ_InvFixed(int32_t val)
{
    _pq_inv_fx0(val);
    return _pq_readMult0_fx();
}

/*!
 * @brief Processing function for the fixed square-root.
 *
 * @param val value to be calculated
 * @return returns sqrt(val).
 */
static inline uint32_t PQ_SqrtFixed(uint32_t val)
{
    _pq_sqrt_fx0(val);
    return _pq_readMult0_fx();
}

/*!
 * @brief Processing function for the fixed inverse square-root.
 *
 * @param val value to be calculated
 * @return returns 1/sqrt(val).
 */
static inline int32_t PQ_InvSqrtFixed(int32_t val)
{
    _pq_invsqrt_fx0(val);
    return _pq_readMult0_fx();
}

/*!
 * @brief Processing function for the Fixed natural exponent.
 *
 * @param val value to be calculated
 * @return returns etox^(val).
 */
static inline int32_t PQ_EtoxFixed(int32_t val)
{
    _pq_etox_fx0(val);
    return _pq_readMult0_fx();
}

/*!
 * @brief Processing function for the fixed natural exponent with negative parameter.
 *
 * @param val value to be calculated
 * @return returns etonx^(val).
 */
static inline int32_t PQ_EtonxFixed(int32_t val)
{
    _pq_etonx_fx0(val);
    return _pq_readMult0_fx();
}

/*!
 * @brief Processing function for the fixed sine.
 *
 * @param val value to be calculated
 * @return returns sin(val).
 */
static inline int32_t PQ_SinQ31(int32_t val)
{
    int32_t ret;
    uint32_t cppre;
#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    const int32_t magic = 0x30c90fdb;
    float valFloat = *(const float *)(&magic) * (float)val;
    val = *(int32_t *)(&valFloat);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif

    cppre = POWERQUAD->CPPRE;
    POWERQUAD->CPPRE = POWERQUAD_CPPRE_CPPRE_OUT(31);

#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
    _pq_sin0(val);

    ret = _pq_readAdd0();
    ret = _pq_readAdd0_fx();
#else
    _pq_sin_fx0(val);
    ret = _pq_readAdd0_fx();
#endif

    POWERQUAD->CPPRE = cppre;

    return ret;
}

/*!
 * @brief Processing function for the fixed sine.
 *
 * @param val value to be calculated
 * @return returns sin(val).
 */
static inline int16_t PQ_SinQ15(int16_t val)
{
    int32_t ret;
    uint32_t cppre;
#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    int32_t val32 = val;
    const int32_t magic = 0x30c90fdb;
    float valFloat = *(const float *)(&magic) * (float)(val32 << 16);
    val32 = *(int32_t *)(&valFloat);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif

    cppre = POWERQUAD->CPPRE;
    /* Don't use 15 here, it is wrong then val is 0x4000 */
    POWERQUAD->CPPRE = POWERQUAD_CPPRE_CPPRE_OUT(31);

#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
    _pq_sin0(val32);

    ret = _pq_readAdd0();
    ret = _pq_readAdd0_fx() >> 16;
#else
    _pq_sin_fx0((uint32_t)val << 16);
    ret = (_pq_readAdd0_fx()) >> 16;
#endif

    POWERQUAD->CPPRE = cppre;

    return (int16_t)ret;
}

/*!
 * @brief Processing function for the fixed cosine.
 *
 * @param val value to be calculated
 * @return returns cos(val).
 */
static inline int32_t PQ_CosQ31(int32_t val)
{
    int32_t ret;
    uint32_t cppre;
#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    const int32_t magic = 0x30c90fdb;
    float valFloat = *(const float *)(&magic) * (float)val;
    val = *(int32_t *)(&valFloat);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif

    cppre = POWERQUAD->CPPRE;
    POWERQUAD->CPPRE = POWERQUAD_CPPRE_CPPRE_OUT(31);

#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
    _pq_cos0(val);

    ret = _pq_readAdd0();
    ret = _pq_readAdd0_fx();
#else
    _pq_cos_fx0(val);
    ret = _pq_readAdd0_fx();
#endif

    POWERQUAD->CPPRE = cppre;

    return ret;
}

/*!
 * @brief Processing function for the fixed sine.
 *
 * @param val value to be calculated
 * @return returns sin(val).
 */
static inline int16_t PQ_CosQ15(int16_t val)
{
    int32_t ret;
    uint32_t cppre;
#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    int32_t val32 = val;
    const int32_t magic = 0x30c90fdb;
    float valFloat = *(const float *)(&magic) * (float)(val32 << 16);
    val32 = *(int32_t *)(&valFloat);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif

    cppre = POWERQUAD->CPPRE;
    POWERQUAD->CPPRE = POWERQUAD_CPPRE_CPPRE_OUT(31);

#if defined(FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA) && FSL_FEATURE_POWERQUAD_SIN_COS_FIX_ERRATA
    _pq_cos0(val32);

    ret = _pq_readAdd0();
    ret = _pq_readAdd0_fx() >> 16;
#else
    _pq_cos_fx0((uint32_t)val << 16);
    ret = (_pq_readAdd0_fx()) >> 16;
#endif

    POWERQUAD->CPPRE = cppre;

    return (int16_t)ret;
}

/*!
 * @brief Processing function for the fixed biquad.
 *
 * @param val value to be calculated
 * @return returns biquad(val).
 */
static inline int32_t PQ_BiquadFixed(int32_t val)
{
    _pq_biquad0_fx(val);
    return _pq_readAdd0_fx();
}

/*!
 * @brief Processing function for the floating-point vectorised natural log.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorLnF32(float *pSrc, float *pDst, int32_t length);

/*!
 * @brief Processing function for the floating-point vectorised reciprocal.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorInvF32(float *pSrc, float *pDst, int32_t length);

/*!
 * @brief Processing function for the floating-point vectorised square-root.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorSqrtF32(float *pSrc, float *pDst, int32_t length);

/*!
 * @brief Processing function for the floating-point vectorised inverse square-root.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorInvSqrtF32(float *pSrc, float *pDst, int32_t length);

/*!
 * @brief Processing function for the floating-point vectorised natural exponent.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorEtoxF32(float *pSrc, float *pDst, int32_t length);

/*!
 * @brief Processing function for the floating-point vectorised natural exponent with negative parameter.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorEtonxF32(float *pSrc, float *pDst, int32_t length);

/*!
 * @brief Processing function for the floating-point vectorised sine
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorSinF32(float *pSrc, float *pDst, int32_t length);

/*!
 * @brief Processing function for the floating-point vectorised cosine.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorCosF32(float *pSrc, float *pDst, int32_t length);

/*!
 * @brief Processing function for the Q31 vectorised natural log.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorLnFixed32(int32_t *pSrc, int32_t *pDst, int32_t length);

/*!
 * @brief Processing function for the Q31 vectorised reciprocal.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorInvFixed32(int32_t *pSrc, int32_t *pDst, int32_t length);

/*!
 * @brief Processing function for the 32-bit integer vectorised square-root.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorSqrtFixed32(int32_t *pSrc, int32_t *pDst, int32_t length);

/*!
 * @brief Processing function for the 32-bit integer vectorised inverse square-root.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorInvSqrtFixed32(int32_t *pSrc, int32_t *pDst, int32_t length);

/*!
 * @brief Processing function for the 32-bit integer vectorised natural exponent.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorEtoxFixed32(int32_t *pSrc, int32_t *pDst, int32_t length);

/*!
 * @brief Processing function for the 32-bit integer vectorised natural exponent with negative parameter.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorEtonxFixed32(int32_t *pSrc, int32_t *pDst, int32_t length);

/*!
 * @brief Processing function for the Q15 vectorised sine
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorSinQ15(int16_t *pSrc, int16_t *pDst, int32_t length);

/*!
 * @brief Processing function for the Q15 vectorised cosine.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorCosQ15(int16_t *pSrc, int16_t *pDst, int32_t length);

/*!
 * @brief Processing function for the Q31 vectorised sine
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorSinQ31(int32_t *pSrc, int32_t *pDst, int32_t length);

/*!
 * @brief Processing function for the Q31 vectorised cosine.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorCosQ31(int32_t *pSrc, int32_t *pDst, int32_t length);

/*!
 * @brief Processing function for the 16-bit integer vectorised natural log.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorLnFixed16(int16_t *pSrc, int16_t *pDst, int32_t length);

/*!
 * @brief Processing function for the 16-bit integer vectorised reciprocal.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorInvFixed16(int16_t *pSrc, int16_t *pDst, int32_t length);

/*!
 * @brief Processing function for the 16-bit integer vectorised square-root.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorSqrtFixed16(int16_t *pSrc, int16_t *pDst, int32_t length);

/*!
 * @brief Processing function for the 16-bit integer vectorised inverse square-root.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorInvSqrtFixed16(int16_t *pSrc, int16_t *pDst, int32_t length);

/*!
 * @brief Processing function for the 16-bit integer vectorised natural exponent.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorEtoxFixed16(int16_t *pSrc, int16_t *pDst, int32_t length);

/*!
 * @brief Processing function for the 16-bit integer vectorised natural exponent with negative parameter.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length     the block of input data.
 */
void PQ_VectorEtonxFixed16(int16_t *pSrc, int16_t *pDst, int32_t length);

/*!
 * @brief Processing function for the floating-point vectorised biquad direct form II.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  length the block size of input data.
 */
void PQ_VectorBiqaudDf2F32(float *pSrc, float *pDst, int32_t length);

/*!
 * @brief Processing function for the 32-bit integer vectorised biquad direct form II.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  blocksSize the block size of input data
 */
void PQ_VectorBiqaudDf2Fixed32(int32_t *pSrc, int32_t *pDst, int32_t length);

/*!
 * @brief Processing function for the 16-bit integer vectorised biquad direct form II.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  blocksSize the block size of input data
 */
void PQ_VectorBiqaudDf2Fixed16(int16_t *pSrc, int16_t *pDst, int32_t length);

/*!
 * @brief Processing function for the floating-point vectorised biquad direct form II.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  blocksSize the block size of input data
 */
void PQ_VectorBiqaudCascadeDf2F32(float *pSrc, float *pDst, int32_t length);

/*!
 * @brief Processing function for the 32-bit integer vectorised biquad direct form II.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  blocksSize the block size of input data
 */
void PQ_VectorBiqaudCascadeDf2Fixed32(int32_t *pSrc, int32_t *pDst, int32_t length);

/*!
 * @brief Processing function for the 16-bit integer vectorised biquad direct form II.
 *
 * @param  *pSrc      points to the block of input data
 * @param  *pDst      points to the block of output data
 * @param  blocksSize the block size of input data
 */
void PQ_VectorBiqaudCascadeDf2Fixed16(int16_t *pSrc, int16_t *pDst, int32_t length);

/*!
 * @brief Processing function for the fixed inverse trigonometric.
 *
 * @param base  POWERQUAD peripheral base address
 * @param x value of opposite
 * @param y value of adjacent
 * @param iteration iteration times
 * @return The return value is in the range of -2^27 to 2^27, which means -pi to pi.
 * @note The sum of x and y should not exceed the range of int32_t.
 * @note Larger input number gets higher output accuracy, for example the arctan(0.5),
 * the result of PQ_ArctanFixed(POWERQUAD, 100000, 200000, kPQ_Iteration_24) is more
 * accurate than PQ_ArctanFixed(POWERQUAD, 1, 2, kPQ_Iteration_24).
 */
int32_t PQ_ArctanFixed(POWERQUAD_Type *base, int32_t x, int32_t y, pq_cordic_iter_t iteration);

/*!
 * @brief Processing function for the fixed inverse trigonometric.
 *
 * @param base  POWERQUAD peripheral base address
 * @param x value of opposite
 * @param y value of adjacent
 * @param iteration iteration times
 * @return The return value is in the range of -2^27 to 2^27, which means -1 to 1.
 * @note The sum of x and y should not exceed the range of int32_t.
 * @note Larger input number gets higher output accuracy, for example the arctanh(0.5),
 * the result of PQ_ArctanhFixed(POWERQUAD, 100000, 200000, kPQ_Iteration_24) is more
 * accurate than PQ_ArctanhFixed(POWERQUAD, 1, 2, kPQ_Iteration_24).
 */
int32_t PQ_ArctanhFixed(POWERQUAD_Type *base, int32_t x, int32_t y, pq_cordic_iter_t iteration);

/*!
 * @brief Processing function for the fixed biquad.
 *
 * @param val value to be calculated
 * @return returns biquad(val).
 */
static inline int32_t PQ_Biquad1Fixed(int32_t val)
{
    _pq_biquad1_fx(val);
    return _pq_readAdd1_fx();
}

/*!
 * @brief Processing function for the complex FFT.
 *
 * @param base  POWERQUAD peripheral base address
 * @param length number of input samples
 * @param pData input data
 * @param pResult output data.
 */
void PQ_TransformCFFT(POWERQUAD_Type *base, uint32_t length, void *pData, void *pResult);

/*!
 * @brief Processing function for the real FFT.
 *
 * @param base  POWERQUAD peripheral base address
 * @param length number of input samples
 * @param pData input data
 * @param pResult output data.
 */
void PQ_TransformRFFT(POWERQUAD_Type *base, uint32_t length, void *pData, void *pResult);

/*!
 * @brief Processing function for the inverse complex FFT.
 *
 * @param base  POWERQUAD peripheral base address
 * @param length number of input samples
 * @param pData input data
 * @param pResult output data.
 */
void PQ_TransformIFFT(POWERQUAD_Type *base, uint32_t length, void *pData, void *pResult);

/*!
 * @brief Processing function for the complex DCT.
 *
 * @param base  POWERQUAD peripheral base address
 * @param length number of input samples
 * @param pData input data
 * @param pResult output data.
 */
void PQ_TransformCDCT(POWERQUAD_Type *base, uint32_t length, void *pData, void *pResult);

/*!
 * @brief Processing function for the real DCT.
 *
 * @param base  POWERQUAD peripheral base address
 * @param length number of input samples
 * @param pData input data
 * @param pResult output data.
 */
void PQ_TransformRDCT(POWERQUAD_Type *base, uint32_t length, void *pData, void *pResult);

/*!
 * @brief Processing function for the inverse complex DCT.
 *
 * @param base  POWERQUAD peripheral base address
 * @param length number of input samples
 * @param pData input data
 * @param pResult output data.
 */
void PQ_TransformIDCT(POWERQUAD_Type *base, uint32_t length, void *pData, void *pResult);

/*!
 * @brief Processing function for backup biquad context.
 *
 * @param base  POWERQUAD peripheral base address
 * @param biquad_num biquad side
 * @param state point to states.
 */
void PQ_BiquadBackUpInternalState(POWERQUAD_Type *base, int32_t biquad_num, pq_biquad_state_t *state);

/*!
 * @brief Processing function for restore biquad context.
 *
 * @param base  POWERQUAD peripheral base address
 * @param biquad_num biquad side
 * @param state point to states.
 */
void PQ_BiquadRestoreInternalState(POWERQUAD_Type *base, int32_t biquad_num, pq_biquad_state_t *state);

/*!
 * @brief  Initialization function for the direct form II Biquad cascade filter.
 *
 * @param[in,out] *S           points to an instance of the filter data structure.
 * @param[in]     numStages    number of 2nd order stages in the filter.
 * @param[in]     *pState      points to the state buffer.
 */
void PQ_BiquadCascadeDf2Init(pq_biquad_cascade_df2_instance *S, uint8_t numStages, pq_biquad_state_t *pState);

/*!
 * @brief Processing function for the floating-point direct form II Biquad cascade filter.
 *
 * @param[in]  *S        points to an instance of the filter data structure.
 * @param[in]  *pSrc     points to the block of input data.
 * @param[out] *pDst     points to the block of output data
 * @param[in]  blockSize number of samples to process.
 */
void PQ_BiquadCascadeDf2F32(const pq_biquad_cascade_df2_instance *S, float *pSrc, float *pDst, uint32_t blockSize);

/*!
 * @brief Processing function for the Q31 direct form II Biquad cascade filter.
 *
 * @param[in]  *S        points to an instance of the filter data structure.
 * @param[in]  *pSrc     points to the block of input data.
 * @param[out] *pDst     points to the block of output data
 * @param[in]  blockSize number of samples to process.
 */
void PQ_BiquadCascadeDf2Fixed32(const pq_biquad_cascade_df2_instance *S,
                                int32_t *pSrc,
                                int32_t *pDst,
                                uint32_t blockSize);

/*!
 * @brief Processing function for the Q15 direct form II Biquad cascade filter.
 *
 * @param[in]  *S        points to an instance of the filter data structure.
 * @param[in]  *pSrc     points to the block of input data.
 * @param[out] *pDst     points to the block of output data
 * @param[in]  blockSize number of samples to process.
 */
void PQ_BiquadCascadeDf2Fixed16(const pq_biquad_cascade_df2_instance *S,
                                int16_t *pSrc,
                                int16_t *pDst,
                                uint32_t blockSize);

/*!
 * @brief Processing function for the FIR.
 *
 * @param base  POWERQUAD peripheral base address
 * @param pAData the first input sequence
 * @param ALength number of the first input sequence
 * @param pBData the second input sequence
 * @param BLength number of the second input sequence
 * @param pResult array for the output data
 * @param opType operation type, could be PQ_FIR_FIR, PQ_FIR_CONVOLUTION, PQ_FIR_CORRELATION.
 */
void PQ_FIR(
    POWERQUAD_Type *base, void *pAData, int32_t ALength, void *pBData, int32_t BLength, void *pResult, uint32_t opType);

/*!
 * @brief Processing function for the incremental FIR.
 *        This function can be used after pq_fir() for incremental FIR
 *        operation when new x data are available
 *
 * @param base  POWERQUAD peripheral base address
 * @param ALength number of input samples
 * @param BLength number of taps
 * @param xoffset offset for number of input samples
 */
void PQ_FIRIncrement(POWERQUAD_Type *base, int32_t ALength, int32_t BLength, int32_t xOffset);

/*!
 * @brief Processing function for the matrix addition.
 *
 * @param base  POWERQUAD peripheral base address
 * @param length rows and cols for matrix. LENGTH register configuration:
 *        LENGTH[23:16] = M2 cols
 *        LENGTH[15:8]  = M1 cols
 *        LENGTH[7:0]   = M1 rows
 *        This could be constructed using macro @ref POWERQUAD_MAKE_MATRIX_LEN.
 * @param pAData input matrix A
 * @param pBData input matrix B
 * @param pResult array for the output data.
 */
void PQ_MatrixAddition(POWERQUAD_Type *base, uint32_t length, void *pAData, void *pBData, void *pResult);

/*!
 * @brief Processing function for the matrix subtraction.
 *
 * @param base  POWERQUAD peripheral base address
 * @param length rows and cols for matrix. LENGTH register configuration:
 *        LENGTH[23:16] = M2 cols
 *        LENGTH[15:8]  = M1 cols
 *        LENGTH[7:0]   = M1 rows
 *        This could be constructed using macro @ref POWERQUAD_MAKE_MATRIX_LEN.
 * @param pAData input matrix A
 * @param pBData input matrix B
 * @param pResult array for the output data.
 */
void PQ_MatrixSubtraction(POWERQUAD_Type *base, uint32_t length, void *pAData, void *pBData, void *pResult);

/*!
 * @brief Processing function for the matrix multiplication.
 *
 * @param base  POWERQUAD peripheral base address
 * @param length rows and cols for matrix. LENGTH register configuration:
 *        LENGTH[23:16] = M2 cols
 *        LENGTH[15:8]  = M1 cols
 *        LENGTH[7:0]   = M1 rows
 *        This could be constructed using macro @ref POWERQUAD_MAKE_MATRIX_LEN.
 * @param pAData input matrix A
 * @param pBData input matrix B
 * @param pResult array for the output data.
 */
void PQ_MatrixMultiplication(POWERQUAD_Type *base, uint32_t length, void *pAData, void *pBData, void *pResult);

/*!
 * @brief Processing function for the matrix product.
 *
 * @param base  POWERQUAD peripheral base address
 * @param length rows and cols for matrix. LENGTH register configuration:
 *        LENGTH[23:16] = M2 cols
 *        LENGTH[15:8]  = M1 cols
 *        LENGTH[7:0]   = M1 rows
 *        This could be constructed using macro @ref POWERQUAD_MAKE_MATRIX_LEN.
 * @param pAData input matrix A
 * @param pBData input matrix B
 * @param pResult array for the output data.
 */
void PQ_MatrixProduct(POWERQUAD_Type *base, uint32_t length, void *pAData, void *pBData, void *pResult);

/*!
 * @brief Processing function for the vector dot product.
 *
 * @param base  POWERQUAD peripheral base address
 * @param length length of vector
 * @param pAData input vector A
 * @param pBData input vector B
 * @param pResult array for the output data.
 */
void PQ_VectorDotProduct(POWERQUAD_Type *base, uint32_t length, void *pAData, void *pBData, void *pResult);

/*!
 * @brief Processing function for the matrix inverse.
 *
 * @param base  POWERQUAD peripheral base address
 * @param length rows and cols for matrix. LENGTH register configuration:
 *        LENGTH[23:16] = M2 cols
 *        LENGTH[15:8]  = M1 cols
 *        LENGTH[7:0]   = M1 rows
 *        This could be constructed using macro @ref POWERQUAD_MAKE_MATRIX_LEN.
 * @param pData input matrix
 * @param pTmpData input temporary matrix, pTmpData length not less than pData lenght and 1024 words is sufficient for
 * the largest supported matrix.
 * @param pResult array for the output data, round down for fixed point.
 */
void PQ_MatrixInversion(POWERQUAD_Type *base, uint32_t length, void *pData, void *pTmpData, void *pResult);

/*!
 * @brief Processing function for the matrix transpose.
 *
 * @param base  POWERQUAD peripheral base address
 * @param length rows and cols for matrix. LENGTH register configuration:
 *        LENGTH[23:16] = M2 cols
 *        LENGTH[15:8]  = M1 cols
 *        LENGTH[7:0]   = M1 rows
 *        This could be constructed using macro @ref POWERQUAD_MAKE_MATRIX_LEN.
 * @param pData input matrix
 * @param pResult array for the output data.
 */
void PQ_MatrixTranspose(POWERQUAD_Type *base, uint32_t length, void *pData, void *pResult);

/*!
 * @brief Processing function for the matrix scale.
 *
 * @param base  POWERQUAD peripheral base address
 * @param length rows and cols for matrix. LENGTH register configuration:
 *        LENGTH[23:16] = M2 cols
 *        LENGTH[15:8]  = M1 cols
 *        LENGTH[7:0]   = M1 rows
 *        This could be constructed using macro @ref POWERQUAD_MAKE_MATRIX_LEN.
 * @param misc scaling parameters
 * @param pData input matrix
 * @param pResult array for the output data.
 */
void PQ_MatrixScale(POWERQUAD_Type *base, uint32_t length, float misc, void *pData, void *pResult);

/* @} */

#if defined(__cplusplus)
}

#endif /* __cplusplus */

/*! @}*/

#endif /* _FSL_POWERQUAD_H_ */
