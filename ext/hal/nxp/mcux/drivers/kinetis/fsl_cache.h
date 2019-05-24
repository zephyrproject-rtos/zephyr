/*
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_CACHE_H_
#define _FSL_CACHE_H_

#include "fsl_common.h"

/*!
 * @addtogroup cache
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief cache driver version 2.0.1. */
#define FSL_CACHE_DRIVER_VERSION (MAKE_VERSION(2, 0, 1))
/*@}*/

/*! @brief code bus cache line size is equal to system bus line size, so the unified I/D cache line size equals too. */
#define L1CODEBUSCACHE_LINESIZE_BYTE FSL_FEATURE_L1ICACHE_LINESIZE_BYTE /*!< The code bus CACHE line size is 16B = 128b. */
#define L1SYSTEMBUSCACHE_LINESIZE_BYTE L1CODEBUSCACHE_LINESIZE_BYTE /*!< The system bus CACHE line size is 16B = 128b. */

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

#if (FSL_FEATURE_SOC_LMEM_COUNT == 1)
/*!
 * @name cache control for L1 cache (local memory controller for code/system bus cache)
 *@{
 */

/*!
 * @brief Enables the processor code bus cache.
 *
 */
void L1CACHE_EnableCodeCache(void);

/*!
 * @brief Disables the processor code bus cache.
 *
 */
void L1CACHE_DisableCodeCache(void);

/*!
 * @brief Invalidates the processor code bus cache.
 *
 */
void L1CACHE_InvalidateCodeCache(void);

/*!
 * @brief Invalidates processor code bus cache by range.
 *
 * @param address The physical address of cache.
 * @param size_byte size of the memory to be invalidated.
 * @note Address and size should be aligned to "L1CODCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to L1CODEBUSCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void L1CACHE_InvalidateCodeCacheByRange(uint32_t address, uint32_t size_byte);

/*!
 * @brief Cleans the processor code bus cache.
 *
 */
void L1CACHE_CleanCodeCache(void);

/*!
 * @brief Cleans processor code bus cache by range.
 *
 * @param address The physical address of cache.
 * @param size_byte size of the memory to be cleaned.
 * @note Address and size should be aligned to "L1CODEBUSCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to L1CODEBUSCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void L1CACHE_CleanCodeCacheByRange(uint32_t address, uint32_t size_byte);

/*!
 * @brief Cleans and invalidates the processor code bus cache.
 *
 */
void L1CACHE_CleanInvalidateCodeCache(void);

/*!
 * @brief Cleans and invalidate processor code bus cache by range.
 *
 * @param address The physical address of cache.
 * @param size_byte size of the memory to be Cleaned and Invalidated.
 * @note Address and size should be aligned to "L1CODEBUSCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to L1CODEBUSCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void L1CACHE_CleanInvalidateCodeCacheByRange(uint32_t address, uint32_t size_byte);

/*!
 * @brief Enables/disables the processor code bus write buffer.
 *
 * @param enable The enable or disable flag.
 *       true  - enable the code bus write buffer.
 *       false - disable the code bus write buffer.
 */
static inline void L1CACHE_EnableCodeCacheWriteBuffer(bool enable)
{
    if (enable)
    {
        LMEM->PCCCR |= LMEM_PCCCR_ENWRBUF_MASK;
    }
    else
    {
        LMEM->PCCCR &= ~LMEM_PCCCR_ENWRBUF_MASK;
    }
}

#if defined(FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE) && FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE
/*!
 * @brief Enables the processor system bus cache.
 *
 */
void L1CACHE_EnableSystemCache(void);

/*!
 * @brief Disables the processor system bus cache.
 *
 */
void L1CACHE_DisableSystemCache(void);

/*!
 * @brief Invalidates the processor system bus cache.
 *
 */
void L1CACHE_InvalidateSystemCache(void);

/*!
 * @brief Invalidates processor system bus cache by range.
 *
 * @param address The physical address of cache.
 * @param size_byte size of the memory to be invalidated.
 * @note Address and size should be aligned to "L1SYSTEMBUSCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to L1SYSTEMBUSCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void L1CACHE_InvalidateSystemCacheByRange(uint32_t address, uint32_t size_byte);

/*!
 * @brief Cleans the processor system bus cache.
 *
 */
void L1CACHE_CleanSystemCache(void);

/*!
 * @brief Cleans processor system bus cache by range.
 *
 * @param address The physical address of cache.
 * @param size_byte size of the memory to be cleaned.
 * @note Address and size should be aligned to "L1SYSTEMBUSCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to L1SYSTEMBUSCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void L1CACHE_CleanSystemCacheByRange(uint32_t address, uint32_t size_byte);

/*!
 * @brief Cleans and invalidates the processor system bus cache.
 *
 */
void L1CACHE_CleanInvalidateSystemCache(void);

