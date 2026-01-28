/******************************************************************************
* @file  rsi_d_cache.c
*******************************************************************************
* # License
* <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
*******************************************************************************
*
* SPDX-License-Identifier: Zlib
*
* The licensor of this software is Silicon Laboratories Inc.
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*
******************************************************************************/

/*******************************************************************************
 ***************************  Include Files     ********************************
 ******************************************************************************/

#include "rsi_d_cache.h"

DCache_Reg_Type *DCACHE = (DCache_Reg_Type *)M4SS_DCACHE_BASE_ADDR; // DCache register access handle

/*==============================================*/
/**
 * @fn    void rsi_d_cache_enable(void)  
 * @brief This API is used to enable the data cache and sets it to write-through mode
 * @return None
 */
void rsi_d_cache_enable(void)
{
  /*Enable the Cache and write through*/
  DCACHE->CTRL |= (DCACHE_CTRL_ENABLE | DCACHE_CTRL_FORCE_WT);

  while (DCACHE->MAINT_STATUS != (DCACHE_MAINT_STATUS_CACHE_ENABLED | DCACHE_MAINT_STATUS_CACHE_IS_CLEAN))
    ;
}

/*==============================================*/
/**
 * @fn    void rsi_d_cache_disable(void)  
 * @brief This API is used to disable the data cache
 * @return None
 */
void rsi_d_cache_disable(void)
{
  /*Disable the Cache*/
  DCACHE->CTRL &= ~(DCACHE_CTRL_ENABLE);
  while ((DCACHE->MAINT_STATUS & (DCACHE_MAINT_STATUS_CACHE_ENABLED | DCACHE_MAINT_STATUS_ONGOING_EN_DIS)) != 0x0)
    ;
}

/*==============================================*/
/**
 * @fn    void rsi_d_cache_invalidate_all(void)  
 * @brief This API is used to invalidate all cache lines, forcing data to be fetched from memory on subsequent accesses
 * @return None
 */
void rsi_d_cache_invalidate_all(void)
{

  /*Wait until ongoing cache op is done, wait for ONGOING_EN_DIS,ONGOING_MAINT and ONGOING_PWR_MAINT*/
  while (
    (DCACHE->MAINT_STATUS
     & (DCACHE_MAINT_STATUS_ONGOING_EN_DIS | DCACHE_MAINT_STATUS_ONGOING_MAINT | DCACHE_MAINT_STATUS_ONGOING_PWR_MAINT))
    != 0x0)
    ;

  /*Clear Pending all interrupts, if needs to be served, clear after Interrupt serve*/
  DCACHE->SECIRQSCLR = DCACHE_SECIRQSCLR_CLEAR_ALL;

  /*Initiate invalidate entire cache*/
  DCACHE->MAINT_CTRL_ALL |= DCACHE_MAINT_CTRL_ALL_TRIG_INVALIDATE;

  /*Wait until the operation is finished*/
  while ((DCACHE->MAINT_STATUS & DCACHE_MAINT_STATUS_ONGOING_MAINT) != 0x0)
    ;
}

/*==============================================*/
/**
 * @fn    void rsi_d_cache_clean_up_all(void)  
 * @brief This API is used to write back all modified cache lines to memory, ensuring data consistency
 * @return None
 */
void rsi_d_cache_clean_up_all(void)
{
  /*Wait until ongoing cache op is done, wait for ONGOING_EN_DIS,ONGOING_MAINT and ONGOING_PWR_MAINT*/
  while (
    (DCACHE->MAINT_STATUS
     & (DCACHE_MAINT_STATUS_ONGOING_EN_DIS | DCACHE_MAINT_STATUS_ONGOING_MAINT | DCACHE_MAINT_STATUS_ONGOING_PWR_MAINT))
    != 0x0)
    ;

  /*Clear Pending all interrupts, if needs to be served, clear after Interrupt serve*/
  DCACHE->SECIRQSCLR = DCACHE_SECIRQSCLR_CLEAR_ALL;

  /*Initiate clean up for entire cache*/
  DCACHE->MAINT_CTRL_ALL |= DCACHE_MAINT_CTRL_ALL_TRIG_CLEAN;

  /*Wait until the operation is finished*/
  while ((DCACHE->MAINT_STATUS & DCACHE_MAINT_STATUS_ONGOING_MAINT) != 0x0)
    ;
}
/*==============================================*/
/**
 * @fn         void rsi_d_cache_invalidate_address(uint32_t address)
 * @brief      This API is used to invalidate the cache line that contains the specified address, forcing subsequent accesses to fetch data from memory
 * @param[in]  address: The memory address whose cache line needs to be invalidated.
 * @return     None
 */
