/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_LMEM_CACHE_H_
#define _FSL_LMEM_CACHE_H_

#include "fsl_common.h"

/*!
 * @addtogroup lmem_cache
 * @{
 */


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief LMEM controller driver version 2.1.0. */
#define FSL_LMEM_DRIVER_VERSION (MAKE_VERSION(2, 1, 0))
/*@}*/

#define LMEM_CACHE_LINE_SIZE (0x10U)   /*!< Cache line is 16-bytes. */
#define LMEM_CACHE_SIZE_ONEWAY (4096U) /*!< Cache size is 4K-bytes one way. */

/*! @brief LMEM cache mode options. */
typedef enum _lmem_cache_mode
{
    kLMEM_NonCacheable = 0x0U,      /*!< Cache mode: non-cacheable. */
    kLMEM_CacheWriteThrough = 0x2U, /*!< Cache mode: write-through. */
    kLMEM_CacheWriteBack = 0x3U     /*!< Cache mode: write-back. */
} lmem_cache_mode_t;

/*! @brief LMEM cache regions. */
typedef enum _lmem_cache_region
{
    kLMEM_CacheRegion15 = 0U, /*!< Cache Region 15. */
    kLMEM_CacheRegion14,      /*!< Cache Region 14. */
    kLMEM_CacheRegion13,      /*!< Cache Region 13. */
    kLMEM_CacheRegion12,      /*!< Cache Region 12. */
    kLMEM_CacheRegion11,      /*!< Cache Region 11. */
    kLMEM_CacheRegion10,      /*!< Cache Region 10. */
    kLMEM_CacheRegion9,       /*!< Cache Region 9. */
    kLMEM_CacheRegion8,       /*!< Cache Region 8. */
    kLMEM_CacheRegion7,       /*!< Cache Region 7. */
    kLMEM_CacheRegion6,       /*!< Cache Region 6. */
    kLMEM_CacheRegion5,       /*!< Cache Region 5. */
    kLMEM_CacheRegion4,       /*!< Cache Region 4. */
    kLMEM_CacheRegion3,       /*!< Cache Region 3. */
    kLMEM_CacheRegion2,       /*!< Cache Region 2. */
    kLMEM_CacheRegion1,       /*!< Cache Region 1. */
    kLMEM_CacheRegion0        /*!< Cache Region 0. */
} lmem_cache_region_t;

