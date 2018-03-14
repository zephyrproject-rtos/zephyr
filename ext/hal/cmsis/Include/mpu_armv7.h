/******************************************************************************
 * @file     mpu_armv7.h
 * @brief    CMSIS MPU API for Armv7-M MPU
 * @version  V5.0.4
 * @date     10. January 2018
 ******************************************************************************/
/*
 * Copyright (c) 2017-2018 Arm Limited. All rights reserved.
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
  #pragma clang system_header    /* treat file as system include file */
#endif
 
#ifndef ARM_MPU_ARMV7_H
#define ARM_MPU_ARMV7_H

#define ARM_MPU_REGION_SIZE_32B      ((uint8_t)0x04U)
#define ARM_MPU_REGION_SIZE_64B      ((uint8_t)0x05U)
#define ARM_MPU_REGION_SIZE_128B     ((uint8_t)0x06U)
#define ARM_MPU_REGION_SIZE_256B     ((uint8_t)0x07U)
#define ARM_MPU_REGION_SIZE_512B     ((uint8_t)0x08U)
#define ARM_MPU_REGION_SIZE_1KB      ((uint8_t)0x09U)
#define ARM_MPU_REGION_SIZE_2KB      ((uint8_t)0x0AU)
#define ARM_MPU_REGION_SIZE_4KB      ((uint8_t)0x0BU)
#define ARM_MPU_REGION_SIZE_8KB      ((uint8_t)0x0CU)
#define ARM_MPU_REGION_SIZE_16KB     ((uint8_t)0x0DU)
#define ARM_MPU_REGION_SIZE_32KB     ((uint8_t)0x0EU)
#define ARM_MPU_REGION_SIZE_64KB     ((uint8_t)0x0FU)
#define ARM_MPU_REGION_SIZE_128KB    ((uint8_t)0x10U)
#define ARM_MPU_REGION_SIZE_256KB    ((uint8_t)0x11U)
#define ARM_MPU_REGION_SIZE_512KB    ((uint8_t)0x12U)
#define ARM_MPU_REGION_SIZE_1MB      ((uint8_t)0x13U)
#define ARM_MPU_REGION_SIZE_2MB      ((uint8_t)0x14U)
#define ARM_MPU_REGION_SIZE_4MB      ((uint8_t)0x15U)
#define ARM_MPU_REGION_SIZE_8MB      ((uint8_t)0x16U)
#define ARM_MPU_REGION_SIZE_16MB     ((uint8_t)0x17U)
#define ARM_MPU_REGION_SIZE_32MB     ((uint8_t)0x18U)
#define ARM_MPU_REGION_SIZE_64MB     ((uint8_t)0x19U)
#define ARM_MPU_REGION_SIZE_128MB    ((uint8_t)0x1AU)
#define ARM_MPU_REGION_SIZE_256MB    ((uint8_t)0x1BU)
#define ARM_MPU_REGION_SIZE_512MB    ((uint8_t)0x1CU)
#define ARM_MPU_REGION_SIZE_1GB      ((uint8_t)0x1DU)
#define ARM_MPU_REGION_SIZE_2GB      ((uint8_t)0x1EU)
#define ARM_MPU_REGION_SIZE_4GB      ((uint8_t)0x1FU)

#define ARM_MPU_AP_NONE 0U 
#define ARM_MPU_AP_PRIV 1U
#define ARM_MPU_AP_URO  2U
#define ARM_MPU_AP_FULL 3U
#define ARM_MPU_AP_PRO  5U
#define ARM_MPU_AP_RO   6U

/** MPU Region Base Address Register Value
*
* \param Region The region to be configured, number 0 to 15.
* \param BaseAddress The base address for the region.
*/
#define ARM_MPU_RBAR(Region, BaseAddress) \
  (((BaseAddress) & MPU_RBAR_ADDR_Msk) |  \
   ((Region) & MPU_RBAR_REGION_Msk)    |  \
   (MPU_RBAR_VALID_Msk))

