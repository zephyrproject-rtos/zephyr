/**************************************************************************//**
 * @file     core_cr.h
 * @brief    CMSIS Cortex-R Core Peripheral Access Layer Header File
 * @version  V1.0.0
 * @date     17. October 2019
 ******************************************************************************/
/*
 * Copyright (c) 2009-2018 ARM Limited. All rights reserved.
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
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

#if   defined ( __ICCARM__ )
  #pragma system_include         /* treat file as system include file for MISRA check */
#elif defined (__clang__)
  #pragma clang system_header   /* treat file as system include file */
#endif

#ifndef __CORE_CR_H_GENERIC
#define __CORE_CR_H_GENERIC

#ifdef __cplusplus
 extern "C" {
#endif

/*******************************************************************************
 *                 CMSIS definitions
 ******************************************************************************/

/*  CMSIS CR definitions */
#define __CR_CMSIS_VERSION_MAIN  (1U)                                      /*!< \brief [31:16] CMSIS-Core(R) main version   */
#define __CR_CMSIS_VERSION_SUB   (0U)                                      /*!< \brief [15:0]  CMSIS-Core(R) sub version    */
#define __CR_CMSIS_VERSION       ((__CR_CMSIS_VERSION_MAIN << 16U) | \
                                   __CR_CMSIS_VERSION_SUB          )       /*!< \brief CMSIS-Core(R) version number         */

#if defined ( __CC_ARM )
  #if defined __TARGET_FPU_VFP
    #if (__FPU_PRESENT == 1)
      #define __FPU_USED       1U
    #else
      #warning "Compiler generates FPU instructions for a device without an FPU (check __FPU_PRESENT)"
      #define __FPU_USED       0U
    #endif
  #else
    #define __FPU_USED         0U
  #endif

#elif defined (__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
  #if defined __ARM_FP
    #if defined (__FPU_PRESENT) && (__FPU_PRESENT == 1U)
      #define __FPU_USED       1U
    #else
      #warning "Compiler generates FPU instructions for a device without an FPU (check __FPU_PRESENT)"
      #define __FPU_USED       0U
    #endif
  #else
    #define __FPU_USED         0U
  #endif

#elif defined ( __ICCARM__ )
  #if defined __ARMVFP__
    #if (__FPU_PRESENT == 1)
      #define __FPU_USED       1U
    #else
      #warning "Compiler generates FPU instructions for a device without an FPU (check __FPU_PRESENT)"
      #define __FPU_USED       0U
    #endif
  #else
    #define __FPU_USED         0U
  #endif

#elif defined ( __TMS470__ )
  #if defined __TI_VFP_SUPPORT__
    #if (__FPU_PRESENT == 1)
      #define __FPU_USED       1U
    #else
      #warning "Compiler generates FPU instructions for a device without an FPU (check __FPU_PRESENT)"
      #define __FPU_USED       0U
    #endif
  #else
    #define __FPU_USED         0U
  #endif

#elif defined ( __GNUC__ )
  #if defined (__VFP_FP__) && !defined(__SOFTFP__)
    #if (__FPU_PRESENT == 1)
      #define __FPU_USED       1U
    #else
      #warning "Compiler generates FPU instructions for a device without an FPU (check __FPU_PRESENT)"
      #define __FPU_USED       0U
    #endif
  #else
    #define __FPU_USED         0U
  #endif

#elif defined ( __TASKING__ )
  #if defined __FPU_VFP__
    #if (__FPU_PRESENT == 1)
      #define __FPU_USED       1U
    #else
      #error "Compiler generates FPU instructions for a device without an FPU (check __FPU_PRESENT)"
      #define __FPU_USED       0U
    #endif
  #else
    #define __FPU_USED         0U
  #endif
#endif

#include "cmsis_compiler.h"               /* CMSIS compiler specific defines */

#ifdef __cplusplus
}
#endif

#endif /* __CORE_CR_H_GENERIC */

#ifndef __CMSIS_GENERIC

#ifndef __CORE_CR_H_DEPENDANT
#define __CORE_CR_H_DEPENDANT

