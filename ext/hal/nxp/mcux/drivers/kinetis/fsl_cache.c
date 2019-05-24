/*
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
#define FSL_COMPONENT_ID "platform.drivers.cache_lmem"
#endif

#define L1CACHE_ONEWAYSIZE_BYTE (4096U)           /*!< Cache size is 4K-bytes one way. */
#define L1CACHE_CODEBUSADDR_BOUNDARY (0x1FFFFFFF) /*!< The processor code bus address boundary. */

/*******************************************************************************
 * Code
 ******************************************************************************/
#if (FSL_FEATURE_SOC_LMEM_COUNT == 1)
/*!
 * brief Enables the processor code bus cache.
 *
 */
void L1CACHE_EnableCodeCache(void)
{
    /* First, invalidate the entire cache. */
    L1CACHE_InvalidateCodeCache();

    /* Now enable the cache. */
    LMEM->PCCCR |= LMEM_PCCCR_ENCACHE_MASK;
}

/*!
 * brief Disables the processor code bus cache.
 *
 */
void L1CACHE_DisableCodeCache(void)
{
    /* First, push any modified contents. */
    L1CACHE_CleanCodeCache();

    /* Now disable the cache. */
    LMEM->PCCCR &= ~LMEM_PCCCR_ENCACHE_MASK;
}

/*!
 * brief Invalidates the processor code bus cache.
 *
 */
void L1CACHE_InvalidateCodeCache(void)
{
    /* Enables the processor code bus to invalidate all lines in both ways.
    and Initiate the processor code bus code cache command. */
    LMEM->PCCCR |= LMEM_PCCCR_INVW0_MASK | LMEM_PCCCR_INVW1_MASK | LMEM_PCCCR_GO_MASK;

    /* Wait until the cache command completes. */
    while (LMEM->PCCCR & LMEM_PCCCR_GO_MASK)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    LMEM->PCCCR &= ~(LMEM_PCCCR_INVW0_MASK | LMEM_PCCCR_INVW1_MASK);
}

/*!
 * brief Invalidates processor code bus cache by range.
 *
 * param address The physical address of cache.
 * param size_byte size of the memory to be invalidated.
 * note Address and size should be aligned to "L1CODCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to L1CODEBUSCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void L1CACHE_InvalidateCodeCacheByRange(uint32_t address, uint32_t size_byte)
{
    uint32_t endAddr = address + size_byte;
    uint32_t pccReg = 0;
    /* Align address to cache line size. */
    uint32_t startAddr = address & ~(L1CODEBUSCACHE_LINESIZE_BYTE - 1U);

    while (startAddr < endAddr)
    {
        /* Set the invalidate by line command and use the physical address. */
        pccReg = (LMEM->PCCLCR & ~LMEM_PCCLCR_LCMD_MASK) | LMEM_PCCLCR_LCMD(1) | LMEM_PCCLCR_LADSEL_MASK;
        LMEM->PCCLCR = pccReg;

        /* Set the address and initiate the command. */
        LMEM->PCCSAR = (startAddr & LMEM_PCCSAR_PHYADDR_MASK) | LMEM_PCCSAR_LGO_MASK;

        /* Wait until the cache command completes. */
        while (LMEM->PCCSAR & LMEM_PCCSAR_LGO_MASK)
        {
        }
        startAddr += L1CODEBUSCACHE_LINESIZE_BYTE;
    }
}

/*!
 * brief Cleans the processor code bus cache.
 *
 */
void L1CACHE_CleanCodeCache(void)
{
    /* Enable the processor code bus to push all modified lines. */
    LMEM->PCCCR |= LMEM_PCCCR_PUSHW0_MASK | LMEM_PCCCR_PUSHW1_MASK | LMEM_PCCCR_GO_MASK;

    /* Wait until the cache command completes. */
    while (LMEM->PCCCR & LMEM_PCCCR_GO_MASK)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    LMEM->PCCCR &= ~(LMEM_PCCCR_PUSHW0_MASK | LMEM_PCCCR_PUSHW1_MASK);
}