/*!
 * @brief Cleans and Invalidates processor system bus cache by range.
 *
 * @param address The physical address of cache.
 * @param size_byte size of the memory to be Clean and Invalidated.
 * @note Address and size should be aligned to "L1SYSTEMBUSCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to L1SYSTEMBUSCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void L1CACHE_CleanInvalidateSystemCacheByRange(uint32_t address, uint32_t size_byte);

/*!
 * @brief Enables/disables the processor system bus write buffer.
 *
 * @param enable The enable or disable flag.
 *       true  - enable the code bus write buffer.
 *       false - disable the code bus write buffer.
 */
static inline void L1CACHE_EnableSystemCacheWriteBuffer(bool enable)
{
    if (enable)
    {
        LMEM->PSCCR |= LMEM_PSCCR_ENWRBUF_MASK;
    }
    else
    {
        LMEM->PSCCR &= ~LMEM_PSCCR_ENWRBUF_MASK;
    }
}
/*@}*/
#endif /* FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE */

/*!
 * @name cache control for unified L1 cache driver
 *@{
 */

/*!
 * @brief Invalidates cortex-m4 L1 instrument cache by range.
 *
 * @param address  The start address of the memory to be invalidated.
 * @param size_byte  The memory size.
 * @note The start address and size_byte should be 16-Byte(FSL_FEATURE_L1ICACHE_LINESIZE_BYTE) aligned.
 */
void L1CACHE_InvalidateICacheByRange(uint32_t address, uint32_t size_byte);

/*!
 * @brief Invalidates cortex-m4 L1 data cache by range.
 *
 * @param address  The start address of the memory to be invalidated.
 * @param size_byte  The memory size.
 * @note The start address and size_byte should be 16-Byte(FSL_FEATURE_L1DCACHE_LINESIZE_BYTE) aligned.
 */
static inline void L1CACHE_InvalidateDCacheByRange(uint32_t address, uint32_t size_byte)
{
    L1CACHE_InvalidateICacheByRange(address, size_byte);
}

/*!
 * @brief Cleans cortex-m4 L1 data cache by range.
 *
 * @param address  The start address of the memory to be cleaned.
 * @param size_byte  The memory size.
 * @note The start address and size_byte should be 16-Byte(FSL_FEATURE_L1DCACHE_LINESIZE_BYTE) aligned.
 */
void L1CACHE_CleanDCacheByRange(uint32_t address, uint32_t size_byte);

/*!
 * @brief Cleans and Invalidates cortex-m4 L1 data cache by range.
 *
 * @param address  The start address of the memory to be clean and invalidated.
 * @param size_byte  The memory size.
 * @note The start address and size_byte should be 16-Byte(FSL_FEATURE_L1DCACHE_LINESIZE_BYTE) aligned.
 */
void L1CACHE_CleanInvalidateDCacheByRange(uint32_t address, uint32_t size_byte);
/*@}*/
#endif /* FSL_FEATURE_SOC_LMEM_COUNT == 1 */

/*!
 * @name Unified Cache Control for all caches
 *@{
 */

/*!
 * @brief Invalidates instruction cache by range.
 *
 * @param address The physical address.
 * @param size_byte size of the memory to be invalidated.
 * @note Address and size should be aligned to 16-Byte due to the cache operation unit
 * FSL_FEATURE_L1ICACHE_LINESIZE_BYTE. The startAddr here will be forced to align to the cache line
 * size if startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
static inline void ICACHE_InvalidateByRange(uint32_t address, uint32_t size_byte)
{
    L1CACHE_InvalidateICacheByRange(address, size_byte);
}

/*!
 * @brief Invalidates data cache by range.
 *
 * @param address The physical address.
 * @param size_byte size of the memory to be invalidated.
 * @note Address and size should be aligned to 16-Byte due to the cache operation unit
 * FSL_FEATURE_L1DCACHE_LINESIZE_BYTE. The startAddr here will be forced to align to the cache line
 * size if startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
static inline void DCACHE_InvalidateByRange(uint32_t address, uint32_t size_byte)
{
    L1CACHE_InvalidateDCacheByRange(address, size_byte);
}

/*!
 * @brief Clean data cache by range.
 *
 * @param address The physical address.
 * @param size_byte size of the memory to be cleaned.
 * @note Address and size should be aligned to 16-Byte due to the cache operation unit
 * FSL_FEATURE_L1DCACHE_LINESIZE_BYTE. The startAddr here will be forced to align to the cache line
 * size if startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
static inline void DCACHE_CleanByRange(uint32_t address, uint32_t size_byte)
{
    L1CACHE_CleanDCacheByRange(address, size_byte);
}

/*!
 * @brief Cleans and Invalidates data cache by range.
 *
 * @param address The physical address.
 * @param size_byte size of the memory to be Cleaned and Invalidated.
 * @note Address and size should be aligned to 16-Byte due to the cache operation unit
 * FSL_FEATURE_L1DCACHE_LINESIZE_BYTE. The startAddr here will be forced to align to the cache line
 * size if startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
static inline void DCACHE_CleanInvalidateByRange(uint32_t address, uint32_t size_byte)
{
    L1CACHE_CleanInvalidateDCacheByRange(address, size_byte);
}

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_CACHE_H_*/