void rsi_d_cache_invalidate_address(uint32_t address)
{

  /*Wait until ongoing cache op is done, wait for ONGOING_EN_DIS,ONGOING_MAINT and ONGOING_PWR_MAINT*/
  while ((DCACHE->MAINT_STATUS
          & (DCACHE->MAINT_STATUS
             & (DCACHE_MAINT_STATUS_ONGOING_EN_DIS | DCACHE_MAINT_STATUS_ONGOING_MAINT
                | DCACHE_MAINT_STATUS_ONGOING_PWR_MAINT)))
         != 0x0)
    ;

  /*Clear Pending all interrupts, if needs to be served, clear after Interrupt serve*/
  DCACHE->SECIRQSCLR = DCACHE_SECIRQSCLR_CLEAR_ALL;

  address = address & ~(DCACHE_MAINT_CTRL_LINES_LOWER_ADDRESS_MASK);

  address = address | (1 << DCACHE_MAINT_CTRL_LINES_TRIG_INVALIDATE);

  DCACHE->MAINT_CTRL_LINES = address;

  /*Wait until the operation is finished*/
  while ((DCACHE->MAINT_STATUS & DCACHE_MAINT_STATUS_ONGOING_MAINT) != 0x0)
    ;
}
/*==============================================*/
/**
 * @fn         void rsi_d_cache_clean_up_address(uint32_t address)
 * @brief      This API is used to write back the cache line that contains the specified address to memory, ensuring data consistency for that line
 * @param[in]  address: The memory address whose cache line needs to be cleaned.
 * @return     None
 */
void rsi_d_cache_clean_up_address(uint32_t address)
{
  /*Wait until ongoing cache op is done, wait for ONGOING_EN_DIS,ONGOING_MAINT and ONGOING_PWR_MAINT*/
  while ((DCACHE->MAINT_STATUS
          & (DCACHE->MAINT_STATUS
             & (DCACHE_MAINT_STATUS_ONGOING_EN_DIS | DCACHE_MAINT_STATUS_ONGOING_MAINT
                | DCACHE_MAINT_STATUS_ONGOING_PWR_MAINT)))
         != 0x0)
    ;

  /*Clear Pending all interrupts, if needs to be served, clear after Interrupt serve*/
  DCACHE->SECIRQSCLR = DCACHE_SECIRQSCLR_CLEAR_ALL;

  address = address & ~(DCACHE_MAINT_CTRL_LINES_LOWER_ADDRESS_MASK);

  address                  = address | (1 << DCACHE_MAINT_CTRL_LINES_TRIG_CLEAN);
  DCACHE->MAINT_CTRL_LINES = address;

  /*Wait until the operation is finished*/
  while ((DCACHE->MAINT_STATUS & DCACHE_MAINT_STATUS_ONGOING_MAINT) != 0x0)
    ;
}
/*==============================================*/
/**
 * @fn    void rsi_d_cache_enable_stats(void)  
 * @brief This API is used to enable the data cache statistics counter and reset its value to zero
 * @return None
 */
void rsi_d_cache_enable_stats(void)
{
  /*Clear Pending all interrupts, if needs to be served, clear after Interrupt serve*/
  DCACHE->SECIRQSCLR = DCACHE_SECIRQSCLR_CLEAR_ALL;

  /*Enable Statistic counter*/
  DCACHE->SECSTATCTRL |= DCACHE_SECSTATCTRL_ENABLE_COUNTER;

  /*Reset Statistic counter*/
  DCACHE->SECSTATCTRL |= DCACHE_SECSTATCTRL_RESET_COUNTER;
}

/*==============================================*/
/**
 * @fn    void rsi_d_cache_disable_stats(void)  
 * @brief This API is used to disable the data cache statistics counter
 * @return None
 */
void rsi_d_cache_disable_stats(void)
{
  /*Clear Pending all interrupts, if needs to be served, clear after Interrupt serve*/
  DCACHE->SECIRQSCLR = DCACHE_SECIRQSCLR_CLEAR_ALL;

  /*Disable Statistic counter*/
  DCACHE->SECSTATCTRL &= ~(DCACHE_SECSTATCTRL_ENABLE_COUNTER);
}
/*==============================================*/
/**
 * @fn        void rsi_d_cache_get_stats(int *hit_count, int *miss_count)
 * @brief     This API is used to retrieve the data cache hit and miss counts.
 * @param[in] hit_count: Pointer to store the hit count.
 * @param[in] miss_count: Pointer to store the miss count.
 * @return 0 on success, -1 if the counters are saturated..
 *
 * @note This function returns 0 for both hit and miss counts if the counters are saturated.
 */
int rsi_d_cache_get_stats(int *hit_count, int *miss_count)
{

  /*Check if the counters are saturated*/
  if (DCACHE->SECIRQSTAT & DCACHE_SECIRQSTAT_NSECURE_CNT_SAT) {
    *hit_count  = 0x0;
    *miss_count = 0x0;
    return -1;
  }

  *hit_count  = DCACHE->SECHIT;
  *miss_count = DCACHE->SECMISS;
  return 0;
}
/*==============================================*/
/**
 * @fn    void rsi_d_cache_clear_stats(void)  
 * @brief This API is used to reset the data cache statistics counter to zero.
 * @return None
 */
void rsi_d_cache_clear_stats(void)
{
  /*Reset Statistic counter*/
  DCACHE->SECSTATCTRL |= DCACHE_SECSTATCTRL_RESET_COUNTER;
}