/**
* MPU Region Attribute and Size Register Value
* 
* \param DisableExec       Instruction access disable bit, 1= disable instruction fetches.
* \param AccessPermission  Data access permissions, allows you to configure read/write access for User and Privileged mode.
* \param TypeExtField      Type extension field, allows you to configure memory access type, for example strongly ordered, peripheral.
* \param IsShareable       Region is shareable between multiple bus masters.
* \param IsCacheable       Region is cacheable, i.e. its value may be kept in cache.
* \param IsBufferable      Region is bufferable, i.e. using write-back caching. Cacheable but non-bufferable regions use write-through policy.
* \param SubRegionDisable  Sub-region disable field.
* \param Size              Region size of the region to be configured, for example 4K, 8K.
*/                         
#define ARM_MPU_RASR(DisableExec, AccessPermission, TypeExtField, IsShareable, IsCacheable, IsBufferable, SubRegionDisable, Size) \
  ((((DisableExec     ) << MPU_RASR_XN_Pos)     & MPU_RASR_XN_Msk)     | \
   (((AccessPermission) << MPU_RASR_AP_Pos)     & MPU_RASR_AP_Msk)     | \
   (((TypeExtField    ) << MPU_RASR_TEX_Pos)    & MPU_RASR_TEX_Msk)    | \
   (((IsShareable     ) << MPU_RASR_S_Pos)      & MPU_RASR_S_Msk)      | \
   (((IsCacheable     ) << MPU_RASR_C_Pos)      & MPU_RASR_C_Msk)      | \
   (((IsBufferable    ) << MPU_RASR_B_Pos)      & MPU_RASR_B_Msk)      | \
   (((SubRegionDisable) << MPU_RASR_SRD_Pos)    & MPU_RASR_SRD_Msk)    | \
   (((Size            ) << MPU_RASR_SIZE_Pos)   & MPU_RASR_SIZE_Msk)   | \
   (MPU_RASR_ENABLE_Msk))


/**
* Struct for a single MPU Region
*/
typedef struct {
  uint32_t RBAR; //!< The region base address register value (RBAR)
  uint32_t RASR; //!< The region attribute and size register value (RASR) \ref MPU_RASR
} ARM_MPU_Region_t;
    
/** Enable the MPU.
* \param MPU_Control Default access permissions for unconfigured regions.
*/
__STATIC_INLINE void ARM_MPU_Enable(uint32_t MPU_Control)
{
  __DSB();
  __ISB();
  MPU->CTRL = MPU_Control | MPU_CTRL_ENABLE_Msk;
#ifdef SCB_SHCSR_MEMFAULTENA_Msk
  SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
#endif
}

/** Disable the MPU.
*/
__STATIC_INLINE void ARM_MPU_Disable(void)
{
  __DSB();
  __ISB();
#ifdef SCB_SHCSR_MEMFAULTENA_Msk
  SCB->SHCSR &= ~SCB_SHCSR_MEMFAULTENA_Msk;
#endif
  MPU->CTRL  &= ~MPU_CTRL_ENABLE_Msk;
}

/** Clear and disable the given MPU region.
* \param rnr Region number to be cleared.
*/
__STATIC_INLINE void ARM_MPU_ClrRegion(uint32_t rnr)
{
  MPU->RNR = rnr;
  MPU->RASR = 0U;
}

/** Configure an MPU region.
* \param rbar Value for RBAR register.
* \param rsar Value for RSAR register.
*/   
__STATIC_INLINE void ARM_MPU_SetRegion(uint32_t rbar, uint32_t rasr)
{
  MPU->RBAR = rbar;
  MPU->RASR = rasr;
}

/** Configure the given MPU region.
* \param rnr Region number to be configured.
* \param rbar Value for RBAR register.
* \param rsar Value for RSAR register.
*/   
__STATIC_INLINE void ARM_MPU_SetRegionEx(uint32_t rnr, uint32_t rbar, uint32_t rasr)
{
  MPU->RNR = rnr;
  MPU->RBAR = rbar;
  MPU->RASR = rasr;
}

/** Memcopy with strictly ordered memory access, e.g. for register targets.
* \param dst Destination data is copied to.
* \param src Source data is copied from.
* \param len Amount of data words to be copied.
*/
__STATIC_INLINE void orderedCpy(volatile uint32_t* dst, const uint32_t* __RESTRICT src, uint32_t len)
{
  uint32_t i;
  for (i = 0U; i < len; ++i) 
  {
    dst[i] = src[i];
  }
}

/** Load the given number of MPU regions from a table.
* \param table Pointer to the MPU configuration table.
* \param cnt Amount of regions to be configured.
*/
__STATIC_INLINE void ARM_MPU_Load(ARM_MPU_Region_t const* table, uint32_t cnt) 
{
  const uint32_t rowWordSize = sizeof(ARM_MPU_Region_t)/4U;
  while (cnt > MPU_TYPE_RALIASES) {
    orderedCpy(&(MPU->RBAR), &(table->RBAR), MPU_TYPE_RALIASES*rowWordSize);
    table += MPU_TYPE_RALIASES;
    cnt -= MPU_TYPE_RALIASES;
  }
  orderedCpy(&(MPU->RBAR), &(table->RBAR), cnt*rowWordSize);
}

#endif
