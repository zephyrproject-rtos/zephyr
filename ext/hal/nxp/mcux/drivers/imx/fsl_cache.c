/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_cache.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.cache_armv7_m7"
#endif

#if defined(FSL_FEATURE_SOC_L2CACHEC_COUNT) && FSL_FEATURE_SOC_L2CACHEC_COUNT
#define L2CACHE_OPERATION_TIMEOUT 0xFFFFFU
#define L2CACHE_8WAYS_MASK 0xFFU
#define L2CACHE_16WAYS_MASK 0xFFFFU
#define L2CACHE_SMALLWAYS_NUM 8U
#define L2CACHE_1KBCOVERTOB 1024U
#define L2CACHE_SAMLLWAYS_SIZE 16U
#define L2CACHE_LOCKDOWN_REGNUM 8  /*!< Lock down register numbers.*/
/*******************************************************************************
* Prototypes
******************************************************************************/
/*!
 * @brief Set for all ways and waiting for the operation finished.
 *  This is provided for all the background operations.
 *
 * @param auxCtlReg  The auxiliary control register.
 * @param regAddr The register address to be operated.
 */
static void L2CACHE_SetAndWaitBackGroundOperate(uint32_t auxCtlReg, uint32_t regAddr);

/*!
 * @brief Invalidates the Level 2 cache line by physical address.
 * This function invalidates a cache line by physcial address.
 *
 * @param address  The physical addderss of the cache.
 *        The format of the address shall be :
 *        bit 31 ~ bit n+1 | bitn ~ bit5 | bit4 ~ bit0
 *              Tag        |    index    |      0
 *  Note: the physical address shall be aligned to the line size - 32B (256 bit).
 *  so keep the last 5 bits (bit 4 ~ bit 0) of the physical address always be zero.
 *  If the input address is not aligned, it will be changed to 32-byte aligned address.
 *  The n is varies according to the index width.
 * @return The actual 32-byte aligned physical address be operated.
 */
static uint32_t L2CACHE_InvalidateLineByAddr(uint32_t address);

/*!
 * @brief Cleans the Level 2 cache line based on the physical address.
 * This function cleans a cache line based on a physcial address.
 *
 * @param address  The physical addderss of the cache.
 *        The format of the address shall be :
 *        bit 31 ~ bit n+1 | bitn ~ bit5 | bit4 ~ bit0
 *              Tag        |    index    |      0
 *  Note: the physical address shall be aligned to the line size - 32B (256 bit).
 *  so keep the last 5 bits (bit 4 ~ bit 0) of the physical address always be zero.
 *  If the input address is not aligned, it will be changed to 32-byte aligned address.
 *  The n is varies according to the index width.
 * @return The actual 32-byte aligned physical address be operated.
 */
static uint32_t L2CACHE_CleanLineByAddr(uint32_t address);

/*!
 * @brief Cleans and invalidates the Level 2 cache line based on the physical address.
 * This function cleans and invalidates a cache line based on a physcial address.
 *
 * @param address  The physical addderss of the cache.
 *        The format of the address shall be :
 *        bit 31 ~ bit n+1 | bitn ~ bit5 | bit4 ~ bit0
 *              Tag        |    index    |      0
 *  Note: the physical address shall be aligned to the line size - 32B (256 bit).
 *  so keep the last 5 bits (bit 4 ~ bit 0) of the physical address always be zero.
 *  If the input address is not aligned, it will be changed to 32-byte aligned address.
 *  The n is varies according to the index width.
 * @return The actual 32-byte aligned physical address be operated.
 */
static uint32_t L2CACHE_CleanInvalidateLineByAddr(uint32_t address);

/*!
 * @brief Gets the number of the Level 2 cache and the way size.
 * This function cleans and invalidates a cache line based on a physcial address.
 *
 * @param num_ways  The number of the cache way.
 * @param size_way  The way size.
 */
static void L2CACHE_GetWayNumSize(uint32_t *num_ways, uint32_t *size_way);
/*******************************************************************************
 * Code
 ******************************************************************************/
