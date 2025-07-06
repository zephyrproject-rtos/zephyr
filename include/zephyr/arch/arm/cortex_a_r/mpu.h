/* SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2019 Lexmark International, Inc.
 */
#ifndef ARCH_ARM_CORTEX_R_MPU_H
#define ARCH_ARM_CORTEX_R_MPU_H 1

#define MPU_RBAR_ADDR_Msk               (~0x1f)
#define MPU_RASR_ENABLE_Msk             (1)

#define MPU_RASR_SIZE_Pos               1U
#define MPU_RASR_SIZE_Msk               (0x1FUL << MPU_RASR_SIZE_Pos)

#define MPU_TYPE_DREGION_Pos    8U
#define MPU_TYPE_DREGION_Msk    (0xFFUL << MPU_TYPE_DREGION_Pos)

#define MPU_RASR_XN_Pos                 12
#define MPU_RASR_XN_Msk                 (1UL << MPU_RASR_XN_Pos)

#define MPU_RASR_AP_Pos                 8
#define MPU_RASR_AP_Msk                 (0x7UL << MPU_RASR_AP_Pos)

#define MPU_RASR_TEX_Pos                3
#define MPU_RASR_TEX_Msk                (0x7UL << MPU_RASR_TEX_Pos)

#define MPU_RASR_S_Pos                  2
#define MPU_RASR_S_Msk                  (1UL << MPU_RASR_S_Pos)

#define MPU_RASR_C_Pos                  1
#define MPU_RASR_C_Msk                  (1UL << MPU_RASR_C_Pos)

#define MPU_RASR_B_Pos                  0
#define MPU_RASR_B_Msk                  (1UL << MPU_RASR_B_Pos)

#if defined(CONFIG_CPU_CORTEX_R4) || defined(CONFIG_CPU_CORTEX_R5) || defined(CONFIG_CPU_CORTEX_R8)
#define ARM_MPU_REGION_SIZE_32B         ((uint8_t)0x04U)
#define ARM_MPU_REGION_SIZE_64B         ((uint8_t)0x05U)
#define ARM_MPU_REGION_SIZE_128B        ((uint8_t)0x06U)
#endif

#define ARM_MPU_REGION_SIZE_256B        ((uint8_t)0x07U)
#define ARM_MPU_REGION_SIZE_512B        ((uint8_t)0x08U)
#define ARM_MPU_REGION_SIZE_1KB         ((uint8_t)0x09U)
#define ARM_MPU_REGION_SIZE_2KB         ((uint8_t)0x0aU)
#define ARM_MPU_REGION_SIZE_4KB         ((uint8_t)0x0bU)
#define ARM_MPU_REGION_SIZE_8KB         ((uint8_t)0x0cU)
#define ARM_MPU_REGION_SIZE_16KB        ((uint8_t)0x0dU)
#define ARM_MPU_REGION_SIZE_32KB        ((uint8_t)0x0eU)
#define ARM_MPU_REGION_SIZE_64KB        ((uint8_t)0x0fU)
#define ARM_MPU_REGION_SIZE_128KB       ((uint8_t)0x10U)
#define ARM_MPU_REGION_SIZE_256KB       ((uint8_t)0x11U)
#define ARM_MPU_REGION_SIZE_512KB       ((uint8_t)0x12U)
#define ARM_MPU_REGION_SIZE_1MB         ((uint8_t)0x13U)
#define ARM_MPU_REGION_SIZE_2MB         ((uint8_t)0x14U)
#define ARM_MPU_REGION_SIZE_4MB         ((uint8_t)0x15U)
#define ARM_MPU_REGION_SIZE_8MB         ((uint8_t)0x16U)
#define ARM_MPU_REGION_SIZE_16MB        ((uint8_t)0x17U)
#define ARM_MPU_REGION_SIZE_32MB        ((uint8_t)0x18U)
#define ARM_MPU_REGION_SIZE_64MB        ((uint8_t)0x19U)
#define ARM_MPU_REGION_SIZE_128MB       ((uint8_t)0x1aU)
#define ARM_MPU_REGION_SIZE_256MB       ((uint8_t)0x1bU)
#define ARM_MPU_REGION_SIZE_512MB       ((uint8_t)0x1cU)
#define ARM_MPU_REGION_SIZE_1GB         ((uint8_t)0x1dU)
#define ARM_MPU_REGION_SIZE_2GB         ((uint8_t)0x1eU)
#define ARM_MPU_REGION_SIZE_4GB         ((uint8_t)0x1fU)

#endif