/*!
 * brief Cleans processor code bus cache by range.
 *
 * param address The physical address of cache.
 * param size_byte size of the memory to be cleaned.
 * note Address and size should be aligned to "L1CODEBUSCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to L1CODEBUSCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void L1CACHE_CleanCodeCacheByRange(uint32_t address, uint32_t size_byte)
{
    uint32_t endAddr = address + size_byte;
    uint32_t pccReg = 0;
    /* Align address to cache line size. */
    uint32_t startAddr = address & ~(L1CODEBUSCACHE_LINESIZE_BYTE - 1U);

    while (startAddr < endAddr)
    {
        /* Set the push by line command. */
        pccReg = (LMEM->PCCLCR & ~LMEM_PCCLCR_LCMD_MASK) | LMEM_PCCLCR_LCMD(2) | LMEM_PCCLCR_LADSEL_MASK;
        LMEM->PCCLCR = pccReg;

        /* Set the address and initiate the command. */
        LMEM->PCCSAR = (startAddr & LMEM_PCCSAR_PHYADDR_MASK) | LMEM_PCCSAR_LGO_MASK;

        /* Wait until the cache command completes. */
        while (LMEM->PCCSAR & LMEM_PCCSAR_LGO_MASK)
        {
        }
        startAddr += L1CODEBUSCACHE_LINESIZE_BYTE;
    }
}

/*!
 * brief Cleans and invalidates the processor code bus cache.
 *
 */
void L1CACHE_CleanInvalidateCodeCache(void)
{
    /* Push and invalidate all. */
    LMEM->PCCCR |= LMEM_PCCCR_PUSHW0_MASK | LMEM_PCCCR_PUSHW1_MASK | LMEM_PCCCR_INVW0_MASK | LMEM_PCCCR_INVW1_MASK |
                   LMEM_PCCCR_GO_MASK;

    /* Wait until the cache command completes. */
    while (LMEM->PCCCR & LMEM_PCCCR_GO_MASK)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    LMEM->PCCCR &= ~(LMEM_PCCCR_PUSHW0_MASK | LMEM_PCCCR_PUSHW1_MASK | LMEM_PCCCR_INVW0_MASK | LMEM_PCCCR_INVW1_MASK);
}

/*!
 * brief Cleans and invalidate processor code bus cache by range.
 *
 * param address The physical address of cache.
 * param size_byte size of the memory to be Cleaned and Invalidated.
 * note Address and size should be aligned to "L1CODEBUSCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to L1CODEBUSCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void L1CACHE_CleanInvalidateCodeCacheByRange(uint32_t address, uint32_t size_byte)
{
    uint32_t endAddr = address + size_byte;
    uint32_t pccReg = 0;
    /* Align address to cache line size. */
    uint32_t startAddr = address & ~(L1CODEBUSCACHE_LINESIZE_BYTE - 1U);

    while (startAddr < endAddr)
    {
        /* Set the push by line command. */
        pccReg = (LMEM->PCCLCR & ~LMEM_PCCLCR_LCMD_MASK) | LMEM_PCCLCR_LCMD(3) | LMEM_PCCLCR_LADSEL_MASK;
        LMEM->PCCLCR = pccReg;

        /* Set the address and initiate the command. */
        LMEM->PCCSAR = (startAddr & LMEM_PCCSAR_PHYADDR_MASK) | LMEM_PCCSAR_LGO_MASK;

        /* Wait until the cache command completes. */
        while (LMEM->PCCSAR & LMEM_PCCSAR_LGO_MASK)
        {
        }
        startAddr += L1CODEBUSCACHE_LINESIZE_BYTE;
    }
}

#if defined(FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE) && FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE
/*!
 * brief Enables the processor system bus cache.
 *
 */
void L1CACHE_EnableSystemCache(void)
{
    /* First, invalidate the entire cache. */
    L1CACHE_InvalidateSystemCache();

    /* Now enable the cache. */
    LMEM->PSCCR |= LMEM_PSCCR_ENCACHE_MASK;
}

