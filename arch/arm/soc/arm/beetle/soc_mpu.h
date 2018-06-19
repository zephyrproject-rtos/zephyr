/*
 * Copyright (c) 2018 Arm Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM MPU-related macro definitions.
 *
 * ARM MPU macro definitions required for SOCs
 * which are not ARM CMSIS-compliant.
 */
#include <stdint.h>

#if defined(CONFIG_ARM_MPU)

#define     __IM     volatile const
#define     __OM     volatile
#define     __IOM    volatile

/**
  \brief  Structure type to access the Memory Protection Unit (MPU).
 */
typedef struct {
  __IM  u32_t TYPE;
  __IOM u32_t CTRL;
  __IOM u32_t RNR;
  __IOM u32_t RBAR;
  __IOM u32_t RASR;
  __IOM u32_t RBAR_A1;
  __IOM u32_t RASR_A1;
  __IOM u32_t RBAR_A2;
  __IOM u32_t RASR_A2;
  __IOM u32_t RBAR_A3;
  __IOM u32_t RASR_A3;
} MPU_Type;

#define MPU               ((MPU_Type       *)0xE000ED90UL)

/* MPU Control Register Definitions */
#define MPU_CTRL_PRIVDEFENA_Pos             2U
#define MPU_CTRL_PRIVDEFENA_Msk            (1UL << MPU_CTRL_PRIVDEFENA_Pos)

#define MPU_CTRL_HFNMIENA_Pos               1U
#define MPU_CTRL_HFNMIENA_Msk              (1UL << MPU_CTRL_HFNMIENA_Pos)

#define MPU_CTRL_ENABLE_Pos                 0U
#define MPU_CTRL_ENABLE_Msk                (1UL /*<< MPU_CTRL_ENABLE_Pos*/)

/* MPU Region Number Register Definitions */
#define MPU_RNR_REGION_Pos                  0U
#define MPU_RNR_REGION_Msk                 (0xFFUL /*<< MPU_RNR_REGION_Pos*/)

/* MPU Region Base Address Register Definitions */
#define MPU_RBAR_ADDR_Pos                   5U
#define MPU_RBAR_ADDR_Msk                  (0x7FFFFFFUL << MPU_RBAR_ADDR_Pos)

#define MPU_RBAR_VALID_Pos                  4U
#define MPU_RBAR_VALID_Msk                 (1UL << MPU_RBAR_VALID_Pos)

#define MPU_RBAR_REGION_Pos                 0U
#define MPU_RBAR_REGION_Msk                (0xFUL /*<< MPU_RBAR_REGION_Pos*/)

/* MPU Region Attribute and Size Register Definitions */
#define MPU_RASR_ATTRS_Pos                 16U
#define MPU_RASR_ATTRS_Msk                 (0xFFFFUL << MPU_RASR_ATTRS_Pos)

#define MPU_RASR_XN_Pos                    28U
#define MPU_RASR_XN_Msk                    (1UL << MPU_RASR_XN_Pos)

#define MPU_RASR_AP_Pos                    24U
#define MPU_RASR_AP_Msk                    (0x7UL << MPU_RASR_AP_Pos)

#define MPU_RASR_TEX_Pos                   19U
#define MPU_RASR_TEX_Msk                   (0x7UL << MPU_RASR_TEX_Pos)

#define MPU_RASR_S_Pos                     18U
#define MPU_RASR_S_Msk                     (1UL << MPU_RASR_S_Pos)

#define MPU_RASR_C_Pos                     17U
#define MPU_RASR_C_Msk                     (1UL << MPU_RASR_C_Pos)

#define MPU_RASR_B_Pos                     16U
#define MPU_RASR_B_Msk                     (1UL << MPU_RASR_B_Pos)

#define MPU_RASR_SRD_Pos                    8U
#define MPU_RASR_SRD_Msk                   (0xFFUL << MPU_RASR_SRD_Pos)

#define MPU_RASR_SIZE_Pos                   1U
#define MPU_RASR_SIZE_Msk                  (0x1FUL << MPU_RASR_SIZE_Pos)

#define MPU_RASR_ENABLE_Pos                 0U
#define MPU_RASR_ENABLE_Msk                (1UL /*<< MPU_RASR_ENABLE_Pos*/)

#endif /* CONFIG_ARM_MPU */