static void L2CACHE_SetAndWaitBackGroundOperate(uint32_t auxCtlReg, uint32_t regAddr)
{
    uint16_t mask = L2CACHE_8WAYS_MASK;
    uint32_t timeout = L2CACHE_OPERATION_TIMEOUT;

    /* Check the ways used at first. */
    if (auxCtlReg & L2CACHEC_REG1_AUX_CONTROL_ASSOCIATIVITY_MASK)
    {
        mask = L2CACHE_16WAYS_MASK;
    }

    /* Set the opeartion for all ways/entries of the cache. */
    *(uint32_t *)regAddr = mask;
    /* Waiting for until the operation is complete. */
    while ((*(volatile uint32_t *)regAddr & mask) && timeout)
    {
        __ASM("nop");
        timeout--;
    }
}

static uint32_t L2CACHE_InvalidateLineByAddr(uint32_t address)
{
    /* Align the address first. */
    address &= ~(uint32_t)(FSL_FEATURE_L2CACHE_LINESIZE_BYTE - 1);
    /* Invalidate the cache line by physical address. */
    L2CACHEC->REG7_INV_PA = address;

    return address;
}

static uint32_t L2CACHE_CleanLineByAddr(uint32_t address)
{
    /* Align the address first. */
    address &= ~(uint32_t)(FSL_FEATURE_L2CACHE_LINESIZE_BYTE - 1);
    /* Invalidate the cache line by physical address. */
    L2CACHEC->REG7_CLEAN_PA = address;

    return address;
}

static uint32_t L2CACHE_CleanInvalidateLineByAddr(uint32_t address)
{
    /* Align the address first. */
    address &= ~(uint32_t)(FSL_FEATURE_L2CACHE_LINESIZE_BYTE - 1);
    /* Clean and invalidate the cache line by physical address. */
    L2CACHEC->REG7_CLEAN_INV_PA = address;

    return address;
}

static void L2CACHE_GetWayNumSize(uint32_t *num_ways, uint32_t *size_way)
{
    assert(num_ways);
    assert(size_way);

    uint32_t number = (L2CACHEC->REG1_AUX_CONTROL & L2CACHEC_REG1_AUX_CONTROL_ASSOCIATIVITY_MASK) >>
                      L2CACHEC_REG1_AUX_CONTROL_ASSOCIATIVITY_SHIFT;
    uint32_t size = (L2CACHEC->REG1_AUX_CONTROL & L2CACHEC_REG1_AUX_CONTROL_WAYSIZE_MASK) >>
                    L2CACHEC_REG1_AUX_CONTROL_WAYSIZE_SHIFT;

    *num_ways = (number + 1) * L2CACHE_SMALLWAYS_NUM;
    if (!size)
    {
        /* 0 internally mapped to the same size as 1 - 16KB.*/
        size += 1;
    }
    *size_way = (1 << (size - 1)) * L2CACHE_SAMLLWAYS_SIZE * L2CACHE_1KBCOVERTOB;
}

void L2CACHE_Init(l2cache_config_t *config)
{
    assert (config);
    
    uint16_t waysNum = 0xFFU; /* Default use the 8-way mask. */
    uint8_t count;
    uint32_t auxReg = 0;

    /*The aux register must be configured when the cachec is disabled
     * So disable first if the cache controller is enabled.
     */
    if (L2CACHEC->REG1_CONTROL & L2CACHEC_REG1_CONTROL_CE_MASK)
    {
        L2CACHE_Disable();
    }    

    /* Unlock all entries. */
    if (L2CACHEC->REG1_AUX_CONTROL & L2CACHEC_REG1_AUX_CONTROL_ASSOCIATIVITY_MASK)
    {
        waysNum = 0xFFFFU;
    }

    for (count = 0; count < L2CACHE_LOCKDOWN_REGNUM; count ++)
    {
        L2CACHE_LockdownByWayEnable(count, waysNum, false);    
    }
    
    /* Set the ways and way-size etc. */
    auxReg = L2CACHEC_REG1_AUX_CONTROL_ASSOCIATIVITY(config->wayNum) |
            L2CACHEC_REG1_AUX_CONTROL_WAYSIZE(config->waySize) | 
            L2CACHEC_REG1_AUX_CONTROL_CRP(config->repacePolicy) |
            L2CACHEC_REG1_AUX_CONTROL_IPE(config->istrPrefetchEnable) |
            L2CACHEC_REG1_AUX_CONTROL_DPE(config->dataPrefetchEnable) |
            L2CACHEC_REG1_AUX_CONTROL_NLE(config->nsLockdownEnable) |
            L2CACHEC_REG1_AUX_CONTROL_FWA(config->writeAlloc) |
            L2CACHEC_REG1_AUX_CONTROL_HPSDRE(config->writeAlloc);
    L2CACHEC->REG1_AUX_CONTROL = auxReg;

    /* Set the tag/data ram latency. */
    if (config->lateConfig)
    {
        uint32_t data = 0;
        /* Tag latency. */
        data = L2CACHEC_REG1_TAG_RAM_CONTROL_SL(config->lateConfig->tagSetupLate)|
            L2CACHEC_REG1_TAG_RAM_CONTROL_SL(config->lateConfig->tagSetupLate)|
            L2CACHEC_REG1_TAG_RAM_CONTROL_RAL(config->lateConfig->tagReadLate)|
            L2CACHEC_REG1_TAG_RAM_CONTROL_WAL(config->lateConfig->dataWriteLate);
        L2CACHEC->REG1_TAG_RAM_CONTROL = data;
        /* Data latency. */
        data = L2CACHEC_REG1_DATA_RAM_CONTROL_SL(config->lateConfig->dataSetupLate)|
            L2CACHEC_REG1_DATA_RAM_CONTROL_SL(config->lateConfig->dataSetupLate)|
            L2CACHEC_REG1_DATA_RAM_CONTROL_RAL(config->lateConfig->dataReadLate)|
            L2CACHEC_REG1_DATA_RAM_CONTROL_WAL(config->lateConfig->dataWriteLate);
        L2CACHEC->REG1_DATA_RAM_CONTROL = data;
    }
}

void L2CACHE_GetDefaultConfig(l2cache_config_t *config)
{
    assert(config);
    uint32_t number = (L2CACHEC->REG1_AUX_CONTROL & L2CACHEC_REG1_AUX_CONTROL_ASSOCIATIVITY_MASK) >>
                      L2CACHEC_REG1_AUX_CONTROL_ASSOCIATIVITY_SHIFT;
    uint32_t size = (L2CACHEC->REG1_AUX_CONTROL & L2CACHEC_REG1_AUX_CONTROL_WAYSIZE_MASK) >>
                    L2CACHEC_REG1_AUX_CONTROL_WAYSIZE_SHIFT;

    /* Get the default value */
    config->wayNum = (l2cache_way_num_t)number;
    config->waySize = (l2cache_way_size)size;
    config->repacePolicy = kL2CACHE_Roundrobin;
    config->lateConfig = NULL;
    config->istrPrefetchEnable = false;
    config->dataPrefetchEnable = false;
    config->nsLockdownEnable = false;
    config->writeAlloc = kL2CACHE_UseAwcache; 
}

void L2CACHE_Enable(void)
{
    /* Invalidate first. */
    L2CACHE_Invalidate();
    /* Enable the level 2 cache controller. */
    L2CACHEC->REG1_CONTROL = L2CACHEC_REG1_CONTROL_CE_MASK;
}

void L2CACHE_Disable(void)
{
    /* First CleanInvalidate all enties in the cache. */
    L2CACHE_CleanInvalidate();
    /* Disable the level 2 cache controller. */
    L2CACHEC->REG1_CONTROL &= ~L2CACHEC_REG1_CONTROL_CE_MASK;
    /* DSB - data sync barrier.*/
    __DSB();
}

void L2CACHE_Invalidate(void)
{
    /* Invalidate all entries in cache. */
    L2CACHE_SetAndWaitBackGroundOperate(L2CACHEC->REG1_AUX_CONTROL, (uint32_t)&L2CACHEC->REG7_INV_WAY);
    /* Cache sync. */
    L2CACHEC->REG7_CACHE_SYNC = 0;
}