/*!
 * brief Disables the processor system bus cache.
 *
 */
void L1CACHE_DisableSystemCache(void)
{
    /* First, push any modified contents. */
    L1CACHE_CleanSystemCache();

    /* Now disable the cache. */
    LMEM->PSCCR &= ~LMEM_PSCCR_ENCACHE_MASK;
}

/*!
 * brief Invalidates the processor system bus cache.
 *
 */
void L1CACHE_InvalidateSystemCache(void)
{
    /* Enables the processor system bus to invalidate all lines in both ways.
    and Initiate the processor system bus cache command. */
    LMEM->PSCCR |= LMEM_PSCCR_INVW0_MASK | LMEM_PSCCR_INVW1_MASK | LMEM_PSCCR_GO_MASK;

    /* Wait until the cache command completes */
    while (LMEM->PSCCR & LMEM_PSCCR_GO_MASK)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    LMEM->PSCCR &= ~(LMEM_PSCCR_INVW0_MASK | LMEM_PSCCR_INVW1_MASK);
}

/*!
 * brief Invalidates processor system bus cache by range.
 *
 * param address The physical address of cache.
 * param size_byte size of the memory to be invalidated.
 * note Address and size should be aligned to "L1SYSTEMBUSCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to L1SYSTEMBUSCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void L1CACHE_InvalidateSystemCacheByRange(uint32_t address, uint32_t size_byte)
{
    uint32_t endAddr = address + size_byte;
    uint32_t pscReg = 0;
    uint32_t startAddr = address & ~(L1SYSTEMBUSCACHE_LINESIZE_BYTE - 1U); /* Align address to cache line size */

    while (startAddr < endAddr)
    {
        /* Set the invalidate by line command and use the physical address. */
        pscReg = (LMEM->PSCLCR & ~LMEM_PSCLCR_LCMD_MASK) | LMEM_PSCLCR_LCMD(1) | LMEM_PSCLCR_LADSEL_MASK;
        LMEM->PSCLCR = pscReg;

        /* Set the address and initiate the command. */
        LMEM->PSCSAR = (startAddr & LMEM_PSCSAR_PHYADDR_MASK) | LMEM_PSCSAR_LGO_MASK;

        /* Wait until the cache command completes. */
        while (LMEM->PSCSAR & LMEM_PSCSAR_LGO_MASK)
        {
        }
        startAddr += L1SYSTEMBUSCACHE_LINESIZE_BYTE;
    }
}

/*!
 * brief Cleans the processor system bus cache.
 *
 */
void L1CACHE_CleanSystemCache(void)
{
    /* Enable the processor system bus to push all modified lines. */
    LMEM->PSCCR |= LMEM_PSCCR_PUSHW0_MASK | LMEM_PSCCR_PUSHW1_MASK | LMEM_PSCCR_GO_MASK;

    /* Wait until the cache command completes. */
    while (LMEM->PSCCR & LMEM_PSCCR_GO_MASK)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    LMEM->PSCCR &= ~(LMEM_PSCCR_PUSHW0_MASK | LMEM_PSCCR_PUSHW1_MASK);
}

/*!
 * brief Cleans processor system bus cache by range.
 *
 * param address The physical address of cache.
 * param size_byte size of the memory to be cleaned.
 * note Address and size should be aligned to "L1SYSTEMBUSCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to L1SYSTEMBUSCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void L1CACHE_CleanSystemCacheByRange(uint32_t address, uint32_t size_byte)
{
    uint32_t endAddr = address + size_byte;
    uint32_t pscReg = 0;
    uint32_t startAddr = address & ~(L1SYSTEMBUSCACHE_LINESIZE_BYTE - 1U); /* Align address to cache line size. */

    while (startAddr < endAddr)
    {
        /* Set the push by line command. */
        pscReg = (LMEM->PSCLCR & ~LMEM_PSCLCR_LCMD_MASK) | LMEM_PSCLCR_LCMD(2) | LMEM_PSCLCR_LADSEL_MASK;
        LMEM->PSCLCR = pscReg;

        /* Set the address and initiate the command. */
        LMEM->PSCSAR = (startAddr & LMEM_PSCSAR_PHYADDR_MASK) | LMEM_PSCSAR_LGO_MASK;

        /* Wait until the cache command completes. */
        while (LMEM->PSCSAR & LMEM_PSCSAR_LGO_MASK)
        {
        }
        startAddr += L1SYSTEMBUSCACHE_LINESIZE_BYTE;
    }
}