#ifdef __cplusplus
 extern "C" {
#endif

 /* check device defines and use defaults */
#if defined __CHECK_DEVICE_DEFINES
  #ifndef __CR_REV
    #define __CR_REV              0x0000U
    #warning "__CR_REV not defined in device header file; using default!"
  #endif

  #ifndef __FPU_PRESENT
    #define __FPU_PRESENT             0U
    #warning "__FPU_PRESENT not defined in device header file; using default!"
  #endif

  #ifndef __GIC_PRESENT
    #define __GIC_PRESENT             1U
    #warning "__GIC_PRESENT not defined in device header file; using default!"
  #endif

  #ifndef __TIM_PRESENT
    #define __TIM_PRESENT             1U
    #warning "__TIM_PRESENT not defined in device header file; using default!"
  #endif

  #ifndef __MPU_PRESENT
    #define __MPU_PRESENT             1U
    #warning "__MPU_PRESENT not defined in device header file; using default!"
  #endif
#endif

/* IO definitions (access restrictions to peripheral registers) */
#ifdef __cplusplus
  #define   __I     volatile             /*!< \brief Defines 'read only' permissions */
#else
  #define   __I     volatile const       /*!< \brief Defines 'read only' permissions */
#endif
#define     __O     volatile             /*!< \brief Defines 'write only' permissions */
#define     __IO    volatile             /*!< \brief Defines 'read / write' permissions */

/* following defines should be used for structure members */
#define     __IM     volatile const      /*!< \brief Defines 'read only' structure member permissions */
#define     __OM     volatile            /*!< \brief Defines 'write only' structure member permissions */
#define     __IOM    volatile            /*!< \brief Defines 'read / write' structure member permissions */
#define RESERVED(N, T) T RESERVED##N;    // placeholder struct members used for "reserved" areas

 /*******************************************************************************
  *                 Register Abstraction
   Core Register contain:
   - CPSR
   - CP15 Registers
   - Generic Interrupt Controller Distributor
   - Generic Interrupt Controller Interface
  ******************************************************************************/

/* Core Register CPSR */
typedef union
{
  struct
  {
    uint32_t M:5;                        /*!< \brief bit:  0.. 4  Mode field */
    uint32_t T:1;                        /*!< \brief bit:      5  Thumb execution state bit */
    uint32_t F:1;                        /*!< \brief bit:      6  FIQ mask bit */
    uint32_t I:1;                        /*!< \brief bit:      7  IRQ mask bit */
    uint32_t A:1;                        /*!< \brief bit:      8  Asynchronous abort mask bit */
    uint32_t E:1;                        /*!< \brief bit:      9  Endianness execution state bit */
    uint32_t IT1:6;                      /*!< \brief bit: 10..15  If-Then execution state bits 2-7 */
    uint32_t GE:4;                       /*!< \brief bit: 16..19  Greater than or Equal flags */
    RESERVED(0:4, uint32_t)
    uint32_t J:1;                        /*!< \brief bit:     24  Jazelle bit */
    uint32_t IT0:2;                      /*!< \brief bit: 25..26  If-Then execution state bits 0-1 */
    uint32_t Q:1;                        /*!< \brief bit:     27  Saturation condition flag */
    uint32_t V:1;                        /*!< \brief bit:     28  Overflow condition code flag */
    uint32_t C:1;                        /*!< \brief bit:     29  Carry condition code flag */
    uint32_t Z:1;                        /*!< \brief bit:     30  Zero condition code flag */
    uint32_t N:1;                        /*!< \brief bit:     31  Negative condition code flag */
  } b;                                   /*!< \brief Structure used for bit  access */
  uint32_t w;                            /*!< \brief Type      used for word access */
} CPSR_Type;



/* CPSR Register Definitions */
#define CPSR_N_Pos                       31U                                    /*!< \brief CPSR: N Position */
#define CPSR_N_Msk                       (1UL << CPSR_N_Pos)                    /*!< \brief CPSR: N Mask */

#define CPSR_Z_Pos                       30U                                    /*!< \brief CPSR: Z Position */
#define CPSR_Z_Msk                       (1UL << CPSR_Z_Pos)                    /*!< \brief CPSR: Z Mask */

#define CPSR_C_Pos                       29U                                    /*!< \brief CPSR: C Position */
#define CPSR_C_Msk                       (1UL << CPSR_C_Pos)                    /*!< \brief CPSR: C Mask */

#define CPSR_V_Pos                       28U                                    /*!< \brief CPSR: V Position */
#define CPSR_V_Msk                       (1UL << CPSR_V_Pos)                    /*!< \brief CPSR: V Mask */

#define CPSR_Q_Pos                       27U                                    /*!< \brief CPSR: Q Position */
#define CPSR_Q_Msk                       (1UL << CPSR_Q_Pos)                    /*!< \brief CPSR: Q Mask */

#define CPSR_IT0_Pos                     25U                                    /*!< \brief CPSR: IT0 Position */
#define CPSR_IT0_Msk                     (3UL << CPSR_IT0_Pos)                  /*!< \brief CPSR: IT0 Mask */

#define CPSR_J_Pos                       24U                                    /*!< \brief CPSR: J Position */
#define CPSR_J_Msk                       (1UL << CPSR_J_Pos)                    /*!< \brief CPSR: J Mask */

#define CPSR_GE_Pos                      16U                                    /*!< \brief CPSR: GE Position */
#define CPSR_GE_Msk                      (0xFUL << CPSR_GE_Pos)                 /*!< \brief CPSR: GE Mask */

#define CPSR_IT1_Pos                     10U                                    /*!< \brief CPSR: IT1 Position */
#define CPSR_IT1_Msk                     (0x3FUL << CPSR_IT1_Pos)               /*!< \brief CPSR: IT1 Mask */

#define CPSR_E_Pos                       9U                                     /*!< \brief CPSR: E Position */
#define CPSR_E_Msk                       (1UL << CPSR_E_Pos)                    /*!< \brief CPSR: E Mask */

#define CPSR_A_Pos                       8U                                     /*!< \brief CPSR: A Position */
#define CPSR_A_Msk                       (1UL << CPSR_A_Pos)                    /*!< \brief CPSR: A Mask */

#define CPSR_I_Pos                       7U                                     /*!< \brief CPSR: I Position */
#define CPSR_I_Msk                       (1UL << CPSR_I_Pos)                    /*!< \brief CPSR: I Mask */

#define CPSR_F_Pos                       6U                                     /*!< \brief CPSR: F Position */
#define CPSR_F_Msk                       (1UL << CPSR_F_Pos)                    /*!< \brief CPSR: F Mask */

#define CPSR_T_Pos                       5U                                     /*!< \brief CPSR: T Position */
#define CPSR_T_Msk                       (1UL << CPSR_T_Pos)                    /*!< \brief CPSR: T Mask */

#define CPSR_M_Pos                       0U                                     /*!< \brief CPSR: M Position */
#define CPSR_M_Msk                       (0x1FUL << CPSR_M_Pos)                 /*!< \brief CPSR: M Mask */

#define CPSR_M_USR                       0x10U                                  /*!< \brief CPSR: M User mode (PL0) */
#define CPSR_M_FIQ                       0x11U                                  /*!< \brief CPSR: M Fast Interrupt mode (PL1) */
#define CPSR_M_IRQ                       0x12U                                  /*!< \brief CPSR: M Interrupt mode (PL1) */
#define CPSR_M_SVC                       0x13U                                  /*!< \brief CPSR: M Supervisor mode (PL1) */
#define CPSR_M_MON                       0x16U                                  /*!< \brief CPSR: M Monitor mode (PL1) */
#define CPSR_M_ABT                       0x17U                                  /*!< \brief CPSR: M Abort mode (PL1) */
#define CPSR_M_HYP                       0x1AU                                  /*!< \brief CPSR: M Hypervisor mode (PL2) */
#define CPSR_M_UND                       0x1BU                                  /*!< \brief CPSR: M Undefined mode (PL1) */
#define CPSR_M_SYS                       0x1FU                                  /*!< \brief CPSR: M System mode (PL1) */

/* CP15 Register SCTLR */
typedef union
{
#if __CORTEX_R == 4 || __CORTEX_R == 5 || defined(DOXYGEN)
  struct
  {
    uint32_t M:1;                        /*!< \brief bit:     0  MPU enable */
    uint32_t A:1;                        /*!< \brief bit:     1  Alignment check enable */
    uint32_t C:1;                        /*!< \brief bit:     2  Cache enable */
    RESERVED(0:7, uint32_t)
    uint32_t SW:1;                       /*!< \brief bit:    10  SWP and SWPB enable */
    uint32_t Z:1;                        /*!< \brief bit:    11  Branch prediction enable */
    uint32_t I:1;                        /*!< \brief bit:    12  Instruction cache enable */
    uint32_t V:1;                        /*!< \brief bit:    13  Vectors bit */
    uint32_t RR:1;                       /*!< \brief bit:    14  Round Robin select */
    RESERVED(1:2, uint32_t)
    uint32_t BR:1;                       /*!< \brief bit:    17  Background region access */
    RESERVED(2:1, uint32_t)
    uint32_t DZ:1;                       /*!< \brief bit:    19  Divide-by-zero fault enable */
    RESERVED(3:1, uint32_t)
    uint32_t FI:1;                       /*!< \brief bit:    21  Fast interrupts configuration enable */
    RESERVED(4:2, uint32_t)
    uint32_t VE:1;                       /*!< \brief bit:    24  Interrupt Vectors Enable */
    uint32_t EE:1;                       /*!< \brief bit:    25  Exception Endianness */
    RESERVED(5:1, uint32_t)
    uint32_t NMFI:1;                     /*!< \brief bit:    27  Non-maskable FIQ (NMFI) support */
    uint32_t TRE:1;                      /*!< \brief bit:    28  TEX remap enable. */
    uint32_t AFE:1;                      /*!< \brief bit:    29  Access flag enable */
    uint32_t TE:1;                       /*!< \brief bit:    30  Thumb Exception enable */
    uint32_t IE:1;                       /*!< \brief bit:    31  Instruction endianness */
  } b;                                   /*!< \brief Structure used for bit  access */
#endif
#if __CORTEX_R == 7 || __CORTEX_R == 8 || defined(DOXYGEN)
  struct
  {
    uint32_t M:1;                        /*!< \brief bit:     0  MPU enable */
    uint32_t A:1;                        /*!< \brief bit:     1  Alignment check enable */
    uint32_t C:1;                        /*!< \brief bit:     2  Cache enable */
    RESERVED(0:7, uint32_t)
    uint32_t SW:1;                       /*!< \brief bit:    10  SWP and SWPB enable */
    uint32_t Z:1;                        /*!< \brief bit:    11  Branch prediction enable */
    uint32_t I:1;                        /*!< \brief bit:    12  Instruction cache enable */
    uint32_t V:1;                        /*!< \brief bit:    13  Vectors bit */
    RESERVED(1:3, uint32_t)
    uint32_t BR:1;                       /*!< \brief bit:    17  Background region access */
    RESERVED(2:1, uint32_t)
    uint32_t DZ:1;                       /*!< \brief bit:    19  Divide-by-zero fault enable */
    RESERVED(3:1, uint32_t)
    uint32_t FI:1;                       /*!< \brief bit:    21  Fast interrupts configuration enable */
    RESERVED(4:3, uint32_t)
    uint32_t EE:1;                       /*!< \brief bit:    25  Exception Endianness */
    RESERVED(5:1, uint32_t)
    uint32_t NMFI:1;                     /*!< \brief bit:    27  Non-maskable FIQ (NMFI) support */
    RESERVED(6:2, uint32_t)
    uint32_t TE:1;                       /*!< \brief bit:    30  Thumb Exception enable */
    RESERVED(7:1, uint32_t)
  } b;                                   /*!< \brief Structure used for bit  access */
#endif
#if __CORTEX_R == 52 || defined(DOXYGEN)
  struct
  {
    uint32_t M:1;                        /*!< \brief bit:     0  MPU enable */
    uint32_t A:1;                        /*!< \brief bit:     1  Alignment check enable */
    uint32_t C:1;                        /*!< \brief bit:     2  Cache enable */
    RESERVED(0:2, uint32_t)
    uint32_t CP15BEN:1;                  /*!< \brief bit:     5  CP15 barrier enable */
    RESERVED(1:1, uint32_t)
    uint32_t ITD:1;                      /*!< \brief bit:     7  If-Then disable */
    uint32_t SED:1;                      /*!< \brief bit:     8  SETEND disable */
    RESERVED(2:3, uint32_t)
    uint32_t I:1;                        /*!< \brief bit:    12  Instruction cache enable */
    RESERVED(3:3, uint32_t)
    uint32_t NTWI:1;                     /*!< \brief bit:    16  Do not trap WFI */
    uint32_t BR:1;                       /*!< \brief bit:    17  Background region access */
    uint32_t NTWE:1;                     /*!< \brief bit:    18  Do not trap WFE */
    uint32_t WXN:1;                      /*!< \brief bit:    19  Write permission implies XN */
    uint32_t UWXN:1;                     /*!< \brief bit:    20  Unprivileged write permission implies PL1 XN */
    uint32_t FI:1;                       /*!< \brief bit:    21  Fast interrupts configuration enable */
    RESERVED(4:3, uint32_t)
    uint32_t EE:1;                       /*!< \brief bit:    25  Exception Endianness */
    RESERVED(5:4, uint32_t)
    uint32_t TE:1;                       /*!< \brief bit:    30  Thumb Exception enable */
    RESERVED(6:1, uint32_t)
  } b;                                   /*!< \brief Structure used for bit  access */
#endif
  uint32_t w;                            /*!< \brief Type      used for word access */
} SCTLR_Type;

#define SCTLR_IE_Pos                     31U                                    /*!< \brief SCTLR: IE Position */
#define SCTLR_IE_Msk                     (1UL << SCTLR_IE_Pos)                  /*!< \brief SCTLR: IE Mask */

#define SCTLR_TE_Pos                     30U                                    /*!< \brief SCTLR: TE Position */
#define SCTLR_TE_Msk                     (1UL << SCTLR_TE_Pos)                  /*!< \brief SCTLR: TE Mask */

#define SCTLR_AFE_Pos                    29U                                    /*!< \brief SCTLR: AFE Position */
#define SCTLR_AFE_Msk                    (1UL << SCTLR_AFE_Pos)                 /*!< \brief SCTLR: AFE Mask */

#define SCTLR_TRE_Pos                    28U                                    /*!< \brief SCTLR: TRE Position */
#define SCTLR_TRE_Msk                    (1UL << SCTLR_TRE_Pos)                 /*!< \brief SCTLR: TRE Mask */

#define SCTLR_NMFI_Pos                   27U                                    /*!< \brief SCTLR: NMFI Position */
#define SCTLR_NMFI_Msk                   (1UL << SCTLR_NMFI_Pos)                /*!< \brief SCTLR: NMFI Mask */

#define SCTLR_EE_Pos                     25U                                    /*!< \brief SCTLR: EE Position */
#define SCTLR_EE_Msk                     (1UL << SCTLR_EE_Pos)                  /*!< \brief SCTLR: EE Mask */

#define SCTLR_VE_Pos                     24U                                    /*!< \brief SCTLR: VE Position */
#define SCTLR_VE_Msk                     (1UL << SCTLR_VE_Pos)                  /*!< \brief SCTLR: VE Mask */

#define SCTLR_FI_Pos                     21U                                    /*!< \brief SCTLR: FI Position */
#define SCTLR_FI_Msk                     (1UL << SCTLR_FI_Pos)                  /*!< \brief SCTLR: FI Mask */

#define SCTLR_UWXN_Pos                   20U                                    /*!< \brief SCTLR: UWXN Position */
#define SCTLR_UWXN_Msk                   (1UL << SCTLR_UWXN_Pos)                /*!< \brief SCTLR: UWXN Mask */

#define SCTLR_DZ_Pos                     19U                                    /*!< \brief SCTLR: DZ Position */
#define SCTLR_DZ_Msk                     (1UL << SCTLR_DZ_Pos)                  /*!< \brief SCTLR: DZ Mask */

#define SCTLR_WXN_Pos                    19U                                    /*!< \brief SCTLR: WXN Position */
#define SCTLR_WXN_Msk                    (1UL << SCTLR_WXN_Pos)                 /*!< \brief SCTLR: WXN Mask */

#define SCTLR_NTWE_Pos                   18U                                    /*!< \brief SCTLR: NTWE Position */
#define SCTLR_NTWE_Msk                   (1UL << SCTLR_NTWE_Pos)                /*!< \brief SCTLR: NTWE Mask */

#define SCTLR_BR_Pos                     17U                                    /*!< \brief SCTLR: BR Position */
#define SCTLR_BR_Msk                     (1UL << SCTLR_BR_Pos)                  /*!< \brief SCTLR: BR Mask */

#define SCTLR_NTWI_Pos                   16U                                    /*!< \brief SCTLR: NTWI Position */
#define SCTLR_NTWI_Msk                   (1UL << SCTLR_NTWI_Pos)                /*!< \brief SCTLR: NTWI Mask */

#define SCTLR_RR_Pos                     14U                                    /*!< \brief SCTLR: RR Position */
#define SCTLR_RR_Msk                     (1UL << SCTLR_RR_Pos)                  /*!< \brief SCTLR: RR Mask */

#define SCTLR_V_Pos                      13U                                    /*!< \brief SCTLR: V Position */
#define SCTLR_V_Msk                      (1UL << SCTLR_V_Pos)                   /*!< \brief SCTLR: V Mask */

#define SCTLR_I_Pos                      12U                                    /*!< \brief SCTLR: I Position */
#define SCTLR_I_Msk                      (1UL << SCTLR_I_Pos)                   /*!< \brief SCTLR: I Mask */

#define SCTLR_Z_Pos                      11U                                    /*!< \brief SCTLR: Z Position */
#define SCTLR_Z_Msk                      (1UL << SCTLR_Z_Pos)                   /*!< \brief SCTLR: Z Mask */

#define SCTLR_SW_Pos                     10U                                    /*!< \brief SCTLR: SW Position */
#define SCTLR_SW_Msk                     (1UL << SCTLR_SW_Pos)                  /*!< \brief SCTLR: SW Mask */

#define SCTLR_SED_Pos                    8U                                     /*!< \brief SCTLR: SED Position */
#define SCTLR_SED_Msk                    (1UL << SCTLR_SED_Pos)                 /*!< \brief SCTLR: SED Mask */

#define SCTLR_ITD_Pos                    7U                                     /*!< \brief SCTLR: ITD Position */
#define SCTLR_ITD_Msk                    (1UL << SCTLR_ITD_Pos)                 /*!< \brief SCTLR: ITD Mask */

#define SCTLR_CP15BEN_Pos                5U                                     /*!< \brief SCTLR: CP15BEN Position */
#define SCTLR_CP15BEN_Msk                (1UL << SCTLR_CP15BEN_Pos)             /*!< \brief SCTLR: CP15BEN Mask */

#define SCTLR_C_Pos                      2U                                     /*!< \brief SCTLR: C Position */
#define SCTLR_C_Msk                      (1UL << SCTLR_C_Pos)                   /*!< \brief SCTLR: C Mask */

#define SCTLR_A_Pos                      1U                                     /*!< \brief SCTLR: A Position */
#define SCTLR_A_Msk                      (1UL << SCTLR_A_Pos)                   /*!< \brief SCTLR: A Mask */

#define SCTLR_M_Pos                      0U                                     /*!< \brief SCTLR: M Position */
#define SCTLR_M_Msk                      (1UL << SCTLR_M_Pos)                   /*!< \brief SCTLR: M Mask */

/* CP15 Register ACTLR */
typedef union
{
#if __CORTEX_R == 4 || __CORTEX_R == 5 || defined(DOXYGEN)
  struct
  {
    uint32_t ATCMECEN:1;                 /*!< \brief bit:     0  ATCM external error enable */
    uint32_t B0TCMECEN:1;                /*!< \brief bit:     1  B0TCM external error enable */
    uint32_t B1TCMECEN:1;                /*!< \brief bit:     2  B1TCM external error enable */
    uint32_t CEC:3;                      /*!< \brief bit: 3.. 5  Cache error control */
    uint32_t DILS:1;                     /*!< \brief bit:     6  Low interrupt latency disable */
    uint32_t SMOV:1;                     /*!< \brief bit:     7  SMOV out-of-order disable */
    uint32_t FDSNS:1;                    /*!< \brief bit:     8  Force D-side not shared when MPU is off */
    uint32_t FWT:1;                      /*!< \brief bit:     9  Force write-through for write-back regions */
    uint32_t FORA:1;                     /*!< \brief bit:    10  Force out-read-allocate for out-write-allocate regions */
    uint32_t DNCH:1;                     /*!< \brief bit:    11  Data forwarding for non-cachable access disable */
    uint32_t ERPEG:1;                    /*!< \brief bit:    12  Random parity error generation enable */
    uint32_t DLFO:1;                     /*!< \brief bit:    13  Linefill optimization disable */
    uint32_t DBWR:1;                     /*!< \brief bit:    14  Write burst disable */
    uint32_t BP:2;                       /*!< \brief bit:15..16  Branch prediction policy */
    uint32_t RSDIS:1;                    /*!< \brief bit:    17  Return stack disable */
    RESERVED(0:1, uint32_t)
    uint32_t FRCDIS:1;                   /*!< \brief bit:    19  Fetch rate control disable */
    uint32_t DBHE:1;                     /*!< \brief bit:    20  Branch history extension disable */
    uint32_t DEOLP:1;                    /*!< \brief bit:    21  End-of-loop prediction disable */
    uint32_t DILSM:1;                    /*!< \brief bit:    22  Low interrupt latency on load/store multiples disable */
    uint32_t AXISCUEN:1;                 /*!< \brief bit:    23  AXI slave cache RAM non-privileged access enable */
    uint32_t AXISCEN:1;                  /*!< \brief bit:    24  AXI slave cache RAM access enable */
    uint32_t ATCMPCEN:1;                 /*!< \brief bit:    25  ATCM ECC check enable */
    uint32_t B0TCMPCEN:1;                /*!< \brief bit:    26  B0TCM ECC check enable */
    uint32_t B1TCMPCEN:1;                /*!< \brief bit:    27  B1TCM ECC check enable */
    uint32_t DIADI:1;                    /*!< \brief bit:    28  Case A dual issue disable */
    uint32_t DIB1DI:1;                   /*!< \brief bit:    29  Case B1 dual issue disable */
    uint32_t DIB2DI:1;                   /*!< \brief bit:    30  Case B2 dual issue disable */
    uint32_t DICDI:1;                    /*!< \brief bit:    31  Case C dual issue disable */
 } b;
#endif
#if __CORTEX_R == 7 || __CORTEX_R == 8 || defined(DOXYGEN)
  struct
  {
    uint32_t FW:1;                       /*!< \brief bit:     0  Cache and TLB maintenance broadcast */
    RESERVED(0:2, uint32_t)
    uint32_t MRPEN:1;                    /*!< \brief bit:     3  MRP enable */
    RESERVED(1:2, uint32_t)
    uint32_t SMP:1;                      /*!< \brief bit:     6  Processor coherency enable */
    RESERVED(2:1, uint32_t)
    uint32_t AOW:1;                      /*!< \brief bit:     8  One way cache allocation enable */
    uint32_t DTCMECEN:1;                 /*!< \brief bit:     9  Cache and DTCM ECC support */
    uint32_t ITCMECEN:1;                 /*!< \brief bit:    10  ITCM ECC support */
    uint32_t QOSEN:1;                    /*!< \brief bit:    11  Quality-of-service enable */
    RESERVED(3:20, uint32_t)
  } b;
#endif
#if __CORTEX_R == 52 || defined(DOXYGEN)
  struct
  {
    RESERVED(0:32, uint32_t)
  } b;
#endif
  uint32_t w;                            /*!< \brief Type      used for word access */
} ACTLR_Type;

#define ACTLR_DICDI_Pos                  31U                                     /*!< \brief ACTLR: DICDI Position */
#define ACTLR_DICDI_Msk                  (1UL << ACTLR_DICDI_Pos)                /*!< \brief ACTLR: DICDI Mask */

#define ACTLR_DIB2DI_Pos                 30U                                     /*!< \brief ACTLR: DIB2DI Position */
#define ACTLR_DIB2DI_Msk                 (1UL << ACTLR_DIB2DI_Pos)               /*!< \brief ACTLR: DIB2DI Mask */

#define ACTLR_DIB1DI_Pos                 29U                                     /*!< \brief ACTLR: DIB1DI Position */
#define ACTLR_DIB1DI_Msk                 (1UL << ACTLR_DIB1DI_Pos)               /*!< \brief ACTLR: DIB1DI Mask */

#define ACTLR_DIADI_Pos                  28U                                     /*!< \brief ACTLR: DIADI Position */
#define ACTLR_DIADI_Msk                  (1UL << ACTLR_DIADI_Pos)                /*!< \brief ACTLR: DIADI Mask */

#define ACTLR_B1TCMPCEN_Pos              27U                                     /*!< \brief ACTLR: B1TCMPCEN Position */
#define ACTLR_B1TCMPCEN_Msk              (1UL << ACTLR_B1TCMPCEN_Pos)            /*!< \brief ACTLR: B1TCMPCEN Mask */

#define ACTLR_B0TCMPCEN_Pos              26U                                     /*!< \brief ACTLR: B0TCMPCEN Position */
#define ACTLR_B0TCMPCEN_Msk              (1UL << ACTLR_B0TCMPCEN_Pos)            /*!< \brief ACTLR: B0TCMPCEN Mask */

#define ACTLR_ATCMPCEN_Pos               25U                                     /*!< \brief ACTLR: ATCMPCEN Position */
#define ACTLR_ATCMPCEN_Msk               (1UL << ACTLR_ATCMPCEN_Pos)             /*!< \brief ACTLR: ATCMPCEN Mask */

#define ACTLR_AXISCEN_Pos                24U                                     /*!< \brief ACTLR: AXISCEN Position */
#define ACTLR_AXISCEN_Msk                (1UL << ACTLR_AXISCEN_Pos)              /*!< \brief ACTLR: AXISCEN Mask */

#define ACTLR_AXISCUEN_Pos               23U                                     /*!< \brief ACTLR: AXISCUEN Position */
#define ACTLR_AXISCUEN_Msk               (1UL << ACTLR_AXISCUEN_Pos)             /*!< \brief ACTLR: AXISCUEN Mask */

#define ACTLR_DILSM_Pos                  22U                                     /*!< \brief ACTLR: DILSM Position */
#define ACTLR_DILSM_Msk                  (1UL << ACTLR_DILSM_Pos)                /*!< \brief ACTLR: DILSM Mask */

#define ACTLR_DEOLP_Pos                  21U                                     /*!< \brief ACTLR: DEOLP Position */
#define ACTLR_DEOLP_Msk                  (1UL << ACTLR_DEOLP_Pos)                /*!< \brief ACTLR: DEOLP Mask */

#define ACTLR_DBHE_Pos                   20U                                     /*!< \brief ACTLR: DBHE Position */
#define ACTLR_DBHE_Msk                   (1UL << ACTLR_DBHE_Pos)                 /*!< \brief ACTLR: DBHE Mask */

#define ACTLR_FRCDIS_Pos                 19U                                     /*!< \brief ACTLR: FRCDIS Position */
#define ACTLR_FRCDIS_Msk                 (1UL << ACTLR_FRCDIS_Pos)               /*!< \brief ACTLR: FRCDIS Mask */

#define ACTLR_RSDIS_Pos                  17U                                     /*!< \brief ACTLR: RSDIS Position */
#define ACTLR_RSDIS_Msk                  (1UL << ACTLR_RSDIS_Pos)                /*!< \brief ACTLR: RSDIS Mask */

#define ACTLR_BP_Pos                     15U                                     /*!< \brief ACTLR: BP Position */
#define ACTLR_BP_Msk                     (3UL << ACTLR_BP_Pos)                   /*!< \brief ACTLR: BP Mask */

#define ACTLR_DBWR_Pos                   14U                                     /*!< \brief ACTLR: DBWR Position */
#define ACTLR_DBWR_Msk                   (1UL << ACTLR_DBWR_Pos)                 /*!< \brief ACTLR: DBWR Mask */

#define ACTLR_DLFO_Pos                   13U                                     /*!< \brief ACTLR: DLFO Position */
#define ACTLR_DLFO_Msk                   (1UL << ACTLR_DLFO_Pos)                 /*!< \brief ACTLR: DLFO Mask */

#define ACTLR_ERPEG_Pos                  12U                                     /*!< \brief ACTLR: ERPEG Position */
#define ACTLR_ERPEG_Msk                  (1UL << ACTLR_ERPEG_Pos)                /*!< \brief ACTLR: ERPEG Mask */

#define ACTLR_DNCH_Pos                   11U                                     /*!< \brief ACTLR: DNCH Position */
#define ACTLR_DNCH_Msk                   (1UL << ACTLR_DNCH_Pos)                 /*!< \brief ACTLR: DNCH Mask */

#define ACTLR_QOSEN_Pos                  11U                                     /*!< \brief ACTLR: QOSEN Position */
#define ACTLR_QOSEN_Msk                  (1UL << ACTLR_QOSEN_Pos)                /*!< \brief ACTLR: QOSEN Mask */

#define ACTLR_FORA_Pos                   10U                                     /*!< \brief ACTLR: FORA Position */
#define ACTLR_FORA_Msk                   (1UL << ACTLR_FORA_Pos)                 /*!< \brief ACTLR: FORA Mask */

#define ACTLR_ITCMECEN_Pos               10U                                     /*!< \brief ACTLR: ITCMECEN Position */
#define ACTLR_ITCMECEN_Msk               (1UL << ACTLR_ITCMECEN_Pos)             /*!< \brief ACTLR: ITCMECEN Mask */

#define ACTLR_FWT_Pos                    9U                                      /*!< \brief ACTLR: FWT Position */
#define ACTLR_FWT_Msk                    (1UL << ACTLR_FWT_Pos)                  /*!< \brief ACTLR: FWT Mask */

#define ACTLR_DTCMECEN_Pos               9U                                      /*!< \brief ACTLR: DTCMECEN Position */
#define ACTLR_DTCMECEN_Msk               (1UL << ACTLR_DTCMECEN_Pos)             /*!< \brief ACTLR: DTCMECEN Mask */

#define ACTLR_FDSNS_Pos                  8U                                      /*!< \brief ACTLR: FDSNS Position */
#define ACTLR_FDSNS_Msk                  (1UL << ACTLR_FDSNS_Pos)                /*!< \brief ACTLR: FDSNS Mask */

#define ACTLR_AOW_Pos                    8U                                      /*!< \brief ACTLR: AOW Position */
#define ACTLR_AOW_Msk                    (1UL << ACTLR_AOW_Pos)                  /*!< \brief ACTLR: AOW Mask */

#define ACTLR_SMOV_Pos                   7U                                      /*!< \brief ACTLR: SMOV Position */
#define ACTLR_SMOV_Msk                   (1UL << ACTLR_SMOV_Pos)                 /*!< \brief ACTLR: SMOV Mask */

#define ACTLR_DILS_Pos                   6U                                      /*!< \brief ACTLR: DILS Position */
#define ACTLR_DILS_Msk                   (1UL << ACTLR_DILS_Pos)                 /*!< \brief ACTLR: DILS Mask */

#define ACTLR_SMP_Pos                    6U                                      /*!< \brief ACTLR: SMP Position */
#define ACTLR_SMP_Msk                    (1UL << ACTLR_SMP_Pos)                  /*!< \brief ACTLR: SMP Mask */

#define ACTLR_CEC_Pos                    3U                                      /*!< \brief ACTLR: CEC Position */
#define ACTLR_CEC_Msk                    (7UL << ACTLR_CEC_Pos)                  /*!< \brief ACTLR: CEC Mask */

#define ACTLR_MRPEN_Pos                  3U                                      /*!< \brief ACTLR: MRPEN Position */
#define ACTLR_MRPEN_Msk                  (1UL << ACTLR_MRPEN_Pos)                /*!< \brief ACTLR: MRPEN Mask */

#define ACTLR_B1TCMECEN_Pos              2U                                      /*!< \brief ACTLR: B1TCMECEN Position */
#define ACTLR_B1TCMECEN_Msk              (1UL << ACTLR_B1TCMECEN_Pos)            /*!< \brief ACTLR: B1TCMECEN Mask */

#define ACTLR_B0TCMECEN_Pos              1U                                      /*!< \brief ACTLR: B0TCMECEN Position */
#define ACTLR_B0TCMECEN_Msk              (1UL << ACTLR_B0TCMECEN_Pos)            /*!< \brief ACTLR: B0TCMECEN Mask */

#define ACTLR_ATCMECEN_Pos               0U                                      /*!< \brief ACTLR: ATCMECEN Position */
#define ACTLR_ATCMECEN_Msk               (1UL << ACTLR_ATCMECEN_Pos)             /*!< \brief ACTLR: ATCMECEN Mask */

#define ACTLR_FW_Pos                     0U                                      /*!< \brief ACTLR: FW Position */
#define ACTLR_FW_Msk                     (1UL << ACTLR_FW_Pos)                   /*!< \brief ACTLR: FW Mask */

/* CP15 Register CPACR */
typedef union
{
  struct
  {
    uint32_t CP0:2;                      /*!< \brief bit:  0..1  Access rights for coprocessor 0 */
    uint32_t CP1:2;                      /*!< \brief bit:  2..3  Access rights for coprocessor 1 */
    uint32_t CP2:2;                      /*!< \brief bit:  4..5  Access rights for coprocessor 2 */
    uint32_t CP3:2;                      /*!< \brief bit:  6..7  Access rights for coprocessor 3 */
    uint32_t CP4:2;                      /*!< \brief bit:  8..9  Access rights for coprocessor 4 */
    uint32_t CP5:2;                      /*!< \brief bit:10..11  Access rights for coprocessor 5 */
    uint32_t CP6:2;                      /*!< \brief bit:12..13  Access rights for coprocessor 6 */
    uint32_t CP7:2;                      /*!< \brief bit:14..15  Access rights for coprocessor 7 */
    uint32_t CP8:2;                      /*!< \brief bit:16..17  Access rights for coprocessor 8 */
    uint32_t CP9:2;                      /*!< \brief bit:18..19  Access rights for coprocessor 9 */
    uint32_t CP10:2;                     /*!< \brief bit:20..21  Access rights for coprocessor 10 */
    uint32_t CP11:2;                     /*!< \brief bit:22..23  Access rights for coprocessor 11 */
    uint32_t CP12:2;                     /*!< \brief bit:24..25  Access rights for coprocessor 11 */
    uint32_t CP13:2;                     /*!< \brief bit:26..27  Access rights for coprocessor 11 */
    uint32_t TRCDIS:1;                   /*!< \brief bit:    28  Disable CP14 access to trace registers */
    RESERVED(0:1, uint32_t)
    uint32_t D32DIS:1;                   /*!< \brief bit:    30  Disable use of registers D16-D31 of the VFP register file */
    uint32_t ASEDIS:1;                   /*!< \brief bit:    31  Disable Advanced SIMD Functionality */
  } b;                                   /*!< \brief Structure used for bit  access */
  uint32_t w;                            /*!< \brief Type      used for word access */
} CPACR_Type;

#define CPACR_ASEDIS_Pos                 31U                                    /*!< \brief CPACR: ASEDIS Position */
#define CPACR_ASEDIS_Msk                 (1UL << CPACR_ASEDIS_Pos)              /*!< \brief CPACR: ASEDIS Mask */

#define CPACR_D32DIS_Pos                 30U                                    /*!< \brief CPACR: D32DIS Position */
#define CPACR_D32DIS_Msk                 (1UL << CPACR_D32DIS_Pos)              /*!< \brief CPACR: D32DIS Mask */

#define CPACR_TRCDIS_Pos                 28U                                    /*!< \brief CPACR: D32DIS Position */
#define CPACR_TRCDIS_Msk                 (1UL << CPACR_D32DIS_Pos)              /*!< \brief CPACR: D32DIS Mask */

#define CPACR_CP_Pos_(n)                 (n*2U)                                 /*!< \brief CPACR: CPn Position */
#define CPACR_CP_Msk_(n)                 (3UL << CPACR_CP_Pos_(n))              /*!< \brief CPACR: CPn Mask */

#define CPACR_CP_NA                      0U                                     /*!< \brief CPACR CPn field: Access denied. */
#define CPACR_CP_PL1                     1U                                     /*!< \brief CPACR CPn field: Accessible from PL1 only. */
#define CPACR_CP_FA                      3U                                     /*!< \brief CPACR CPn field: Full access. */

/* CP15 Register DFSR */
typedef union
{
  struct
  {
    uint32_t FS0:4;                      /*!< \brief bit: 0.. 3  Fault Status bits bit 0-3 */
    uint32_t Domain:4;                   /*!< \brief bit: 4.. 7  Fault on which domain */
    RESERVED(0:1, uint32_t)
    uint32_t LPAE:1;                     /*!< \brief bit:     9  Large Physical Address Extension */
    uint32_t FS1:1;                      /*!< \brief bit:    10  Fault Status bits bit 4 */
    uint32_t WnR:1;                      /*!< \brief bit:    11  Write not Read bit */
    uint32_t ExT:1;                      /*!< \brief bit:    12  External abort type */
    uint32_t CM:1;                       /*!< \brief bit:    13  Cache maintenance fault */
    RESERVED(1:2, uint32_t)
    uint32_t FnV:1;                      /*!< \brief bit:    16  FAR not valid */
    RESERVED(2:15, uint32_t)
  } s;                                   /*!< \brief Structure used for bit  access in short format */
  struct
  {
    uint32_t STATUS:5;                   /*!< \brief bit: 0.. 5  Fault Status bits */
    RESERVED(0:3, uint32_t)
    uint32_t LPAE:1;                     /*!< \brief bit:     9  Large Physical Address Extension */
    RESERVED(1:1, uint32_t)
    uint32_t WnR:1;                      /*!< \brief bit:    11  Write not Read bit */
    uint32_t ExT:1;                      /*!< \brief bit:    12  External abort type */
    uint32_t CM:1;                       /*!< \brief bit:    13  Cache maintenance fault */
    RESERVED(2:2, uint32_t)
    uint32_t FnV:1;                      /*!< \brief bit:    16  FAR not valid */
    RESERVED(3:15, uint32_t)
  } l;                                   /*!< \brief Structure used for bit  access in long format */
  uint32_t w;                            /*!< \brief Type      used for word access */
} DFSR_Type;

#define DFSR_FnV_Pos                     16U                                    /*!< \brief DFSR: FnV Position */
#define DFSR_FnV_Msk                     (1UL << DFSR_FnV_Pos)                  /*!< \brief DFSR: FnV Mask */

#define DFSR_CM_Pos                      13U                                    /*!< \brief DFSR: CM Position */
#define DFSR_CM_Msk                      (1UL << DFSR_CM_Pos)                   /*!< \brief DFSR: CM Mask */

#define DFSR_Ext_Pos                     12U                                    /*!< \brief DFSR: Ext Position */
#define DFSR_Ext_Msk                     (1UL << DFSR_Ext_Pos)                  /*!< \brief DFSR: Ext Mask */

#define DFSR_WnR_Pos                     11U                                    /*!< \brief DFSR: WnR Position */
#define DFSR_WnR_Msk                     (1UL << DFSR_WnR_Pos)                  /*!< \brief DFSR: WnR Mask */

#define DFSR_FS1_Pos                     10U                                    /*!< \brief DFSR: FS1 Position */
#define DFSR_FS1_Msk                     (1UL << DFSR_FS1_Pos)                  /*!< \brief DFSR: FS1 Mask */

#define DFSR_LPAE_Pos                    9U                                    /*!< \brief DFSR: LPAE Position */
#define DFSR_LPAE_Msk                    (1UL << DFSR_LPAE_Pos)                /*!< \brief DFSR: LPAE Mask */

#define DFSR_Domain_Pos                  4U                                     /*!< \brief DFSR: Domain Position */
#define DFSR_Domain_Msk                  (0xFUL << DFSR_Domain_Pos)             /*!< \brief DFSR: Domain Mask */

#define DFSR_FS0_Pos                     0U                                     /*!< \brief DFSR: FS0 Position */
#define DFSR_FS0_Msk                     (0xFUL << DFSR_FS0_Pos)                /*!< \brief DFSR: FS0 Mask */

#define DFSR_STATUS_Pos                  0U                                     /*!< \brief DFSR: STATUS Position */
#define DFSR_STATUS_Msk                  (0x3FUL << DFSR_STATUS_Pos)            /*!< \brief DFSR: STATUS Mask */

/* CP15 Register IFSR */
typedef union
{
  struct
  {
    uint32_t FS0:4;                      /*!< \brief bit: 0.. 3  Fault Status bits bit 0-3 */
    RESERVED(0:5, uint32_t)
    uint32_t LPAE:1;                     /*!< \brief bit:     9  Large Physical Address Extension */
    uint32_t FS1:1;                      /*!< \brief bit:    10  Fault Status bits bit 4 */
    RESERVED(1:1, uint32_t)
    uint32_t ExT:1;                      /*!< \brief bit:    12  External abort type */
    RESERVED(2:19, uint32_t)
  } s;                                   /*!< \brief Structure used for bit access in short format */
  struct
  {
    uint32_t STATUS:6;                   /*!< \brief bit: 0.. 5  Fault Status bits */
    RESERVED(0:3, uint32_t)
    uint32_t LPAE:1;                     /*!< \brief bit:     9  Large Physical Address Extension */
    RESERVED(1:2, uint32_t)
    uint32_t ExT:1;                      /*!< \brief bit:    12  External abort type */
    RESERVED(2:3, uint32_t)
    uint32_t FnV:1;                      /*!< \brief bit:    16  FAR not valid */
    RESERVED(3:15, uint32_t)
  } l;                                   /*!< \brief Structure used for bit access in long format */
  uint32_t w;                            /*!< \brief Type      used for word access */
} IFSR_Type;

#define IFSR_FnV_Pos                     16U                                    /*!< \brief IFSR: FnV Position */
#define IFSR_FnV_Msk                     (1UL << IFSR_FnV_Pos)                  /*!< \brief IFSR: FnV Mask */

#define IFSR_ExT_Pos                     12U                                    /*!< \brief IFSR: ExT Position */
#define IFSR_ExT_Msk                     (1UL << IFSR_ExT_Pos)                  /*!< \brief IFSR: ExT Mask */

#define IFSR_FS1_Pos                     10U                                    /*!< \brief IFSR: FS1 Position */
#define IFSR_FS1_Msk                     (1UL << IFSR_FS1_Pos)                  /*!< \brief IFSR: FS1 Mask */

#define IFSR_LPAE_Pos                    9U                                     /*!< \brief IFSR: LPAE Position */
#define IFSR_LPAE_Msk                    (0x1UL << IFSR_LPAE_Pos)               /*!< \brief IFSR: LPAE Mask */

#define IFSR_FS0_Pos                     0U                                     /*!< \brief IFSR: FS0 Position */
#define IFSR_FS0_Msk                     (0xFUL << IFSR_FS0_Pos)                /*!< \brief IFSR: FS0 Mask */

#define IFSR_STATUS_Pos                  0U                                     /*!< \brief IFSR: STATUS Position */
#define IFSR_STATUS_Msk                  (0x3FUL << IFSR_STATUS_Pos)            /*!< \brief IFSR: STATUS Mask */

/* CP15 Register ISR */
typedef union
{
  struct
  {
    RESERVED(0:6, uint32_t)
    uint32_t F:1;                        /*!< \brief bit:     6  FIQ pending bit */
    uint32_t I:1;                        /*!< \brief bit:     7  IRQ pending bit */
    uint32_t A:1;                        /*!< \brief bit:     8  External abort pending bit */
    RESERVED(1:23, uint32_t)
  } b;                                   /*!< \brief Structure used for bit  access */
  uint32_t w;                            /*!< \brief Type      used for word access */
} ISR_Type;

#define ISR_A_Pos                        13U                                    /*!< \brief ISR: A Position */
#define ISR_A_Msk                        (1UL << ISR_A_Pos)                     /*!< \brief ISR: A Mask */

#define ISR_I_Pos                        12U                                    /*!< \brief ISR: I Position */
#define ISR_I_Msk                        (1UL << ISR_I_Pos)                     /*!< \brief ISR: I Mask */

#define ISR_F_Pos                        11U                                    /*!< \brief ISR: F Position */
#define ISR_F_Msk                        (1UL << ISR_F_Pos)                     /*!< \brief ISR: F Mask */

/* DACR Register */
#define DACR_D_Pos_(n)                   (2U*n)                                 /*!< \brief DACR: Dn Position */
#define DACR_D_Msk_(n)                   (3UL << DACR_D_Pos_(n))                /*!< \brief DACR: Dn Mask */
#define DACR_Dn_NOACCESS                 0U                                     /*!< \brief DACR Dn field: No access */
#define DACR_Dn_CLIENT                   1U                                     /*!< \brief DACR Dn field: Client */
#define DACR_Dn_MANAGER                  3U                                     /*!< \brief DACR Dn field: Manager */

/**
  \brief     Mask and shift a bit field value for use in a register bit range.
  \param [in] field  Name of the register bit field.
  \param [in] value  Value of the bit field. This parameter is interpreted as an uint32_t type.
  \return           Masked and shifted value.
*/
#define _VAL2FLD(field, value)    (((uint32_t)(value) << field ## _Pos) & field ## _Msk)

/**
  \brief     Mask and shift a register value to extract a bit filed value.
  \param [in] field  Name of the register bit field.
  \param [in] value  Value of register. This parameter is interpreted as an uint32_t type.
  \return           Masked and shifted bit field value.
*/
#define _FLD2VAL(field, value)    (((uint32_t)(value) & field ## _Msk) >> field ## _Pos)

#if (__GIC_PRESENT == 1U) || defined(DOXYGEN)

/** \brief  Structure type to access the Generic Interrupt Controller Distributor (GICD)
*/
typedef struct
{
  __IOM uint32_t CTLR;                 /*!< \brief  Offset: 0x000 (R/W) Distributor Control Register */
  __IM  uint32_t TYPER;                /*!< \brief  Offset: 0x004 (R/ ) Interrupt Controller Type Register */
  __IM  uint32_t IIDR;                 /*!< \brief  Offset: 0x008 (R/ ) Distributor Implementer Identification Register */
        RESERVED(0, uint32_t)
  __IOM uint32_t STATUSR;              /*!< \brief  Offset: 0x010 (R/W) Error Reporting Status Register, optional */
        RESERVED(1[11], uint32_t)
  __OM  uint32_t SETSPI_NSR;           /*!< \brief  Offset: 0x040 ( /W) Set SPI Register */
        RESERVED(2, uint32_t)
  __OM  uint32_t CLRSPI_NSR;           /*!< \brief  Offset: 0x048 ( /W) Clear SPI Register */
        RESERVED(3, uint32_t)
  __OM  uint32_t SETSPI_SR;            /*!< \brief  Offset: 0x050 ( /W) Set SPI, Secure Register */
        RESERVED(4, uint32_t)
  __OM  uint32_t CLRSPI_SR;            /*!< \brief  Offset: 0x058 ( /W) Clear SPI, Secure Register */
        RESERVED(5[9], uint32_t)
  __IOM uint32_t IGROUPR[32];          /*!< \brief  Offset: 0x080 (R/W) Interrupt Group Registers */
  __IOM uint32_t ISENABLER[32];        /*!< \brief  Offset: 0x100 (R/W) Interrupt Set-Enable Registers */
  __IOM uint32_t ICENABLER[32];        /*!< \brief  Offset: 0x180 (R/W) Interrupt Clear-Enable Registers */
  __IOM uint32_t ISPENDR[32];          /*!< \brief  Offset: 0x200 (R/W) Interrupt Set-Pending Registers */
  __IOM uint32_t ICPENDR[32];          /*!< \brief  Offset: 0x280 (R/W) Interrupt Clear-Pending Registers */
  __IOM uint32_t ISACTIVER[32];        /*!< \brief  Offset: 0x300 (R/W) Interrupt Set-Active Registers */
  __IOM uint32_t ICACTIVER[32];        /*!< \brief  Offset: 0x380 (R/W) Interrupt Clear-Active Registers */
  __IOM uint32_t IPRIORITYR[255];      /*!< \brief  Offset: 0x400 (R/W) Interrupt Priority Registers */
        RESERVED(6, uint32_t)
  __IOM uint32_t  ITARGETSR[255];      /*!< \brief  Offset: 0x800 (R/W) Interrupt Targets Registers */
        RESERVED(7, uint32_t)
  __IOM uint32_t ICFGR[64];            /*!< \brief  Offset: 0xC00 (R/W) Interrupt Configuration Registers */
  __IOM uint32_t IGRPMODR[32];         /*!< \brief  Offset: 0xD00 (R/W) Interrupt Group Modifier Registers */
        RESERVED(8[32], uint32_t)
  __IOM uint32_t NSACR[64];            /*!< \brief  Offset: 0xE00 (R/W) Non-secure Access Control Registers */
  __OM  uint32_t SGIR;                 /*!< \brief  Offset: 0xF00 ( /W) Software Generated Interrupt Register */
        RESERVED(9[3], uint32_t)
  __IOM uint32_t CPENDSGIR[4];         /*!< \brief  Offset: 0xF10 (R/W) SGI Clear-Pending Registers */
  __IOM uint32_t SPENDSGIR[4];         /*!< \brief  Offset: 0xF20 (R/W) SGI Set-Pending Registers */
        RESERVED(10[5236], uint32_t)
  __IOM uint64_t IROUTER[988];         /*!< \brief  Offset: 0x6100(R/W) Interrupt Routing Registers */
}  GICDistributor_Type;

#define GICDistributor      ((GICDistributor_Type      *)     GIC_DISTRIBUTOR_BASE ) /*!< \brief GIC Distributor register set access pointer */

/** \brief  Structure type to access the Generic Interrupt Controller Interface (GICC)
*/
typedef struct
{
  __IOM uint32_t CTLR;                 /*!< \brief  Offset: 0x000 (R/W) CPU Interface Control Register */
  __IOM uint32_t PMR;                  /*!< \brief  Offset: 0x004 (R/W) Interrupt Priority Mask Register */
  __IOM uint32_t BPR;                  /*!< \brief  Offset: 0x008 (R/W) Binary Point Register */
  __IM  uint32_t IAR;                  /*!< \brief  Offset: 0x00C (R/ ) Interrupt Acknowledge Register */
  __OM  uint32_t EOIR;                 /*!< \brief  Offset: 0x010 ( /W) End Of Interrupt Register */
  __IM  uint32_t RPR;                  /*!< \brief  Offset: 0x014 (R/ ) Running Priority Register */
  __IM  uint32_t HPPIR;                /*!< \brief  Offset: 0x018 (R/ ) Highest Priority Pending Interrupt Register */
  __IOM uint32_t ABPR;                 /*!< \brief  Offset: 0x01C (R/W) Aliased Binary Point Register */
  __IM  uint32_t AIAR;                 /*!< \brief  Offset: 0x020 (R/ ) Aliased Interrupt Acknowledge Register */
  __OM  uint32_t AEOIR;                /*!< \brief  Offset: 0x024 ( /W) Aliased End Of Interrupt Register */
  __IM  uint32_t AHPPIR;               /*!< \brief  Offset: 0x028 (R/ ) Aliased Highest Priority Pending Interrupt Register */
  __IOM uint32_t STATUSR;              /*!< \brief  Offset: 0x02C (R/W) Error Reporting Status Register, optional */
        RESERVED(1[40], uint32_t)
  __IOM uint32_t APR[4];               /*!< \brief  Offset: 0x0D0 (R/W) Active Priority Register */
  __IOM uint32_t NSAPR[4];             /*!< \brief  Offset: 0x0E0 (R/W) Non-secure Active Priority Register */
        RESERVED(2[3], uint32_t)
  __IM  uint32_t IIDR;                 /*!< \brief  Offset: 0x0FC (R/ ) CPU Interface Identification Register */
        RESERVED(3[960], uint32_t)
  __OM  uint32_t DIR;                  /*!< \brief  Offset: 0x1000( /W) Deactivate Interrupt Register */
}  GICInterface_Type;

#define GICInterface        ((GICInterface_Type        *)     GIC_INTERFACE_BASE )   /*!< \brief GIC Interface register set access pointer */
#endif

#if (__TIM_PRESENT == 1U) || defined(DOXYGEN)
#if ((__CORTEX_R == 7U) || (__CORTEX_R == 8U)) || defined(DOXYGEN)
/** \brief Structure type to access the Private Timer
*/
typedef struct
{
  __IOM uint32_t LOAD;            //!< \brief  Offset: 0x000 (R/W) Private Timer Load Register
  __IOM uint32_t COUNTER;         //!< \brief  Offset: 0x004 (R/W) Private Timer Counter Register
  __IOM uint32_t CONTROL;         //!< \brief  Offset: 0x008 (R/W) Private Timer Control Register
  __IOM uint32_t ISR;             //!< \brief  Offset: 0x00C (R/W) Private Timer Interrupt Status Register
        RESERVED(0[4], uint32_t)
  __IOM uint32_t WLOAD;           //!< \brief  Offset: 0x020 (R/W) Watchdog Load Register
  __IOM uint32_t WCOUNTER;        //!< \brief  Offset: 0x024 (R/W) Watchdog Counter Register
  __IOM uint32_t WCONTROL;        //!< \brief  Offset: 0x028 (R/W) Watchdog Control Register
  __IOM uint32_t WISR;            //!< \brief  Offset: 0x02C (R/W) Watchdog Interrupt Status Register
  __IOM uint32_t WRESET;          //!< \brief  Offset: 0x030 (R/W) Watchdog Reset Status Register
  __OM  uint32_t WDISABLE;        //!< \brief  Offset: 0x034 ( /W) Watchdog Disable Register
} Timer_Type;
#define PTIM ((Timer_Type *) TIMER_BASE )   /*!< \brief Timer register struct */
#endif
#endif

 /*******************************************************************************
  *                Hardware Abstraction Layer
   Core Function Interface contains:
   - L1 Cache Functions
   - PL1 Timer Functions
   - GIC Functions
   - MMU Functions
  ******************************************************************************/

/* ##########################  L1 Cache functions  ################################# */

/** \brief Enable Caches by setting I and C bits in SCTLR register.
*/
__STATIC_FORCEINLINE void L1C_EnableCaches(void) {
  __set_SCTLR( __get_SCTLR() | SCTLR_I_Msk | SCTLR_C_Msk);
  __ISB();
}

/** \brief Disable Caches by clearing I and C bits in SCTLR register.
*/
__STATIC_FORCEINLINE void L1C_DisableCaches(void) {
  __set_SCTLR( __get_SCTLR() & (~SCTLR_I_Msk) & (~SCTLR_C_Msk));
  __ISB();
}

/** \brief  Enable Branch Prediction by setting Z bit in SCTLR register.
*/
__STATIC_FORCEINLINE void L1C_EnableBTAC(void) {
  __set_SCTLR( __get_SCTLR() | SCTLR_Z_Msk);
  __ISB();
}

/** \brief  Disable Branch Prediction by clearing Z bit in SCTLR register.
*/
__STATIC_FORCEINLINE void L1C_DisableBTAC(void) {
  __set_SCTLR( __get_SCTLR() & (~SCTLR_Z_Msk));
  __ISB();
}

/** \brief  Invalidate entire branch predictor array
*/
__STATIC_FORCEINLINE void L1C_InvalidateBTAC(void) {
  __set_BPIALL(0);
  __DSB();     //ensure completion of the invalidation
  __ISB();     //ensure instruction fetch path sees new state
}

/** \brief  Invalidate the whole instruction cache
*/
__STATIC_FORCEINLINE void L1C_InvalidateICacheAll(void) {
  __set_ICIALLU(0);
  __DSB();     //ensure completion of the invalidation
  __ISB();     //ensure instruction fetch path sees new I cache state
}

/** \brief  Clean data cache line by address.
* \param [in] va Pointer to data to clear the cache for.
*/
__STATIC_FORCEINLINE void L1C_CleanDCacheMVA(void *va) {
  __set_DCCMVAC((uint32_t)va);
  __DMB();     //ensure the ordering of data cache maintenance operations and their effects
}

/** \brief  Invalidate data cache line by address.
* \param [in] va Pointer to data to invalidate the cache for.
*/
__STATIC_FORCEINLINE void L1C_InvalidateDCacheMVA(void *va) {
  __set_DCIMVAC((uint32_t)va);
  __DMB();     //ensure the ordering of data cache maintenance operations and their effects
}

/** \brief  Clean and Invalidate data cache by address.
* \param [in] va Pointer to data to invalidate the cache for.
*/
__STATIC_FORCEINLINE void L1C_CleanInvalidateDCacheMVA(void *va) {
  __set_DCCIMVAC((uint32_t)va);
  __DMB();     //ensure the ordering of data cache maintenance operations and their effects
}

/** \brief Calculate log2 rounded up
*  - log(0)  => 0
*  - log(1)  => 0
*  - log(2)  => 1
*  - log(3)  => 2
*  - log(4)  => 2
*  - log(5)  => 3
*        :      :
*  - log(16) => 4
*  - log(32) => 5
*        :      :
* \param [in] n input value parameter
* \return log2(n)
*/
__STATIC_FORCEINLINE uint8_t __log2_up(uint32_t n)
{
  if (n < 2U) {
    return 0U;
  }
  uint8_t log = 0U;
  uint32_t t = n;
  while(t > 1U)
  {
    log++;
    t >>= 1U;
  }
  if (n & 1U) { log++; }
  return log;
}

/** \brief  Apply cache maintenance to given cache level.
* \param [in] level cache level to be maintained
* \param [in] maint 0 - invalidate, 1 - clean, otherwise - invalidate and clean
*/
__STATIC_FORCEINLINE void __L1C_MaintainDCacheSetWay(uint32_t level, uint32_t maint)
{
  uint32_t Dummy;
  uint32_t ccsidr;
  uint32_t num_sets;
  uint32_t num_ways;
  uint32_t shift_way;
  uint32_t log2_linesize;
   int32_t log2_num_ways;

  Dummy = level << 1U;
  /* set csselr, select ccsidr register */
  __set_CSSELR(Dummy);
  /* get current ccsidr register */
  ccsidr = __get_CCSIDR();
  num_sets = ((ccsidr & 0x0FFFE000U) >> 13U) + 1U;
  num_ways = ((ccsidr & 0x00001FF8U) >> 3U) + 1U;
  log2_linesize = (ccsidr & 0x00000007U) + 2U + 2U;
  log2_num_ways = __log2_up(num_ways);
  if ((log2_num_ways < 0) || (log2_num_ways > 32)) {
    return; // FATAL ERROR
  }
  shift_way = 32U - (uint32_t)log2_num_ways;
  for(int32_t way = num_ways-1; way >= 0; way--)
  {
    for(int32_t set = num_sets-1; set >= 0; set--)
    {
      Dummy = (level << 1U) | (((uint32_t)set) << log2_linesize) | (((uint32_t)way) << shift_way);
      switch (maint)
      {
        case 0U: __set_DCISW(Dummy);  break;
        case 1U: __set_DCCSW(Dummy);  break;
        default: __set_DCCISW(Dummy); break;
      }
    }
  }
  __DMB();
}

/** \brief  Clean and Invalidate the entire data or unified cache
* Generic mechanism for cleaning/invalidating the entire data or unified cache to the point of coherency
* \param [in] op 0 - invalidate, 1 - clean, otherwise - invalidate and clean
*/
__STATIC_FORCEINLINE void L1C_CleanInvalidateCache(uint32_t op) {
  uint32_t clidr;
  uint32_t cache_type;
  clidr =  __get_CLIDR();
  for(uint32_t i = 0U; i<7U; i++)
  {
    cache_type = (clidr >> i*3U) & 0x7UL;
    if ((cache_type >= 2U) && (cache_type <= 4U))
    {
      __L1C_MaintainDCacheSetWay(i, op);
    }
  }
}

/** \brief  Clean and Invalidate the entire data or unified cache
* Generic mechanism for cleaning/invalidating the entire data or unified cache to the point of coherency
* \param [in] op 0 - invalidate, 1 - clean, otherwise - invalidate and clean
* \deprecated Use generic L1C_CleanInvalidateCache instead.
*/
CMSIS_DEPRECATED
__STATIC_FORCEINLINE void __L1C_CleanInvalidateCache(uint32_t op) {
  L1C_CleanInvalidateCache(op);
}

/** \brief  Invalidate the whole data cache.
*/
__STATIC_FORCEINLINE void L1C_InvalidateDCacheAll(void) {
  L1C_CleanInvalidateCache(0);
}

/** \brief  Clean the whole data cache.
 */
__STATIC_FORCEINLINE void L1C_CleanDCacheAll(void) {
  L1C_CleanInvalidateCache(1);
}

/** \brief  Clean and invalidate the whole data cache.
 */
__STATIC_FORCEINLINE void L1C_CleanInvalidateDCacheAll(void) {
  L1C_CleanInvalidateCache(2);
}

/* ##########################  GIC functions  ###################################### */
#if (__GIC_PRESENT == 1U) || defined(DOXYGEN)

/** \brief  Enable the interrupt distributor using the GIC's CTLR register.
*/
__STATIC_INLINE void GIC_EnableDistributor(void)
{
  GICDistributor->CTLR |= 1U;
}

/** \brief Disable the interrupt distributor using the GIC's CTLR register.
*/
__STATIC_INLINE void GIC_DisableDistributor(void)
{
  GICDistributor->CTLR &=~1U;
}

/** \brief Read the GIC's TYPER register.
* \return GICDistributor_Type::TYPER
*/
__STATIC_INLINE uint32_t GIC_DistributorInfo(void)
{
  return (GICDistributor->TYPER);
}

/** \brief Reads the GIC's IIDR register.
* \return GICDistributor_Type::IIDR
*/
__STATIC_INLINE uint32_t GIC_DistributorImplementer(void)
{
  return (GICDistributor->IIDR);
}

/** \brief Sets the GIC's ITARGETSR register for the given interrupt.
* \param [in] IRQn Interrupt to be configured.
* \param [in] cpu_target CPU interfaces to assign this interrupt to.
*/
__STATIC_INLINE void GIC_SetTarget(IRQn_Type IRQn, uint32_t cpu_target)
{
  uint32_t mask = GICDistributor->ITARGETSR[IRQn / 4U] & ~(0xFFUL << ((IRQn % 4U) * 8U));
  GICDistributor->ITARGETSR[IRQn / 4U] = mask | ((cpu_target & 0xFFUL) << ((IRQn % 4U) * 8U));
}

/** \brief Read the GIC's ITARGETSR register.
* \param [in] IRQn Interrupt to acquire the configuration for.
* \return GICDistributor_Type::ITARGETSR
*/
__STATIC_INLINE uint32_t GIC_GetTarget(IRQn_Type IRQn)
{
  return (GICDistributor->ITARGETSR[IRQn / 4U] >> ((IRQn % 4U) * 8U)) & 0xFFUL;
}

/** \brief Enable the CPU's interrupt interface.
*/
__STATIC_INLINE void GIC_EnableInterface(void)
{
  GICInterface->CTLR |= 1U; //enable interface
}

/** \brief Disable the CPU's interrupt interface.
*/
__STATIC_INLINE void GIC_DisableInterface(void)
{
  GICInterface->CTLR &=~1U; //disable distributor
}

/** \brief Read the CPU's IAR register.
* \return GICInterface_Type::IAR
*/
__STATIC_INLINE IRQn_Type GIC_AcknowledgePending(void)
{
  return (IRQn_Type)(GICInterface->IAR);
}

/** \brief Writes the given interrupt number to the CPU's EOIR register.
* \param [in] IRQn The interrupt to be signaled as finished.
*/
__STATIC_INLINE void GIC_EndInterrupt(IRQn_Type IRQn)
{
  GICInterface->EOIR = IRQn;
}

/** \brief Enables the given interrupt using GIC's ISENABLER register.
* \param [in] IRQn The interrupt to be enabled.
*/
__STATIC_INLINE void GIC_EnableIRQ(IRQn_Type IRQn)
{
  GICDistributor->ISENABLER[IRQn / 32U] = 1U << (IRQn % 32U);
}

/** \brief Get interrupt enable status using GIC's ISENABLER register.
* \param [in] IRQn The interrupt to be queried.
* \return 0 - interrupt is not enabled, 1 - interrupt is enabled.
*/
__STATIC_INLINE uint32_t GIC_GetEnableIRQ(IRQn_Type IRQn)
{
  return (GICDistributor->ISENABLER[IRQn / 32U] >> (IRQn % 32U)) & 1UL;
}

/** \brief Disables the given interrupt using GIC's ICENABLER register.
* \param [in] IRQn The interrupt to be disabled.
*/
__STATIC_INLINE void GIC_DisableIRQ(IRQn_Type IRQn)
{
  GICDistributor->ICENABLER[IRQn / 32U] = 1U << (IRQn % 32U);
}

/** \brief Get interrupt pending status from GIC's ISPENDR register.
* \param [in] IRQn The interrupt to be queried.
* \return 0 - interrupt is not pending, 1 - interrupt is pendig.
*/
__STATIC_INLINE uint32_t GIC_GetPendingIRQ(IRQn_Type IRQn)
{
  uint32_t pend;

  if (IRQn >= 16U) {
    pend = (GICDistributor->ISPENDR[IRQn / 32U] >> (IRQn % 32U)) & 1UL;
  } else {
    // INTID 0-15 Software Generated Interrupt
    pend = (GICDistributor->SPENDSGIR[IRQn / 4U] >> ((IRQn % 4U) * 8U)) & 0xFFUL;
    // No CPU identification offered
    if (pend != 0U) {
      pend = 1U;
    } else {
      pend = 0U;
    }
  }

  return (pend);
}

/** \brief Sets the given interrupt as pending using GIC's ISPENDR register.
* \param [in] IRQn The interrupt to be enabled.
*/
__STATIC_INLINE void GIC_SetPendingIRQ(IRQn_Type IRQn)
{
  if (IRQn >= 16U) {
    GICDistributor->ISPENDR[IRQn / 32U] = 1U << (IRQn % 32U);
  } else {
    // INTID 0-15 Software Generated Interrupt
    GICDistributor->SPENDSGIR[IRQn / 4U] = 1U << ((IRQn % 4U) * 8U);
  }
}

/** \brief Clears the given interrupt from being pending using GIC's ICPENDR register.
* \param [in] IRQn The interrupt to be enabled.
*/
__STATIC_INLINE void GIC_ClearPendingIRQ(IRQn_Type IRQn)
{
  if (IRQn >= 16U) {
    GICDistributor->ICPENDR[IRQn / 32U] = 1U << (IRQn % 32U);
  } else {
    // INTID 0-15 Software Generated Interrupt
    GICDistributor->CPENDSGIR[IRQn / 4U] = 1U << ((IRQn % 4U) * 8U);
  }
}

/** \brief Sets the interrupt configuration using GIC's ICFGR register.
* \param [in] IRQn The interrupt to be configured.
* \param [in] int_config Int_config field value. Bit 0: Reserved (0 - N-N model, 1 - 1-N model for some GIC before v1)
*                                           Bit 1: 0 - level sensitive, 1 - edge triggered
*/
__STATIC_INLINE void GIC_SetConfiguration(IRQn_Type IRQn, uint32_t int_config)
{
  uint32_t icfgr = GICDistributor->ICFGR[IRQn / 16U];
  uint32_t shift = (IRQn % 16U) << 1U;

  icfgr &= (~(3U         << shift));
  icfgr |= (  int_config << shift);

  GICDistributor->ICFGR[IRQn / 16U] = icfgr;
}

/** \brief Get the interrupt configuration from the GIC's ICFGR register.
* \param [in] IRQn Interrupt to acquire the configuration for.
* \return Int_config field value. Bit 0: Reserved (0 - N-N model, 1 - 1-N model for some GIC before v1)
*                                 Bit 1: 0 - level sensitive, 1 - edge triggered
*/
__STATIC_INLINE uint32_t GIC_GetConfiguration(IRQn_Type IRQn)
{
  return (GICDistributor->ICFGR[IRQn / 16U] >> ((IRQn % 16U) >> 1U));
}

/** \brief Set the priority for the given interrupt in the GIC's IPRIORITYR register.
* \param [in] IRQn The interrupt to be configured.
* \param [in] priority The priority for the interrupt, lower values denote higher priorities.
*/
__STATIC_INLINE void GIC_SetPriority(IRQn_Type IRQn, uint32_t priority)
{
  uint32_t mask = GICDistributor->IPRIORITYR[IRQn / 4U] & ~(0xFFUL << ((IRQn % 4U) * 8U));
  GICDistributor->IPRIORITYR[IRQn / 4U] = mask | ((priority & 0xFFUL) << ((IRQn % 4U) * 8U));
}

/** \brief Read the current interrupt priority from GIC's IPRIORITYR register.
* \param [in] IRQn The interrupt to be queried.
*/
__STATIC_INLINE uint32_t GIC_GetPriority(IRQn_Type IRQn)
{
  return (GICDistributor->IPRIORITYR[IRQn / 4U] >> ((IRQn % 4U) * 8U)) & 0xFFUL;
}

/** \brief Set the interrupt priority mask using CPU's PMR register.
* \param [in] priority Priority mask to be set.
*/
__STATIC_INLINE void GIC_SetInterfacePriorityMask(uint32_t priority)
{
  GICInterface->PMR = priority & 0xFFUL; //set priority mask
}

/** \brief Read the current interrupt priority mask from CPU's PMR register.
* \result GICInterface_Type::PMR
*/
__STATIC_INLINE uint32_t GIC_GetInterfacePriorityMask(void)
{
  return GICInterface->PMR;
}

/** \brief Configures the group priority and subpriority split point using CPU's BPR register.
* \param [in] binary_point Amount of bits used as subpriority.
*/
__STATIC_INLINE void GIC_SetBinaryPoint(uint32_t binary_point)
{
  GICInterface->BPR = binary_point & 7U; //set binary point
}

/** \brief Read the current group priority and subpriority split point from CPU's BPR register.
* \return GICInterface_Type::BPR
*/
__STATIC_INLINE uint32_t GIC_GetBinaryPoint(void)
{
  return GICInterface->BPR;
}

/** \brief Get the status for a given interrupt.
* \param [in] IRQn The interrupt to get status for.
* \return 0 - not pending/active, 1 - pending, 2 - active, 3 - pending and active
*/
__STATIC_INLINE uint32_t GIC_GetIRQStatus(IRQn_Type IRQn)
{
  uint32_t pending, active;

  active = ((GICDistributor->ISACTIVER[IRQn / 32U])  >> (IRQn % 32U)) & 1UL;
  pending = ((GICDistributor->ISPENDR[IRQn / 32U]) >> (IRQn % 32U)) & 1UL;

  return ((active<<1U) | pending);
}

/** \brief Generate a software interrupt using GIC's SGIR register.
* \param [in] IRQn Software interrupt to be generated.
* \param [in] target_list List of CPUs the software interrupt should be forwarded to.
* \param [in] filter_list Filter to be applied to determine interrupt receivers.
*/
__STATIC_INLINE void GIC_SendSGI(IRQn_Type IRQn, uint32_t target_list, uint32_t filter_list)
{
  GICDistributor->SGIR = ((filter_list & 3U) << 24U) | ((target_list & 0xFFUL) << 16U) | (IRQn & 0x0FUL);
}

/** \brief Get the interrupt number of the highest interrupt pending from CPU's HPPIR register.
* \return GICInterface_Type::HPPIR
*/
__STATIC_INLINE uint32_t GIC_GetHighPendingIRQ(void)
{
  return GICInterface->HPPIR;
}

/** \brief Provides information about the implementer and revision of the CPU interface.
* \return GICInterface_Type::IIDR
*/
__STATIC_INLINE uint32_t GIC_GetInterfaceId(void)
{
  return GICInterface->IIDR;
}

/** \brief Set the interrupt group from the GIC's IGROUPR register.
* \param [in] IRQn The interrupt to be queried.
* \param [in] group Interrupt group number: 0 - Group 0, 1 - Group 1
*/
__STATIC_INLINE void GIC_SetGroup(IRQn_Type IRQn, uint32_t group)
{
  uint32_t igroupr = GICDistributor->IGROUPR[IRQn / 32U];
  uint32_t shift   = (IRQn % 32U);

  igroupr &= (~(1U          << shift));
  igroupr |= ( (group & 1U) << shift);

  GICDistributor->IGROUPR[IRQn / 32U] = igroupr;
}
#define GIC_SetSecurity         GIC_SetGroup

/** \brief Get the interrupt group from the GIC's IGROUPR register.
* \param [in] IRQn The interrupt to be queried.
* \return 0 - Group 0, 1 - Group 1
*/
__STATIC_INLINE uint32_t GIC_GetGroup(IRQn_Type IRQn)
{
  return (GICDistributor->IGROUPR[IRQn / 32U] >> (IRQn % 32U)) & 1UL;
}
#define GIC_GetSecurity         GIC_GetGroup

/** \brief Initialize the interrupt distributor.
*/
__STATIC_INLINE void GIC_DistInit(void)
{
  uint32_t i;
  uint32_t num_irq = 0U;
  uint32_t priority_field;

  //A reset sets all bits in the IGROUPRs corresponding to the SPIs to 0,
  //configuring all of the interrupts as Secure.

  //Disable interrupt forwarding
  GIC_DisableDistributor();
  //Get the maximum number of interrupts that the GIC supports
  num_irq = 32U * ((GIC_DistributorInfo() & 0x1FU) + 1U);

  /* Priority level is implementation defined.
   To determine the number of priority bits implemented write 0xFF to an IPRIORITYR
   priority field and read back the value stored.*/
  GIC_SetPriority((IRQn_Type)0U, 0xFFU);
  priority_field = GIC_GetPriority((IRQn_Type)0U);

  for (i = 32U; i < num_irq; i++)
  {
      //Disable the SPI interrupt
      GIC_DisableIRQ((IRQn_Type)i);
      //Set level-sensitive (and N-N model)
      GIC_SetConfiguration((IRQn_Type)i, 0U);
      //Set priority
      GIC_SetPriority((IRQn_Type)i, priority_field/2U);
      //Set target list to CPU0
      GIC_SetTarget((IRQn_Type)i, 1U);
  }
  //Enable distributor
  GIC_EnableDistributor();
}

/** \brief Initialize the CPU's interrupt interface
*/
__STATIC_INLINE void GIC_CPUInterfaceInit(void)
{
  uint32_t i;
  uint32_t priority_field;

  //A reset sets all bits in the IGROUPRs corresponding to the SPIs to 0,
  //configuring all of the interrupts as Secure.

  //Disable interrupt forwarding
  GIC_DisableInterface();

  /* Priority level is implementation defined.
   To determine the number of priority bits implemented write 0xFF to an IPRIORITYR
   priority field and read back the value stored.*/
  GIC_SetPriority((IRQn_Type)0U, 0xFFU);
  priority_field = GIC_GetPriority((IRQn_Type)0U);

  //SGI and PPI
  for (i = 0U; i < 32U; i++)
  {
    if(i > 15U) {
      //Set level-sensitive (and N-N model) for PPI
      GIC_SetConfiguration((IRQn_Type)i, 0U);
    }
    //Disable SGI and PPI interrupts
    GIC_DisableIRQ((IRQn_Type)i);
    //Set priority
    GIC_SetPriority((IRQn_Type)i, priority_field/2U);
  }
  //Enable interface
  GIC_EnableInterface();
  //Set binary point to 0
  GIC_SetBinaryPoint(0U);
  //Set priority mask
  GIC_SetInterfacePriorityMask(0xFFU);
}

/** \brief Initialize and enable the GIC
*/
__STATIC_INLINE void GIC_Enable(void)
{
  GIC_DistInit();
  GIC_CPUInterfaceInit(); //per CPU
}
#endif

/* ##########################  Generic Timer functions  ############################ */
#if (__TIM_PRESENT == 1U) || defined(DOXYGEN)

/* PL1 Physical Timer */
#if (__CORTEX_R == 52U) || defined(DOXYGEN)

/** \brief Physical Timer Control register */
typedef union
{
  struct
  {
    uint32_t ENABLE:1;      /*!< \brief bit: 0      Enables the timer. */
    uint32_t IMASK:1;       /*!< \brief bit: 1      Timer output signal mask bit. */
    uint32_t ISTATUS:1;     /*!< \brief bit: 2      The status of the timer. */
    RESERVED(0:29, uint32_t)
  } b;                      /*!< \brief Structure used for bit  access */
  uint32_t w;               /*!< \brief Type      used for word access */
} CNTP_CTL_Type;

/** \brief Configures the frequency the timer shall run at.
* \param [in] value The timer frequency in Hz.
*/
__STATIC_INLINE void PL1_SetCounterFrequency(uint32_t value)
{
  __set_CNTFRQ(value);
  __ISB();
}

/** \brief Sets the reset value of the timer.
* \param [in] value The value the timer is loaded with.
*/
__STATIC_INLINE void PL1_SetLoadValue(uint32_t value)
{
  __set_CNTP_TVAL(value);
  __ISB();
}

/** \brief Get the current counter value.
* \return Current counter value.
*/
__STATIC_INLINE uint32_t PL1_GetCurrentValue(void)
{
  return(__get_CNTP_TVAL());
}

/** \brief Get the current physical counter value.
* \return Current physical counter value.
*/
__STATIC_INLINE uint64_t PL1_GetCurrentPhysicalValue(void)
{
  return(__get_CNTPCT());
}

/** \brief Set the physical compare value.
* \param [in] value New physical timer compare value.
*/
__STATIC_INLINE void PL1_SetPhysicalCompareValue(uint64_t value)
{
  __set_CNTP_CVAL(value);
  __ISB();
}

/** \brief Get the physical compare value.
* \return Physical compare value.
*/
__STATIC_INLINE uint64_t PL1_GetPhysicalCompareValue(void)
{
  return(__get_CNTP_CVAL());
}

/** \brief Configure the timer by setting the control value.
* \param [in] value New timer control value.
*/
__STATIC_INLINE void PL1_SetControl(uint32_t value)
{
  __set_CNTP_CTL(value);
  __ISB();
}

/** \brief Get the control value.
* \return Control value.
*/
__STATIC_INLINE uint32_t PL1_GetControl(void)
{
  return(__get_CNTP_CTL());
}
#endif

/* Private Timer */
#if ((__CORTEX_R == 7U) || (__CORTEX_R == 8U)) || defined(DOXYGEN)
/** \brief Set the load value to timers LOAD register.
* \param [in] value The load value to be set.
*/
__STATIC_INLINE void PTIM_SetLoadValue(uint32_t value)
{
  PTIM->LOAD = value;
}

/** \brief Get the load value from timers LOAD register.
* \return Timer_Type::LOAD
*/
__STATIC_INLINE uint32_t PTIM_GetLoadValue(void)
{
  return(PTIM->LOAD);
}

/** \brief Set current counter value from its COUNTER register.
*/
__STATIC_INLINE void PTIM_SetCurrentValue(uint32_t value)
{
  PTIM->COUNTER = value;
}

/** \brief Get current counter value from timers COUNTER register.
* \result Timer_Type::COUNTER
*/
__STATIC_INLINE uint32_t PTIM_GetCurrentValue(void)
{
  return(PTIM->COUNTER);
}

/** \brief Configure the timer using its CONTROL register.
* \param [in] value The new configuration value to be set.
*/
__STATIC_INLINE void PTIM_SetControl(uint32_t value)
{
  PTIM->CONTROL = value;
}

/** ref Timer_Type::CONTROL Get the current timer configuration from its CONTROL register.
* \return Timer_Type::CONTROL
*/
__STATIC_INLINE uint32_t PTIM_GetControl(void)
{
  return(PTIM->CONTROL);
}

/** ref Timer_Type::CONTROL Get the event flag in timers ISR register.
* \return 0 - flag is not set, 1- flag is set
*/
__STATIC_INLINE uint32_t PTIM_GetEventFlag(void)
{
  return (PTIM->ISR & 1UL);
}

/** ref Timer_Type::CONTROL Clears the event flag in timers ISR register.
*/
__STATIC_INLINE void PTIM_ClearEventFlag(void)
{
  PTIM->ISR = 1;
}
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CORE_CR_H_DEPENDANT */

#endif /* __CMSIS_GENERIC */