/*! @brief LMEM cache line command. */
typedef enum _lmem_cache_line_command
{
    kLMEM_CacheLineSearchReadOrWrite = 0U, /*!< Cache line search and read or write. */
    kLMEM_CacheLineInvalidate,             /*!< Cache line invalidate. */
    kLMEM_CacheLinePush,                   /*!< Cache line push. */
    kLMEM_CacheLineClear,                  /*!< Cache line clear. */
} lmem_cache_line_command_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Local Memory Processor Code Bus Cache Control
 *@{
 */

/*!
 * @brief Enables/disables the processor code bus cache.
 * This function enables/disables the cache.  The function first invalidates the entire cache
 * and then enables/disables both the cache and write buffers.
 *
 * @param base LMEM peripheral base address.
 * @param enable The enable or disable flag.
 *       true  - enable the code cache.
 *       false - disable the code cache.
 */
void LMEM_EnableCodeCache(LMEM_Type *base, bool enable);

/*!
 * @brief Enables/disables the processor code bus write buffer.
 *
 * @param base LMEM peripheral base address.
 * @param enable The enable or disable flag.
 *       true  - enable the code bus write buffer.
 *       false - disable the code bus write buffer.
 */
static inline void LMEM_EnableCodeWriteBuffer(LMEM_Type *base, bool enable)
{
    if (enable)
    {
        base->PCCCR |= LMEM_PCCCR_ENWRBUF_MASK;
    }
    else
    {
        base->PCCCR &= ~LMEM_PCCCR_ENWRBUF_MASK;
    }
}

/*!
 * @brief Invalidates the processor code bus cache.
 * This function invalidates the cache both ways, which means that
 * it unconditionally clears valid bits and modifies bits of a cache entry.
 *
 * @param base LMEM peripheral base address.
 */
void LMEM_CodeCacheInvalidateAll(LMEM_Type *base);

/*!
 * @brief Pushes all modified lines in the processor code bus cache.
 * This function pushes all modified lines in both ways in the entire cache.
 * It pushes a cache entry if it is valid and modified and clears the modified bit. If
 * the entry is not valid or not modified, leave as is. This action does not clear the valid
 * bit. A cache push is synonymous with a cache flush.
 *
 * @param base LMEM peripheral base address.
 */
void LMEM_CodeCachePushAll(LMEM_Type *base);

/*!
 * @brief Clears the processor code bus cache.
 * This function clears the entire cache and pushes (flushes) and
 * invalidates the operation.
 * Clear - Pushes a cache entry if it is valid and modified, then clears the valid and
 * modified bits. If the entry is not valid or not modified, clear the valid bit.
 *
 * @param base LMEM peripheral base address.
 */
void LMEM_CodeCacheClearAll(LMEM_Type *base);

/*!
 * @brief Invalidates a specific line in the processor code bus cache.
 * This function invalidates a specific line in the cache
 * based on the physical address passed in by the user.
 * Invalidate - Unconditionally clears valid and modified bits of a cache entry.
 *
 * @param base LMEM peripheral base address.
 * @param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 */
void LMEM_CodeCacheInvalidateLine(LMEM_Type *base, uint32_t address);

/*!
 * @brief Invalidates multiple lines in the processor code bus cache.
 * This function invalidates multiple lines in the cache
 * based on the physical address and length in bytes passed in by the
 * user.  If the function detects that the length meets or exceeds half the
 * cache, the function performs an entire cache invalidate function, which is
 * more efficient than invalidating the cache line-by-line.
 * Because the cache consists of two ways and line commands based on the physical address searches both ways,
 * check half the total amount of cache.
 * Invalidate - Unconditionally clear valid and modified bits of a cache entry.
 *
 * @param base LMEM peripheral base address.
 * @param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 * @param length The length in bytes of the total amount of cache lines.
 */
void LMEM_CodeCacheInvalidateMultiLines(LMEM_Type *base, uint32_t address, uint32_t length);

/*!
 * @brief Pushes a specific modified line in the processor code bus cache.
 * This function pushes a specific modified line based on the physical address passed in
 * by the user.
 * Push - Push a cache entry if it is valid and modified, then clear the modified bit. If the
 * entry is not valid or not modified, leave as is. This action does not clear the valid
 * bit. A cache push is synonymous with a cache flush.
 *
 * @param base LMEM peripheral base address.
 * @param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 */
void LMEM_CodeCachePushLine(LMEM_Type *base, uint32_t address);

/*!
 * @brief Pushes multiple modified lines in the processor code bus cache.
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
 * @param base LMEM peripheral base address.
 * @param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 * @param length The length in bytes of the total amount of cache lines.
 */
void LMEM_CodeCachePushMultiLines(LMEM_Type *base, uint32_t address, uint32_t length);

/*!
 * @brief Clears a specific line in the processor code bus cache.
 * This function clears a specific line based on the physical address passed in
 * by the user.
 * Clear - Push a cache entry if it is valid and modified, then clear the valid and
 * modify bits. If entry not valid or not modified, clear the valid bit.
 *
 * @param base LMEM peripheral base address.
 * @param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 */
void LMEM_CodeCacheClearLine(LMEM_Type *base, uint32_t address);

/*!
 * @brief Clears multiple lines in the processor code bus cache.
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
 * @param base LMEM peripheral base address.
 * @param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 * @param length The length in bytes of the total amount of cache lines.
 */
void LMEM_CodeCacheClearMultiLines(LMEM_Type *base, uint32_t address, uint32_t length);

#if (!defined(FSL_FEATURE_LMEM_SUPPORT_ICACHE_DEMOTE_REMOVE)) || !FSL_FEATURE_LMEM_SUPPORT_ICACHE_DEMOTE_REMOVE
/*!
 * @brief Demotes the cache mode of a region in processor code bus cache.
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
 * @param base LMEM peripheral base address.
 * @param region The desired region to demote of type lmem_cache_region_t.
 * @param cacheMode The new, demoted cache mode of type lmem_cache_mode_t.
 * @return The execution result.
 * kStatus_Success The cache demote operation is successful.
 * kStatus_Fail The cache demote operation is failure.
 */
status_t LMEM_CodeCacheDemoteRegion(LMEM_Type *base, lmem_cache_region_t region, lmem_cache_mode_t cacheMode);
#endif  /* FSL_FEATURE_LMEM_SUPPORT_ICACHE_DEMOTE_REMOVE */

/*@}*/

#if FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE
/*!
 * @name Local Memory Processor System Bus Cache Control
 *@{
 */

/*!
 * @brief Enables/disables the processor system bus cache.
 * This function enables/disables the cache. It first invalidates the entire cache,
 * then enables /disable both the cache and write buffer.
 *
 * @param base LMEM peripheral base address.
 * @param The enable or disable flag.
 *       true  - enable the system cache.
 *       false - disable the system cache.
 */
void LMEM_EnableSystemCache(LMEM_Type *base, bool enable);

/*!
 * @brief Enables/disables the processor system bus write buffer.
 *
 * @param base LMEM peripheral base address.
 * @param enable The enable or disable flag.
 *       true  - enable the system bus write buffer.
 *       false - disable the system bus write buffer.
 */
static inline void LMEM_EnableSystemWriteBuffer(LMEM_Type *base, bool enable)
{
    if (enable)
    {
        base->PSCCR |= LMEM_PSCCR_ENWRBUF_MASK;       
    }
    else
    {
        base->PSCCR &= ~LMEM_PSCCR_ENWRBUF_MASK;               
    }
}

/*!
 * @brief Invalidates the processor system bus cache.
 * This function invalidates the entire cache both ways.
 * Invalidate - Unconditionally clear valid and modify bits of a cache entry
 *
 * @param base LMEM peripheral base address.
 */
void LMEM_SystemCacheInvalidateAll(LMEM_Type *base);

/*!
 * @brief Pushes all modified lines in the  processor system bus cache.
 * This function pushes all modified lines in both ways (the entire cache).
 * Push - Push a cache entry if it is valid and modified, then clear the modify bit. If
 * the entry is not valid or not modified, leave as is. This action does not clear the valid
 * bit. A cache push is synonymous with a cache flush.
 *
 * @param base LMEM peripheral base address.
 */
void LMEM_SystemCachePushAll(LMEM_Type *base);

/*!
 * @brief Clears the entire processor system bus cache.
 * This function clears the entire cache, which is a push (flush) and
 * invalidate operation.
 * Clear - Push a cache entry if it is valid and modified, then clear the valid and
 * modify bits. If the entry is not valid or not modified, clear the valid bit.
 *
 * @param base LMEM peripheral base address.
 */
void LMEM_SystemCacheClearAll(LMEM_Type *base);

/*!
 * @brief Invalidates a specific line in the processor system bus cache.
 * This function invalidates a specific line in the cache
 * based on the physical address passed in by the user.
 * Invalidate - Unconditionally clears valid and modify bits of a cache entry.
 *
 * @param base LMEM peripheral base address. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 * @param address The physical address of the cache line.
 */
void LMEM_SystemCacheInvalidateLine(LMEM_Type *base, uint32_t address);

/*!
 * @brief Invalidates multiple lines in the processor system bus cache.
 * This function invalidates multiple lines in the cache
 * based on the physical address and length in bytes passed in by the
 * user.  If the function detects that the length meets or exceeds half of the
 * cache, the function performs an entire cache invalidate function (which is
 * more efficient than invalidating the cache line-by-line).
 * Because the cache consists of two ways and line commands based on the physical address searches both ways,
 * check half the total amount of cache.
 * Invalidate - Unconditionally clear valid and modify bits of a cache entry
 *
 * @param base LMEM peripheral base address.
 * @param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 * @param length The length in bytes of the total amount of cache lines.
 */
void LMEM_SystemCacheInvalidateMultiLines(LMEM_Type *base, uint32_t address, uint32_t length);

/*!
 * @brief Pushes a specific modified line in the processor system bus cache.
 * This function pushes a specific modified line based on the physical address passed in
 * by the user.
 * Push - Push a cache entry if it is valid and modified, then clear the modify bit. If
 * the entry is not valid or not modified, leave as is. This action does not clear the valid
 * bit. A cache push is synonymous with a cache flush.
 *
 * @param base LMEM peripheral base address.
 * @param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 */
void LMEM_SystemCachePushLine(LMEM_Type *base, uint32_t address);

/*!
 * @brief Pushes multiple modified lines in the processor system bus cache.
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
 * @param base LMEM peripheral base address.
 * @param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 * @param length The length in bytes of the total amount of cache lines.
 */
void LMEM_SystemCachePushMultiLines(LMEM_Type *base, uint32_t address, uint32_t length);

/*!
 * @brief Clears a specific line in the processor system bus cache.
 * This function clears a specific line based on the physical address passed in
 * by the user.
 * Clear - Push a cache entry if it is valid and modified, then clear the valid and
 * modify bits. If the entry is not valid or not modified, clear the valid bit.
 *
 * @param base LMEM peripheral base address.
 * @param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 */
void LMEM_SystemCacheClearLine(LMEM_Type *base, uint32_t address);

/*!
 * @brief Clears multiple lines in the processor system bus cache.
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
 * @param base LMEM peripheral base address.
 * @param address The physical address of the cache line. Should be 16-byte aligned address.
 * If not, it is changed to the 16-byte aligned memory address.
 * @param length The length in bytes of the total amount of cache lines.
 */
void LMEM_SystemCacheClearMultiLines(LMEM_Type *base, uint32_t address, uint32_t length);

/*!
 * @brief Demotes the cache mode of a region in the processor system bus cache.
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
 * @param base LMEM peripheral base address.
 * @param region The desired region to demote of type lmem_cache_region_t.
 * @param cacheMode The new, demoted cache mode of type lmem_cache_mode_t.
 * @return The execution result.
 * kStatus_Success The cache demote operation is successful.
 * kStatus_Fail The cache demote operation is failure.
 */
status_t LMEM_SystemCacheDemoteRegion(LMEM_Type *base, lmem_cache_region_t region, lmem_cache_mode_t cacheMode);

/*@}*/
#endif /* FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_LMEM_CACHE_H_*/