void L2CACHE_Clean(void)
{
    /* Clean all entries of the cache. */
    L2CACHE_SetAndWaitBackGroundOperate(L2CACHEC->REG1_AUX_CONTROL, (uint32_t)&L2CACHEC->REG7_CLEAN_WAY);
    /* Cache sync. */
    L2CACHEC->REG7_CACHE_SYNC = 0;
}

void L2CACHE_CleanInvalidate(void)
{
    /* Clean all entries of the cache. */
    L2CACHE_SetAndWaitBackGroundOperate(L2CACHEC->REG1_AUX_CONTROL, (uint32_t)&L2CACHEC->REG7_CLEAN_INV_WAY);
    /* Cache sync. */
    L2CACHEC->REG7_CACHE_SYNC = 0;
}

void L2CACHE_InvalidateByRange(uint32_t address, uint32_t size_byte)
{
    uint32_t endAddr = address + size_byte;

    /* Invalidate addresses in the range. */
    while (address < endAddr)
    {
        address = L2CACHE_InvalidateLineByAddr(address);
        /* Update the size. */
        address += FSL_FEATURE_L2CACHE_LINESIZE_BYTE;
    }

    /* Cache sync. */
    L2CACHEC->REG7_CACHE_SYNC = 0;
}

void L2CACHE_CleanByRange(uint32_t address, uint32_t size_byte)
{
    uint32_t num_ways = 0;
    uint32_t size_way = 0;
    uint32_t endAddr = address + size_byte;

    /* Get the number and size of the cache way. */
    L2CACHE_GetWayNumSize(&num_ways, &size_way);

    /* Check if the clean size is over the cache size. */
    if ((endAddr - address) > num_ways * size_way)
    {
        L2CACHE_Clean();
        return;
    }

    /* Clean addresses in the range. */
    while ((address & ~(uint32_t)(FSL_FEATURE_L2CACHE_LINESIZE_BYTE - 1)) < endAddr)
    {
        /* Clean the address in the range. */
        address = L2CACHE_CleanLineByAddr(address);
        address += FSL_FEATURE_L2CACHE_LINESIZE_BYTE;
    }

    L2CACHEC->REG7_CACHE_SYNC = 0;
}

void L2CACHE_CleanInvalidateByRange(uint32_t address, uint32_t size_byte)
{
    uint32_t num_ways = 0;
    uint32_t size_way = 0;
    uint32_t endAddr = address + size_byte;

    /* Get the number and size of the cache way. */
    L2CACHE_GetWayNumSize(&num_ways, &size_way);

    /* Check if the clean size is over the cache size. */
    if ((endAddr - address) > num_ways * size_way)
    {
        L2CACHE_CleanInvalidate();
        return;
    }

    /* Clean addresses in the range. */
    while ((address & ~(uint32_t)(FSL_FEATURE_L2CACHE_LINESIZE_BYTE - 1)) < endAddr)
    {
        /* Clean the address in the range. */
        address = L2CACHE_CleanInvalidateLineByAddr(address);
        address += FSL_FEATURE_L2CACHE_LINESIZE_BYTE;
    }

    L2CACHEC->REG7_CACHE_SYNC = 0;
}

void L2CACHE_LockdownByWayEnable(uint32_t masterId, uint32_t mask, bool enable)
{
    uint8_t num_ways = (L2CACHEC->REG1_AUX_CONTROL & L2CACHEC_REG1_AUX_CONTROL_ASSOCIATIVITY_MASK) >>
                       L2CACHEC_REG1_AUX_CONTROL_ASSOCIATIVITY_SHIFT;
    num_ways = (num_ways + 1) * L2CACHE_SMALLWAYS_NUM;

    assert(mask < (1U << num_ways));
    assert(masterId < L2CACHE_LOCKDOWN_REGNUM);

    uint32_t dataReg = L2CACHEC->LOCKDOWN[masterId].REG9_D_LOCKDOWN;
    uint32_t istrReg = L2CACHEC->LOCKDOWN[masterId].REG9_I_LOCKDOWN;

    if (enable)
    {
        /* Data lockdown. */
        L2CACHEC->LOCKDOWN[masterId].REG9_D_LOCKDOWN = dataReg | mask;
        /* Instruction lockdown. */
        L2CACHEC->LOCKDOWN[masterId].REG9_I_LOCKDOWN = istrReg | mask;
    }
    else
    {
        /* Data lockdown. */
        L2CACHEC->LOCKDOWN[masterId].REG9_D_LOCKDOWN = dataReg & ~mask;
        /* Instruction lockdown. */
        L2CACHEC->LOCKDOWN[masterId].REG9_I_LOCKDOWN = istrReg & ~mask;
    }
}
#endif  /* FSL_FEATURE_SOC_L2CACHEC_COUNT */

void L1CACHE_InvalidateICacheByRange(uint32_t address, uint32_t size_byte)
{
#if (__DCACHE_PRESENT == 1U)
    uint32_t addr = address & (uint32_t)~(FSL_FEATURE_L1ICACHE_LINESIZE_BYTE - 1);
    int32_t size = size_byte + address - addr;
    uint32_t linesize = 32U;

    __DSB();
    while (size > 0)
    {
        SCB->ICIMVAU = addr;
        addr += linesize;
        size -= linesize;
    }
    __DSB();
    __ISB();
#endif    
}

void ICACHE_InvalidateByRange(uint32_t address, uint32_t size_byte)
{
#if defined(FSL_FEATURE_SOC_L2CACHEC_COUNT) && FSL_FEATURE_SOC_L2CACHEC_COUNT
#if defined(FSL_SDK_DISBLE_L2CACHE_PRESENT) && !FSL_SDK_DISBLE_L2CACHE_PRESENT
    L2CACHE_InvalidateByRange(address, size_byte);
#endif /* !FSL_SDK_DISBLE_L2CACHE_PRESENT */
#endif /* FSL_FEATURE_SOC_L2CACHEC_COUNT */

   L1CACHE_InvalidateICacheByRange(address, size_byte);
}

void DCACHE_InvalidateByRange(uint32_t address, uint32_t size_byte)
{
#if defined(FSL_FEATURE_SOC_L2CACHEC_COUNT) && FSL_FEATURE_SOC_L2CACHEC_COUNT
#if defined(FSL_SDK_DISBLE_L2CACHE_PRESENT) && !FSL_SDK_DISBLE_L2CACHE_PRESENT
    L2CACHE_InvalidateByRange(address, size_byte);
#endif /* !FSL_SDK_DISBLE_L2CACHE_PRESENT */
#endif /* FSL_FEATURE_SOC_L2CACHEC_COUNT */
    L1CACHE_InvalidateDCacheByRange(address, size_byte);
}

void DCACHE_CleanByRange(uint32_t address, uint32_t size_byte)
{
    L1CACHE_CleanDCacheByRange(address, size_byte);
#if defined(FSL_FEATURE_SOC_L2CACHEC_COUNT) && FSL_FEATURE_SOC_L2CACHEC_COUNT
#if defined(FSL_SDK_DISBLE_L2CACHE_PRESENT) && !FSL_SDK_DISBLE_L2CACHE_PRESENT
    L2CACHE_CleanByRange(address, size_byte);
#endif /* !FSL_SDK_DISBLE_L2CACHE_PRESENT */
#endif /* FSL_FEATURE_SOC_L2CACHEC_COUNT */
}

void DCACHE_CleanInvalidateByRange(uint32_t address, uint32_t size_byte)
{
    L1CACHE_CleanInvalidateDCacheByRange(address, size_byte);
#if defined(FSL_FEATURE_SOC_L2CACHEC_COUNT) && FSL_FEATURE_SOC_L2CACHEC_COUNT
#if defined(FSL_SDK_DISBLE_L2CACHE_PRESENT) && !FSL_SDK_DISBLE_L2CACHE_PRESENT
    L2CACHE_CleanInvalidateByRange(address, size_byte);
#endif /* !FSL_SDK_DISBLE_L2CACHE_PRESENT */
#endif /* FSL_FEATURE_SOC_L2CACHEC_COUNT */
}