/*!
 * brief Cleans and invalidates the processor system bus cache.
 *
 */
void L1CACHE_CleanInvalidateSystemCache(void)
{
    /* Push and invalidate all. */
    LMEM->PSCCR |= LMEM_PSCCR_PUSHW0_MASK | LMEM_PSCCR_PUSHW1_MASK | LMEM_PSCCR_INVW0_MASK | LMEM_PSCCR_INVW1_MASK |
                   LMEM_PSCCR_GO_MASK;

    /* Wait until the cache command completes. */
    while (LMEM->PSCCR & LMEM_PSCCR_GO_MASK)
    {
    }

    /* As a precaution clear the bits to avoid inadvertently re-running this command. */
    LMEM->PSCCR &= ~(LMEM_PSCCR_PUSHW0_MASK | LMEM_PSCCR_PUSHW1_MASK | LMEM_PSCCR_INVW0_MASK | LMEM_PSCCR_INVW1_MASK);
}

/*!
 * brief Cleans and Invalidates processor system bus cache by range.
 *
 * param address The physical address of cache.
 * param size_byte size of the memory to be Clean and Invalidated.
 * note Address and size should be aligned to "L1SYSTEMBUSCACHE_LINESIZE_BYTE".
 * The startAddr here will be forced to align to L1SYSTEMBUSCACHE_LINESIZE_BYTE if
 * startAddr is not aligned. For the size_byte, application should make sure the
 * alignment or make sure the right operation order if the size_byte is not aligned.
 */
void L1CACHE_CleanInvalidateSystemCacheByRange(uint32_t address, uint32_t size_byte)
{
    uint32_t endAddr = address + size_byte;
    uint32_t pscReg = 0;
    uint32_t startAddr = address & ~(L1SYSTEMBUSCACHE_LINESIZE_BYTE - 1U); /* Align address to cache line size. */

    while (startAddr < endAddr)
    {
        /* Set the push by line command. */
        pscReg = (LMEM->PSCLCR & ~LMEM_PSCLCR_LCMD_MASK) | LMEM_PSCLCR_LCMD(3) | LMEM_PSCLCR_LADSEL_MASK;
        LMEM->PSCLCR = pscReg;

        /* Set the address and initiate the command. */
        LMEM->PSCSAR = (startAddr & LMEM_PSCSAR_PHYADDR_MASK) | LMEM_PSCSAR_LGO_MASK;

        /* Wait until the cache command completes. */
        while (LMEM->PSCSAR & LMEM_PSCSAR_LGO_MASK)
        {
        }
        startAddr += L1SYSTEMBUSCACHE_LINESIZE_BYTE;
    }
}

#endif /* FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE */
#endif /* FSL_FEATURE_SOC_LMEM_COUNT == 1 */

/*!
 * brief Invalidates cortex-m4 L1 instrument cache by range.
 *
 * param address  The start address of the memory to be invalidated.
 * param size_byte  The memory size.
 * note The start address and size_byte should be 16-Byte(FSL_FEATURE_L1ICACHE_LINESIZE_BYTE) aligned.
 */
