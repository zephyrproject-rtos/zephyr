/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_lmem_cache.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.lmem"
#endif

#define LMEM_CACHEMODE_WIDTH (2U)
#define LMEM_CACHEMODE_MASK_UNIT (0x3U)

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * brief Enables/disables the processor code bus cache.
 * This function enables/disables the cache.  The function first invalidates the entire cache
 * and then enables/disables both the cache and write buffers.
 *
 * param base LMEM peripheral base address.
 * param enable The enable or disable flag.
 *       true  - enable the code cache.
 *       false - disable the code cache.
 */
void LMEM_EnableCodeCache(LMEM_Type *base, bool enable)
{
    if (enable)
    {
        /* First, invalidate the entire cache. */
        LMEM_CodeCacheInvalidateAll(base);

        /* Now enable the cache. */
        base->PCCCR |= LMEM_PCCCR_ENCACHE_MASK;
    }
    else
    {
        /* First, push any modified contents. */
        LMEM_CodeCachePushAll(base);

        /* Now disable the cache. */
        base->PCCCR &= ~LMEM_PCCCR_ENCACHE_MASK;
    }
}

/*!
 * brief Invalidates the processor code bus cache.
 * This function invalidates the cache both ways, which means that
 * it unconditionally clears valid bits and modifies bits of a cache entry.
 *
 * param base LMEM peripheral base address.
 */
void LMEM_CodeCacheInvalidateAll(LMEM_Type *base)
{
    /* Enables the processor code bus to invalidate all lines in both ways.
    and Initiate the processor code bus code cache command. */
    base->PCCCR |= LMEM_PCCCR_INVW0_MASK | LMEM_PCCCR_INVW1_MASK | LMEM_PCCCR_GO_MASK;

    /* Wait until the cache command completes. */
    while (base->PCCCR & LMEM_PCCCR_GO_MASK)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    base->PCCCR &= ~(LMEM_PCCCR_INVW0_MASK | LMEM_PCCCR_INVW1_MASK);
}

/*!
 * brief Pushes all modified lines in the processor code bus cache.
 * This function pushes all modified lines in both ways in the entire cache.
 * It pushes a cache entry if it is valid and modified and clears the modified bit. If
 * the entry is not valid or not modified, leave as is. This action does not clear the valid
 * bit. A cache push is synonymous with a cache flush.
 *
 * param base LMEM peripheral base address.
 */
void LMEM_CodeCachePushAll(LMEM_Type *base)
{
    /* Enable the processor code bus to push all modified lines. */
    base->PCCCR |= LMEM_PCCCR_PUSHW0_MASK | LMEM_PCCCR_PUSHW1_MASK | LMEM_PCCCR_GO_MASK;

    /* Wait until the cache command completes. */
    while (base->PCCCR & LMEM_PCCCR_GO_MASK)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    base->PCCCR &= ~(LMEM_PCCCR_PUSHW0_MASK | LMEM_PCCCR_PUSHW1_MASK);
}

/*!
 * brief Clears the processor code bus cache.
 * This function clears the entire cache and pushes (flushes) and
 * invalidates the operation.
 * Clear - Pushes a cache entry if it is valid and modified, then clears the valid and
 * modified bits. If the entry is not valid or not modified, clear the valid bit.
 *
 * param base LMEM peripheral base address.
 */
