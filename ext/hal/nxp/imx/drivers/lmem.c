/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "lmem.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define LMEM_CACHE_LINE_SIZE    32

/*******************************************************************************
 * Code
 ******************************************************************************/

/*******************************************************************************
 * System Cache control functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_EnableSystemCache
 * Description   : This function enable the System Cache.
 *
 *END**************************************************************************/
void LMEM_EnableSystemCache(LMEM_Type *base)
{
    /* set command to invalidate all ways */
    /* and write GO bit to initiate command */
    LMEM_PSCCR_REG(base) = LMEM_PSCCR_INVW1_MASK | LMEM_PSCCR_INVW0_MASK;
    LMEM_PSCCR_REG(base) |= LMEM_PSCCR_GO_MASK;

    /* wait until the command completes */
    while (LMEM_PSCCR_REG(base) & LMEM_PSCCR_GO_MASK);

    /* Enable cache, enable write buffer */
    LMEM_PSCCR_REG(base) = (LMEM_PSCCR_ENWRBUF_MASK | LMEM_PSCCR_ENCACHE_MASK);
    __ISB();
    __DSB();
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_DisableSystemCache
 * Description   : This function disable the System Cache.
 *
 *END**************************************************************************/
void LMEM_DisableSystemCache(LMEM_Type *base)
{
    LMEM_PSCCR_REG(base) = 0x0;
    __ISB();
    __DSB();
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_FlushSystemCache
 * Description   : This function flush the System Cache.
 *
 *END**************************************************************************/
void LMEM_FlushSystemCache(LMEM_Type *base)
{
    LMEM_PSCCR_REG(base) |= LMEM_PSCCR_PUSHW0_MASK | LMEM_PSCCR_PUSHW1_MASK ;
    LMEM_PSCCR_REG(base) |= LMEM_PSCCR_GO_MASK;

    /* wait until the command completes */
    while (LMEM_PSCCR_REG(base) & LMEM_PSCCR_GO_MASK);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_FlushSystemCacheLine
 * Description   : This function is called to push a line out of the System Cache.
 *
 *END**************************************************************************/
static void LMEM_FlushSystemCacheLine(LMEM_Type *base, void *address)
{
    assert((uint32_t)address >= 0x20000000);

    /* Invalidate by physical address */
    LMEM_PSCLCR_REG(base) = LMEM_PSCLCR_LADSEL_MASK | LMEM_PSCLCR_LCMD(2);
    /* Set physical address and activate command */
    LMEM_PSCSAR_REG(base) = ((uint32_t)address & LMEM_PSCSAR_PHYADDR_MASK) | LMEM_PSCSAR_LGO_MASK;

    /* wait until the command completes */
    while (LMEM_PSCSAR_REG(base) & LMEM_PSCSAR_LGO_MASK);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_FlushSystemCacheLines
 * Description   : This function is called to flush the System Cache by
 *                 performing cache copy-backs. It must determine how
 *                 many cache lines need to be copied back and then
 *                 perform the copy-backs.
 *
 *END**************************************************************************/
void LMEM_FlushSystemCacheLines(LMEM_Type *base, void *address, uint32_t length)
{
    void *endAddress = (void *)((uint32_t)address + length);

    address = (void *) ((uint32_t)address & ~(LMEM_CACHE_LINE_SIZE - 1));
    do
    {
        LMEM_FlushSystemCacheLine(base, address);
        address = (void *) ((uint32_t)address + LMEM_CACHE_LINE_SIZE);
    } while (address < endAddress);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_InvalidateSystemCache
 * Description   : This function invalidate the System Cache.
 *
 *END**************************************************************************/
void LMEM_InvalidateSystemCache(LMEM_Type *base)
{
    LMEM_PSCCR_REG(base) |= LMEM_PSCCR_INVW0_MASK | LMEM_PSCCR_INVW1_MASK;
    LMEM_PSCCR_REG(base) |= LMEM_PSCCR_GO_MASK;

    /* wait until the command completes */
    while (LMEM_PSCCR_REG(base) & LMEM_PSCCR_GO_MASK);
    __ISB();
    __DSB();
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_InvalidateSystemCacheLine
 * Description   : This function is called to invalidate a line out of
 *                 the System Cache.
 *
 *END**************************************************************************/
static void LMEM_InvalidateSystemCacheLine(LMEM_Type *base, void *address)
{
    assert((uint32_t)address >= 0x20000000);

    /* Invalidate by physical address */
    LMEM_PSCLCR_REG(base) = LMEM_PSCLCR_LADSEL_MASK | LMEM_PSCLCR_LCMD(1);
    /* Set physical address and activate command */
    LMEM_PSCSAR_REG(base) = ((uint32_t)address & LMEM_PSCSAR_PHYADDR_MASK) | LMEM_PSCSAR_LGO_MASK;

    /* wait until the command completes */
    while (LMEM_PSCSAR_REG(base) & LMEM_PSCSAR_LGO_MASK);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_InvalidateSystemCacheLines
 * Description   : This function is responsible for performing an data
 *                 cache invalidate. It must determine how many cache
 *                 lines need to be invalidated and then perform the
 *                 invalidation.
 *
 *END**************************************************************************/
void LMEM_InvalidateSystemCacheLines(LMEM_Type *base, void *address, uint32_t length)
{
    void *endAddress = (void *)((uint32_t)address + length);
    address = (void *)((uint32_t)address & ~(LMEM_CACHE_LINE_SIZE - 1));

    do
    {
        LMEM_InvalidateSystemCacheLine(base, address);
        address = (void *)((uint32_t)address + LMEM_CACHE_LINE_SIZE);
    } while (address < endAddress);
    __ISB();
    __DSB();
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_EnableCodeCache
 * Description   : This function enable the Code Cache.
 *
 *END**************************************************************************/
void LMEM_EnableCodeCache(LMEM_Type *base)
{
    /* set command to invalidate all ways, enable write buffer */
    /* and write GO bit to initiate command */
    LMEM_PCCCR_REG(base) = LMEM_PCCCR_INVW1_MASK | LMEM_PCCCR_INVW0_MASK;
    LMEM_PCCCR_REG(base) |= LMEM_PCCCR_GO_MASK;

    /* wait until the command completes */
    while (LMEM_PCCCR_REG(base) & LMEM_PCCCR_GO_MASK);

    /* Enable cache, enable write buffer */
    LMEM_PCCCR_REG(base) = (LMEM_PCCCR_ENWRBUF_MASK | LMEM_PCCCR_ENCACHE_MASK);
    __ISB();
    __DSB();
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_DisableCodeCache
 * Description   : This function disable the Code Cache.
 *
 *END**************************************************************************/
void LMEM_DisableCodeCache(LMEM_Type *base)
{
    LMEM_PCCCR_REG(base) = 0x0;
    __ISB();
    __DSB();
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_FlushCodeCache
 * Description   : This function flush the Code Cache.
 *
 *END**************************************************************************/
void LMEM_FlushCodeCache(LMEM_Type *base)
{
    LMEM_PCCCR_REG(base) |= LMEM_PCCCR_PUSHW0_MASK | LMEM_PCCCR_PUSHW1_MASK;
    LMEM_PCCCR_REG(base) |= LMEM_PCCCR_GO_MASK;

    /* wait until the command completes */
    while (LMEM_PCCCR_REG(base) & LMEM_PCCCR_GO_MASK);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_FlushCodeCacheLine
 * Description   : This function is called to push a line out of the
 *                 Code Cache.
 *
 *END**************************************************************************/
static void LMEM_FlushCodeCacheLine(LMEM_Type *base, void *address)
{
    assert((uint32_t)address < 0x20000000);

    /* Invalidate by physical address */
    LMEM_PCCLCR_REG(base) = LMEM_PCCLCR_LADSEL_MASK | LMEM_PCCLCR_LCMD(2);
    /* Set physical address and activate command */
    LMEM_PCCSAR_REG(base) = ((uint32_t)address & LMEM_PCCSAR_PHYADDR_MASK) | LMEM_PCCSAR_LGO_MASK;

    /* wait until the command completes */
    while (LMEM_PCCSAR_REG(base) & LMEM_PCCSAR_LGO_MASK);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_FlushCodeCacheLines
 * Description   : This function is called to flush the instruction
 *                 cache by performing cache copy-backs. It must
 *                 determine how many cache lines need to be copied
 *                 back and then perform the copy-backs.
 *
 *END**************************************************************************/
void LMEM_FlushCodeCacheLines(LMEM_Type *base, void *address, uint32_t length)
{
    void *endAddress = (void *)((uint32_t)address + length);

    address = (void *) ((uint32_t)address & ~(LMEM_CACHE_LINE_SIZE - 1));
    do
    {
        LMEM_FlushCodeCacheLine(base, address);
        address = (void *)((uint32_t)address + LMEM_CACHE_LINE_SIZE);
    } while (address < endAddress);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_InvalidateCodeCache
 * Description   : This function invalidate the Code Cache.
 *
 *END**************************************************************************/
void LMEM_InvalidateCodeCache(LMEM_Type *base)
{
    LMEM_PCCCR_REG(base) |= LMEM_PCCCR_INVW0_MASK | LMEM_PCCCR_INVW1_MASK;
    LMEM_PCCCR_REG(base) |= LMEM_PCCCR_GO_MASK;

    /* wait until the command completes */
    while (LMEM_PCCCR_REG(base) & LMEM_PCCCR_GO_MASK);
    __ISB();
    __DSB();
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_InvalidateCodeCacheLine
 * Description   : This function is called to invalidate a line out
 *                 of the Code Cache.
 *
 *END**************************************************************************/
static void LMEM_InvalidateCodeCacheLine(LMEM_Type *base, void *address)
{
    assert((uint32_t)address < 0x20000000);

    /* Invalidate by physical address */
    LMEM_PCCLCR_REG(base) = LMEM_PCCLCR_LADSEL_MASK | LMEM_PCCLCR_LCMD(1);
    /* Set physical address and activate command */
    LMEM_PCCSAR_REG(base) = ((uint32_t)address & LMEM_PCCSAR_PHYADDR_MASK) | LMEM_PCCSAR_LGO_MASK;

    /* wait until the command completes */
    while (LMEM_PCCSAR_REG(base) & LMEM_PCCSAR_LGO_MASK);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LMEM_InvalidateCodeCacheLines
 * Description   : This function is responsible for performing an
 *                 Code Cache invalidate. It must determine
 *                 how many cache lines need to be invalidated and then
 *                 perform the invalidation.
 *
 *END**************************************************************************/
void LMEM_InvalidateCodeCacheLines(LMEM_Type *base, void *address, uint32_t length)
{
    void *endAddress = (void *)((uint32_t)address + length);
    address = (void *)((uint32_t)address & ~(LMEM_CACHE_LINE_SIZE - 1));

    do
    {
        LMEM_InvalidateCodeCacheLine(base, address);
        address = (void *)((uint32_t)address + LMEM_CACHE_LINE_SIZE);
    } while (address < endAddress);
    __ISB();
    __DSB();
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