void L1CACHE_InvalidateICacheByRange(uint32_t address, uint32_t size_byte)
{
#if (FSL_FEATURE_SOC_LMEM_COUNT == 1)
    uint32_t endAddr = address + size_byte;
    uint32_t size = size_byte;

    if (endAddr <= L1CACHE_CODEBUSADDR_BOUNDARY)
    {
        L1CACHE_InvalidateCodeCacheByRange(address, size);
    }
    else if (address <= L1CACHE_CODEBUSADDR_BOUNDARY)
    {
        size = L1CACHE_CODEBUSADDR_BOUNDARY - address;
        L1CACHE_InvalidateCodeCacheByRange(address, size);
#if defined(FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE) && FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE
        size = size_byte - size;
        L1CACHE_InvalidateSystemCacheByRange((L1CACHE_CODEBUSADDR_BOUNDARY + 1), size);
#endif /* FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE */
    }
    else
    {
#if defined(FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE) && FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE
        L1CACHE_InvalidateSystemCacheByRange(address, size);
#endif /* FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE */
    }
#endif /* FSL_FEATURE_SOC_LMEM_COUNT == 1 */
}

/*!
 * brief Cleans cortex-m4 L1 data cache by range.
 *
 * param address  The start address of the memory to be cleaned.
 * param size_byte  The memory size.
 * note The start address and size_byte should be 16-Byte(FSL_FEATURE_L1DCACHE_LINESIZE_BYTE) aligned.
 */
void L1CACHE_CleanDCacheByRange(uint32_t address, uint32_t size_byte)
{
#if (FSL_FEATURE_SOC_LMEM_COUNT == 1)
    uint32_t endAddr = address + size_byte;
    uint32_t size = size_byte;

    if (endAddr <= L1CACHE_CODEBUSADDR_BOUNDARY)
    {
        L1CACHE_CleanCodeCacheByRange(address, size);
    }
    else if (address <= L1CACHE_CODEBUSADDR_BOUNDARY)
    {
        size = L1CACHE_CODEBUSADDR_BOUNDARY - address;
        L1CACHE_CleanCodeCacheByRange(address, size);
#if defined(FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE) && FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE
        size = size_byte - size;
        L1CACHE_CleanSystemCacheByRange((L1CACHE_CODEBUSADDR_BOUNDARY + 1), size);
#endif /* FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE */
    }
    else
    {
#if defined(FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE) && FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE
        L1CACHE_CleanSystemCacheByRange(address, size);
#endif /* FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE */
    }
#endif /* FSL_FEATURE_SOC_LMEM_COUNT == 1 */
}

/*!
 * brief Cleans and Invalidates cortex-m4 L1 data cache by range.
 *
 * param address  The start address of the memory to be clean and invalidated.
 * param size_byte  The memory size.
 * note The start address and size_byte should be 16-Byte(FSL_FEATURE_L1DCACHE_LINESIZE_BYTE) aligned.
 */
void L1CACHE_CleanInvalidateDCacheByRange(uint32_t address, uint32_t size_byte)
{
#if (FSL_FEATURE_SOC_LMEM_COUNT == 1)
    uint32_t endAddr = address + size_byte;
    uint32_t size = size_byte;

    if (endAddr <= L1CACHE_CODEBUSADDR_BOUNDARY)
    {
        L1CACHE_CleanInvalidateCodeCacheByRange(address, size);
    }
    else if (address <= L1CACHE_CODEBUSADDR_BOUNDARY)
    {
        size = L1CACHE_CODEBUSADDR_BOUNDARY - address;
        L1CACHE_CleanInvalidateCodeCacheByRange(address, size);
#if defined(FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE) && FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE
        size = size_byte - size;
        L1CACHE_CleanInvalidateSystemCacheByRange((L1CACHE_CODEBUSADDR_BOUNDARY + 1), size);
#endif /* FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE */
    }
    else
    {
#if defined(FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE) && FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE
        L1CACHE_CleanInvalidateSystemCacheByRange(address, size);
#endif /* FSL_FEATURE_LMEM_HAS_SYSTEMBUS_CACHE */
    }
#endif /* FSL_FEATURE_SOC_LMEM_COUNT == 1 */
}