void LMEM_CodeCacheClearAll(LMEM_Type *base)
{
    /* Push and invalidate all. */
    base->PCCCR |= LMEM_PCCCR_PUSHW0_MASK | LMEM_PCCCR_PUSHW1_MASK | LMEM_PCCCR_INVW0_MASK | LMEM_PCCCR_INVW1_MASK |
                   LMEM_PCCCR_GO_MASK;

    /* Wait until the cache command completes. */
    while (base->PCCCR & LMEM_PCCCR_GO_MASK)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    base->PCCCR &= ~(LMEM_PCCCR_PUSHW0_MASK | LMEM_PCCCR_PUSHW1_MASK | LMEM_PCCCR_INVW0_MASK | LMEM_PCCCR_INVW1_MASK);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_CodeCacheInvalidateLine
 * Description   : This function invalidates a specific line in the Processor Code bus cache.
 *
 * This function invalidates a specific line in the cache. The function invalidates a
 * line in cache based on the physical address passed in by the user.
 * Invalidate - Unconditionally clear valid and modify bits of a cache entry
 *
 *END**************************************************************************/
/*!
 * brief Invalidates a specific line in the processor code bus cache.
 * This function invalidates a specific line in the cache
 * based on the physical address passed in by the user.
 * Invalidate - Unconditionally clears valid and modified bits of a cache entry.
 *
 * param base LMEM peripheral base address.
 * param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 */
void LMEM_CodeCacheInvalidateLine(LMEM_Type *base, uint32_t address)
{
    uint32_t pccReg = 0;

    /* Set the invalidate by line command and use the physical address. */
    pccReg =
        (base->PCCLCR & ~LMEM_PCCLCR_LCMD_MASK) | LMEM_PCCLCR_LCMD(kLMEM_CacheLineInvalidate) | LMEM_PCCLCR_LADSEL_MASK;
    base->PCCLCR = pccReg;

    /* Set the address and initiate the command. */
    base->PCCSAR = (address & LMEM_PCCSAR_PHYADDR_MASK) | LMEM_PCCSAR_LGO_MASK;

    /* Wait until the cache command completes. */
    while (base->PCCSAR & LMEM_PCCSAR_LGO_MASK)
    {
    }

    /* No need to clear this command since future line commands will overwrite
    the line command field. */
}

/*!
 * brief Invalidates multiple lines in the processor code bus cache.
 * This function invalidates multiple lines in the cache
 * based on the physical address and length in bytes passed in by the
 * user.  If the function detects that the length meets or exceeds half the
 * cache, the function performs an entire cache invalidate function, which is
 * more efficient than invalidating the cache line-by-line.
 * Because the cache consists of two ways and line commands based on the physical address searches both ways,
 * check half the total amount of cache.
 * Invalidate - Unconditionally clear valid and modified bits of a cache entry.
 *
 * param base LMEM peripheral base address.
 * param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 * param length The length in bytes of the total amount of cache lines.
 */
void LMEM_CodeCacheInvalidateMultiLines(LMEM_Type *base, uint32_t address, uint32_t length)
{
    uint32_t endAddr = address + length;
    /* Align address to cache line size. */
    address = address & ~(LMEM_CACHE_LINE_SIZE - 1U);
    /* If the length exceeds 4KB, invalidate all. */
    if (length >= LMEM_CACHE_SIZE_ONEWAY)
    {
        LMEM_CodeCacheInvalidateAll(base);
    }
    else
    { /* Proceed with multi-line invalidate. */
        while (address < endAddr)
        {
            LMEM_CodeCacheInvalidateLine(base, address);
            address = address + LMEM_CACHE_LINE_SIZE;
        }
    }
}

/*!
 * brief Pushes a specific modified line in the processor code bus cache.
 * This function pushes a specific modified line based on the physical address passed in
 * by the user.
 * Push - Push a cache entry if it is valid and modified, then clear the modified bit. If the
 * entry is not valid or not modified, leave as is. This action does not clear the valid
 * bit. A cache push is synonymous with a cache flush.
 *
 * param base LMEM peripheral base address.
 * param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 */
void LMEM_CodeCachePushLine(LMEM_Type *base, uint32_t address)
{
    uint32_t pccReg = 0;

    /* Set the push by line command. */
    pccReg = (base->PCCLCR & ~LMEM_PCCLCR_LCMD_MASK) | LMEM_PCCLCR_LCMD(kLMEM_CacheLinePush) | LMEM_PCCLCR_LADSEL_MASK;
    base->PCCLCR = pccReg;

    /* Set the address and initiate the command. */
    base->PCCSAR = (address & LMEM_PCCSAR_PHYADDR_MASK) | LMEM_PCCSAR_LGO_MASK;

    /* Wait until the cache command completes. */
    while (base->PCCSAR & LMEM_PCCSAR_LGO_MASK)
    {
    }

    /* No need to clear this command since future line commands will overwrite
     the line command field. */
}

/*!
 * brief Pushes multiple modified lines in the processor code bus cache.
 * This function pushes multiple modified lines in the cache
 * based on the physical address and length in bytes passed in by the
 * user.  If the function detects that the length meets or exceeds half of the
 * cache, the function performs an cache push function, which is
 * more efficient than pushing the modified lines in the cache line-by-line.
 * Because the cache consists of two ways and line commands based on the physical address searches both ways,
 * check half the total amount of cache.
 * Push - Push a cache entry if it is valid and modified, then clear the modified bit. If
 * the entry is not valid or not modified, leave as is. This action does not clear the valid
 * bit. A cache push is synonymous with a cache flush.
 *
 * param base LMEM peripheral base address.
 * param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 * param length The length in bytes of the total amount of cache lines.
 */
void LMEM_CodeCachePushMultiLines(LMEM_Type *base, uint32_t address, uint32_t length)
{
    uint32_t endAddr = address + length;
    /* Align address to cache line size. */
    address = address & ~(LMEM_CACHE_LINE_SIZE - 1U);

    /* If the length exceeds 4KB, push all. */
    if (length >= LMEM_CACHE_SIZE_ONEWAY)
    {
        LMEM_CodeCachePushAll(base);
    }
    else
    { /* Proceed with multi-line push. */
        while (address < endAddr)
        {
            LMEM_CodeCachePushLine(base, address);
            address = address + LMEM_CACHE_LINE_SIZE;
        }
    }
}

/*!
 * brief Clears a specific line in the processor code bus cache.
 * This function clears a specific line based on the physical address passed in
 * by the user.
 * Clear - Push a cache entry if it is valid and modified, then clear the valid and
 * modify bits. If entry not valid or not modified, clear the valid bit.
 *
 * param base LMEM peripheral base address.
 * param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 */
void LMEM_CodeCacheClearLine(LMEM_Type *base, uint32_t address)
{
    uint32_t pccReg = 0;

    /* Set the push by line command. */
    pccReg = (base->PCCLCR & ~LMEM_PCCLCR_LCMD_MASK) | LMEM_PCCLCR_LCMD(kLMEM_CacheLineClear) | LMEM_PCCLCR_LADSEL_MASK;
    base->PCCLCR = pccReg;

    /* Set the address and initiate the command. */
    base->PCCSAR = (address & LMEM_PCCSAR_PHYADDR_MASK) | LMEM_PCCSAR_LGO_MASK;

    /* Wait until the cache command completes. */
    while (base->PCCSAR & LMEM_PCCSAR_LGO_MASK)
    {
    }

    /* No need to clear this command since future line commands will overwrite
       the line command field. */
}

/*!
 * brief Clears multiple lines in the processor code bus cache.
 * This function clears multiple lines in the cache
 * based on the physical address and length in bytes passed in by the
 * user.  If the function detects that the length meets or exceeds half the total amount of
 * cache, the function performs a cache clear function which is
 * more efficient than clearing the lines in the cache line-by-line.
 * Because the cache consists of two ways and line commands based on the physical address searches both ways,
 * check half the total amount of cache.
 * Clear - Push a cache entry if it is valid and modified, then clear the valid and
 * modify bits. If entry not valid or not modified, clear the valid bit.
 *
 * param base LMEM peripheral base address.
 * param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 * param length The length in bytes of the total amount of cache lines.
 */
void LMEM_CodeCacheClearMultiLines(LMEM_Type *base, uint32_t address, uint32_t length)
{
    uint32_t endAddr = address + length;
    /* Align address to cache line size. */
    address = address & ~(LMEM_CACHE_LINE_SIZE - 1U);

    /* If the length exceeds 4KB, clear all. */
    if (length >= LMEM_CACHE_SIZE_ONEWAY)
    {
        LMEM_CodeCacheClearAll(base);
    }
    else /* Proceed with multi-line clear. */
    {
        while (address < endAddr)
        {
            LMEM_CodeCacheClearLine(base, address);
            address = address + LMEM_CACHE_LINE_SIZE;
        }
    }
}
#if (!defined(FSL_FEATURE_LMEM_SUPPORT_ICACHE_DEMOTE_REMOVE)) || !FSL_FEATURE_LMEM_SUPPORT_ICACHE_DEMOTE_REMOVE
/*!
 * brief Demotes the cache mode of a region in processor code bus cache.
 * This function allows the user to demote the cache mode of a region within the device's
 * memory map. Demoting the cache mode reduces the cache function applied to a memory
 * region from write-back to write-through to non-cacheable.  The function checks to see
 * if the requested cache mode is higher than or equal to the current cache mode, and if
 * so, returns an error. After a region is demoted, its cache mode can only be raised
 * by a reset, which returns it to its default state which is the highest cache configure for
 * each region.
 * To maintain cache coherency, changes to the cache mode should be completed while the
 * address space being changed is not being accessed or the cache is disabled. Before a
 * cache mode change, this function completes a cache clear all command to push and invalidate any
 * cache entries that may have changed.
 *
 * param base LMEM peripheral base address.
 * param region The desired region to demote of type lmem_cache_region_t.
 * param cacheMode The new, demoted cache mode of type lmem_cache_mode_t.
 * return The execution result.
 * kStatus_Success The cache demote operation is successful.
 * kStatus_Fail The cache demote operation is failure.
 */
status_t LMEM_CodeCacheDemoteRegion(LMEM_Type *base, lmem_cache_region_t region, lmem_cache_mode_t cacheMode)
{
    uint32_t mode = base->PCCRMR;
    uint32_t shift = LMEM_CACHEMODE_WIDTH * (uint32_t)region; /* Region shift. */
    uint32_t mask = LMEM_CACHEMODE_MASK_UNIT << shift;        /* Region mask. */

    /* If the current cache mode is higher than the requested mode, return error. */
    if ((uint32_t)cacheMode >= ((mode & mask) >> shift))
    {
        return kStatus_Fail;
    }
    else
    { /* Proceed to demote the region. */
        LMEM_CodeCacheClearAll(base);
        base->PCCRMR = (mode & ~mask) | cacheMode << shift;
        return kStatus_Success;
    }
}
#endif /* FSL_FEATURE_LMEM_SUPPORT_ICACHE_DEMOTE_REMOVE */

#if FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE
/*!
 * brief Enables/disables the processor system bus cache.
 * This function enables/disables the cache. It first invalidates the entire cache,
 * then enables /disable both the cache and write buffer.
 *
 * param base LMEM peripheral base address.
 * param The enable or disable flag.
 *       true  - enable the system cache.
 *       false - disable the system cache.
 */
void LMEM_EnableSystemCache(LMEM_Type *base, bool enable)
{
    if (enable)
    {
        /* First, invalidate the entire cache. */
        LMEM_SystemCacheInvalidateAll(base);

        /* Now enable the cache. */
        base->PSCCR |= LMEM_PSCCR_ENCACHE_MASK;
    }
    else
    {
        /* First, push any modified contents. */
        LMEM_SystemCachePushAll(base);

        /* Now disable the cache. */
        base->PSCCR &= ~LMEM_PSCCR_ENCACHE_MASK;
    }
}

/*!
 * brief Invalidates the processor system bus cache.
 * This function invalidates the entire cache both ways.
 * Invalidate - Unconditionally clear valid and modify bits of a cache entry
 *
 * param base LMEM peripheral base address.
 */
void LMEM_SystemCacheInvalidateAll(LMEM_Type *base)
{
    /* Enables the processor system bus to invalidate all lines in both ways.
    and Initiate the processor system bus cache command. */
    base->PSCCR |= LMEM_PSCCR_INVW0_MASK | LMEM_PSCCR_INVW1_MASK | LMEM_PSCCR_GO_MASK;

    /* Wait until the cache command completes */
    while (base->PSCCR & LMEM_PSCCR_GO_MASK)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    base->PSCCR &= ~(LMEM_PSCCR_INVW0_MASK | LMEM_PSCCR_INVW1_MASK);
}

/*!
 * brief Pushes all modified lines in the  processor system bus cache.
 * This function pushes all modified lines in both ways (the entire cache).
 * Push - Push a cache entry if it is valid and modified, then clear the modify bit. If
 * the entry is not valid or not modified, leave as is. This action does not clear the valid
 * bit. A cache push is synonymous with a cache flush.
 *
 * param base LMEM peripheral base address.
 */
void LMEM_SystemCachePushAll(LMEM_Type *base)
{
    /* Enable the processor system bus to push all modified lines. */
    base->PSCCR |= LMEM_PSCCR_PUSHW0_MASK | LMEM_PSCCR_PUSHW1_MASK | LMEM_PSCCR_GO_MASK;

    /* Wait until the cache command completes. */
    while (base->PSCCR & LMEM_PSCCR_GO_MASK)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    base->PSCCR &= ~(LMEM_PSCCR_PUSHW0_MASK | LMEM_PSCCR_PUSHW1_MASK);
}

/*!
 * brief Clears the entire processor system bus cache.
 * This function clears the entire cache, which is a push (flush) and
 * invalidate operation.
 * Clear - Push a cache entry if it is valid and modified, then clear the valid and
 * modify bits. If the entry is not valid or not modified, clear the valid bit.
 *
 * param base LMEM peripheral base address.
 */
void LMEM_SystemCacheClearAll(LMEM_Type *base)
{
    /* Push and invalidate all. */
    base->PSCCR |= LMEM_PSCCR_PUSHW0_MASK | LMEM_PSCCR_PUSHW1_MASK | LMEM_PSCCR_INVW0_MASK | LMEM_PSCCR_INVW1_MASK |
                   LMEM_PSCCR_GO_MASK;

    /* Wait until the cache command completes. */
    while (base->PSCCR & LMEM_PSCCR_GO_MASK)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    base->PSCCR &= ~(LMEM_PSCCR_PUSHW0_MASK | LMEM_PSCCR_PUSHW1_MASK | LMEM_PSCCR_INVW0_MASK | LMEM_PSCCR_INVW1_MASK);
}

/*!
 * brief Invalidates a specific line in the processor system bus cache.
 * This function invalidates a specific line in the cache
 * based on the physical address passed in by the user.
 * Invalidate - Unconditionally clears valid and modify bits of a cache entry.
 *
 * param base LMEM peripheral base address. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 * param address The physical address of the cache line.
 */
void LMEM_SystemCacheInvalidateLine(LMEM_Type *base, uint32_t address)
{
    uint32_t pscReg = 0;

    /* Set the invalidate by line command and use the physical address. */
    pscReg =
        (base->PSCLCR & ~LMEM_PSCLCR_LCMD_MASK) | LMEM_PSCLCR_LCMD(kLMEM_CacheLineInvalidate) | LMEM_PSCLCR_LADSEL_MASK;
    base->PSCLCR = pscReg;

    /* Set the address and initiate the command. */
    base->PSCSAR = (address & LMEM_PSCSAR_PHYADDR_MASK) | LMEM_PSCSAR_LGO_MASK;

    /* Wait until the cache command completes. */
    while (base->PSCSAR & LMEM_PSCSAR_LGO_MASK)
    {
    }

    /* No need to clear this command since future line commands will overwrite
      the line command field. */
}

/*!
 * brief Invalidates multiple lines in the processor system bus cache.
 * This function invalidates multiple lines in the cache
 * based on the physical address and length in bytes passed in by the
 * user.  If the function detects that the length meets or exceeds half of the
 * cache, the function performs an entire cache invalidate function (which is
 * more efficient than invalidating the cache line-by-line).
 * Because the cache consists of two ways and line commands based on the physical address searches both ways,
 * check half the total amount of cache.
 * Invalidate - Unconditionally clear valid and modify bits of a cache entry
 *
 * param base LMEM peripheral base address.
 * param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 * param length The length in bytes of the total amount of cache lines.
 */
void LMEM_SystemCacheInvalidateMultiLines(LMEM_Type *base, uint32_t address, uint32_t length)
{
    uint32_t endAddr = address + length;
    address = address & ~(LMEM_CACHE_LINE_SIZE - 1U); /* Align address to cache line size */

    /* If the length exceeds 4KB, invalidate all. */
    if (length >= LMEM_CACHE_SIZE_ONEWAY)
    {
        LMEM_SystemCacheInvalidateAll(base);
    }
    else /* Proceed with multi-line invalidate. */
    {
        while (address < endAddr)
        {
            LMEM_SystemCacheInvalidateLine(base, address);
            address = address + LMEM_CACHE_LINE_SIZE;
        }
    }
}

/*!
 * brief Pushes a specific modified line in the processor system bus cache.
 * This function pushes a specific modified line based on the physical address passed in
 * by the user.
 * Push - Push a cache entry if it is valid and modified, then clear the modify bit. If
 * the entry is not valid or not modified, leave as is. This action does not clear the valid
 * bit. A cache push is synonymous with a cache flush.
 *
 * param base LMEM peripheral base address.
 * param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 */
void LMEM_SystemCachePushLine(LMEM_Type *base, uint32_t address)
{
    uint32_t pscReg = 0;

    /* Set the push by line command. */
    pscReg = (base->PSCLCR & ~LMEM_PSCLCR_LCMD_MASK) | LMEM_PSCLCR_LCMD(kLMEM_CacheLinePush) | LMEM_PSCLCR_LADSEL_MASK;
    base->PSCLCR = pscReg;

    /* Set the address and initiate the command. */
    base->PSCSAR = (address & LMEM_PSCSAR_PHYADDR_MASK) | LMEM_PSCSAR_LGO_MASK;

    /* Wait until the cache command completes. */
    while (base->PSCSAR & LMEM_PSCSAR_LGO_MASK)
    {
    }

    /* No need to clear this command since future line commands will overwrite
     the line command field. */
}

/*!
 * brief Pushes multiple modified lines in the processor system bus cache.
 * This function pushes multiple modified lines in the cache
 * based on the physical address and length in bytes passed in by the
 * user.  If the function detects that the length meets or exceeds half of the
 * cache, the function performs an entire cache push function (which is
 * more efficient than pushing the modified lines in the cache line-by-line).
 * Because the cache consists of two ways and line commands based on the physical address searches both ways,
 * check half the total amount of cache.
 * Push - Push a cache entry if it is valid and modified, then clear the modify bit. If
 * the entry is not valid or not modified, leave as is. This action does not clear the valid
 * bit. A cache push is synonymous with a cache flush.
 *
 * param base LMEM peripheral base address.
 * param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 * param length The length in bytes of the total amount of cache lines.
 */
void LMEM_SystemCachePushMultiLines(LMEM_Type *base, uint32_t address, uint32_t length)
{
    uint32_t endAddr = address + length;
    address = address & ~(LMEM_CACHE_LINE_SIZE - 1U); /* Align address to cache line size. */

    /* If the length exceeds 4KB, push all. */
    if (length >= LMEM_CACHE_SIZE_ONEWAY)
    {
        LMEM_SystemCachePushAll(base);
    }
    else
    { /* Proceed with multi-line push. */
        while (address < endAddr)
        {
            LMEM_SystemCachePushLine(base, address);
            address = address + LMEM_CACHE_LINE_SIZE;
        }
    }
}

/*!
 * brief Clears a specific line in the processor system bus cache.
 * This function clears a specific line based on the physical address passed in
 * by the user.
 * Clear - Push a cache entry if it is valid and modified, then clear the valid and
 * modify bits. If the entry is not valid or not modified, clear the valid bit.
 *
 * param base LMEM peripheral base address.
 * param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 */
void LMEM_SystemCacheClearLine(LMEM_Type *base, uint32_t address)
{
    uint32_t pscReg = 0;

    /* Set the push by line command. */
    pscReg = (base->PSCLCR & ~LMEM_PSCLCR_LCMD_MASK) | LMEM_PSCLCR_LCMD(kLMEM_CacheLineClear) | LMEM_PSCLCR_LADSEL_MASK;
    base->PSCLCR = pscReg;

    /* Set the address and initiate the command. */
    base->PSCSAR = (address & LMEM_PSCSAR_PHYADDR_MASK) | LMEM_PSCSAR_LGO_MASK;

    /* Wait until the cache command completes. */
    while (base->PSCSAR & LMEM_PSCSAR_LGO_MASK)
    {
    }

    /* No need to clear this command since future line commands will overwrite
     the line command field. */
}

/*!
 * brief Clears multiple lines in the processor system bus cache.
 * This function clears multiple lines in the cache
 * based on the physical address and length in bytes passed in by the
 * user.  If the function detects that the length meets or exceeds half of the
 * cache, the function performs an entire cache clear function (which is
 * more efficient than clearing the lines in the cache line-by-line).
 * Because the cache consists of two ways and line commands based on the physical address searches both ways,
 * check half the total amount of cache.
 * Clear - Push a cache entry if it is valid and modified, then clear the valid and
 * modify bits. If the entry is not valid or not modified, clear the valid bit.
 *
 * param base LMEM peripheral base address.
 * param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 * param length The length in bytes of the total amount of cache lines.
 */
void LMEM_SystemCacheClearMultiLines(LMEM_Type *base, uint32_t address, uint32_t length)
{
    uint32_t endAddr = address + length;
    address = address & ~(LMEM_CACHE_LINE_SIZE - 1U); /* Align address to cache line size. */

    /* If the length exceeds 4KB, clear all. */
    if (length >= LMEM_CACHE_SIZE_ONEWAY)
    {
        LMEM_SystemCacheClearAll(base);
    }
    else /* Proceed with multi-line clear. */
    {
        while (address < endAddr)
        {
            LMEM_SystemCacheClearLine(base, address);
            address = address + LMEM_CACHE_LINE_SIZE;
        }
    }
}

/*!
 * brief Demotes the cache mode of a region in the processor system bus cache.
 * This function allows the user to demote the cache mode of a region within the device's
 * memory map. Demoting the cache mode reduces the cache function applied to a memory
 * region from write-back to write-through to non-cacheable.  The function checks to see
 * if the requested cache mode is higher than or equal to the current cache mode, and if
 * so, returns an error. After a region is demoted, its cache mode can only be raised
 * by a reset, which returns it to its default state which is the highest cache configure
 * for each region.
 * To maintain cache coherency, changes to the cache mode should be completed while the
 * address space being changed is not being accessed or the cache is disabled. Before a
 * cache mode change, this function completes a cache clear all command to push and invalidate any
 * cache entries that may have changed.
 *
 * param base LMEM peripheral base address.
 * param region The desired region to demote of type lmem_cache_region_t.
 * param cacheMode The new, demoted cache mode of type lmem_cache_mode_t.
 * return The execution result.
 * kStatus_Success The cache demote operation is successful.
 * kStatus_Fail The cache demote operation is failure.
 */
status_t LMEM_SystemCacheDemoteRegion(LMEM_Type *base, lmem_cache_region_t region, lmem_cache_mode_t cacheMode)
{
    uint32_t mode = base->PSCRMR;
    uint32_t shift = LMEM_CACHEMODE_WIDTH * (uint32_t)region; /* Region shift. */
    uint32_t mask = LMEM_CACHEMODE_MASK_UNIT << shift;        /* Region mask. */

    /* If the current cache mode is higher than the requested mode, return error. */
    if ((uint32_t)cacheMode >= ((mode & mask) >> shift))
    {
        return kStatus_Fail;
    }
    else
    { /* Proceed to demote the region. */
        LMEM_SystemCacheClearAll(base);
        base->PSCRMR = (mode & ~mask) | (cacheMode << shift);
        return kStatus_Success;
    }
}
#endif /* FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE */
